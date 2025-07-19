/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file network/asio/caresolver.cpp
  *
  *   DNS resolver that uses c-ares resolving library ans ASIO events
  *
  */

/****************************************************************************/

//! @todo Refactor cares to use modern not deprecated API
#define CARES_NO_DEPRECATED

#include <ares.h>
#include <ares_build.h>

#if defined(CARES_HAVE_WINSOCK2_H)
#define HAVE_CLOSESOCKET
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET (ares_socket_t)(~0)
#endif

/*
 * Macro SOCKERRNO / SET_SOCKERRNO() returns / sets the *socket-related* errno
 * (or equivalent) on this platform to hide platform details to code using it.
 */

#ifdef USE_WINSOCK
#  define SOCKERRNO        ((int)WSAGetLastError())
#  define SET_SOCKERRNO(x) (WSASetLastError((int)(x)))
#else
#  define SOCKERRNO        (errno)
#  define SET_SOCKERRNO(x) (errno = (x))
#endif

#if defined(HAVE_CLOSESOCKET)
#  define sclose(x) closesocket((x))
#elif defined(HAVE_CLOSESOCKET_CAMEL)
#  define sclose(x) CloseSocket((x))
#elif defined(HAVE_CLOSE_S)
#  define sclose(x) close_s((x))
#else
#  define sclose(x) close((x))
#endif

#include <hatn/common/thread.h>
#include <hatn/common/asiotimer.h>
#include <hatn/common/translate.h>

#include <hatn/network/networkerrorcodes.h>
#include <hatn/network/asio/careslib.h>
#include <hatn/network/asio/caresolver.h>

HATN_NETWORK_NAMESPACE_BEGIN

using StringBuf=common::StringOnStackT<256>;

static const int CHECK_TIMEOUTS_PERIOD=1000; // ms

/********************** CaresLib **************************/

const common::pmr::AllocatorFactory* CaresLib::m_allocatorFactory=nullptr;

namespace {

std::unique_ptr<common::pmr::polymorphic_allocator<char>> Allocator;

} // anonymous namespace

#if 0

static void* myMalloc(size_t size)
{
    size+=sizeof(size_t);
    auto buf=Allocator->allocate(size);
    auto sizeVal=reinterpret_cast<size_t*>(buf);
    *sizeVal=size;
    return (buf+sizeof(size_t));
}
static void myFree(void* ptr)
{
    if (ptr==nullptr)
    {
        return;
    }

    auto buf=reinterpret_cast<char*>(ptr)-sizeof(size_t);
    auto size=reinterpret_cast<size_t*>(buf);
    Allocator->deallocate(buf,*size);
}
static void* myRealloc(void *ptr, size_t size)
{
    if (ptr==nullptr)
    {
        return myMalloc(size);
    }

    auto newSize=size+sizeof(size_t);
    auto buf=Allocator->allocate(newSize);
    auto prevBuf=reinterpret_cast<char*>(ptr)-sizeof(size_t);
    auto prevSize=reinterpret_cast<size_t*>(prevBuf);
    auto copySize=(std::min)(size,(*prevSize-sizeof(size_t)));
    memcpy(buf+sizeof(size_t),ptr,copySize);
    auto sizeVal=reinterpret_cast<size_t*>(buf);
    *sizeVal=newSize;
    Allocator->deallocate(prevBuf,*prevSize);
    return (buf+sizeof(size_t));
}
#endif

//---------------------------------------------------------------
Error CaresLib::init(const common::pmr::AllocatorFactory *allocatorFactory)
{
    int res=ARES_SUCCESS;
#if 0
    if (allocatorFactory!=nullptr)
    {
        m_allocatorFactory=allocatorFactory;
        Allocator=std::make_unique<common::pmr::polymorphic_allocator<char>>(allocatorFactory->dataMemoryResource());
        res=ares_library_init_mem(ARES_LIB_INIT_ALL,myMalloc,myFree,myRealloc);
    }
    else
#else
    std::ignore=allocatorFactory;
#endif
    {
        res=ares_library_init(ARES_LIB_INIT_ALL);
    }
    if (res!=ARES_SUCCESS)
    {
        return caresError(res);
    }
    return Error();
}

//---------------------------------------------------------------
void CaresLib::cleanup()
{
    ares_library_cleanup();
    Allocator.reset();
    m_allocatorFactory=nullptr;
}

#if defined(ANDROID) || defined(__ANDROID__)

//---------------------------------------------------------------

Error CaresLib::initAndroid(jobject connectivityManager)
{
    return caresError(ares_library_init_android(connectivityManager));
}

//---------------------------------------------------------------

bool CaresLib::isAndroidInitialized()
{
    return ares_library_android_initialized()==ARES_SUCCESS;
}

//---------------------------------------------------------------

void CaresLib::initJvm(JavaVM *jvm)
{
    ares_library_init_jvm(jvm);
}

#endif

/********************** CaresErrorCategory **************************/

//---------------------------------------------------------------
std::string CaresErrorCategory::message(int code) const
{
    std::string result;
    switch (code)
    {
        HATN_CARES_ERRORS(HATN_ERROR_MESSAGE)

        default:
            result=_TR("unknown error");
    }

    return result;
}

//---------------------------------------------------------------
const char* CaresErrorCategory::codeString(int code) const
{
    return errorString(code,CaresErrorStrings);
}

//---------------------------------------------------------------
const CaresErrorCategory& CaresErrorCategory::getCategory() noexcept
{
    static CaresErrorCategory inst;
    return inst;
}

namespace asio {

/*********************** CaResolverTraits **************************/

namespace {

constexpr const int MAX_ADDRTTL_RECORDS=16;

constexpr const int DNSCLASS_IN=1;

constexpr const int NS_TYPE_A=1;
//constexpr const int NS_TYPE_CNAME=5;
constexpr const int NS_TYPE_MX=15;
//constexpr const int NS_TYPE_TXT=16;
constexpr const int NS_TYPE_AAAA=28;
constexpr const int NS_TYPE_SRV=33;

enum class QueryStep : int
{
    LocalFile,

    RecordA,
    RecordAAAA,
    RecordSRV,
    RecordMX,

    Next,
    Done
};

struct Query : public common::EnableManaged<Query>
{
    Query(
            std::weak_ptr<CaResolverTraits_p> impl,
            std::function<void (const Error &, std::vector<asio::IpEndpoint>)> callback,
            const common::TaskContextShared& context,
            IpVersion ipVersion,
            const lib::string_view& name=lib::string_view{},
            uint16_t port=0
          ) : callback(std::move(callback)),
              impl(std::move(impl)),
              context(context),
              name(name),
              port(port),
              ipVersion(ipVersion),
              step(QueryStep::LocalFile),
              depth(0),
              intermediateIndex(0)
    {
    }

    std::function<void (const Error &, std::vector<asio::IpEndpoint>)> callback;
    std::weak_ptr<CaResolverTraits_p> impl;
    common::WeakPtr<common::TaskContext> context;
    StringBuf name;
    uint16_t port;
    IpVersion ipVersion;

    std::vector<asio::IpEndpoint> endpoints;
    std::vector<std::pair<StringBuf,uint16_t>> intermediateEndpoints; // for SRV and CNAME
    QueryStep step;
    size_t depth;
    size_t intermediateIndex;

    common::EmbeddedSharedPtr<Query> cyclicRefToSelf;
    Error lastError;
};

}

static void queryCb(void *arg, int status, int timeouts, unsigned char *abuf, int alen);

static bool setSocketBlockingMode(ares_socket_t fd, bool blocking)
{
   if (fd == ARES_SOCKET_BAD) return false;

#ifdef _WIN32
   unsigned long mode = blocking ? 0 : 1;
   return (ioctlsocket(fd, FIONBIO, &mode) == 0) ? true : false;
#else
   int flags = fcntl(fd, F_GETFL, 0);
   if (flags == -1) return false;
   flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
   return (fcntl(fd, F_SETFL, flags) == 0) ? true : false;
#endif
}

class CaResolverTraits_p : public std::enable_shared_from_this<CaResolverTraits_p>
{
    public:

        CaResolverTraits_p(CaResolver* resolver,CaResolverTraits* obj, common::Thread* thread, bool useLocalHostsFile) noexcept
            : channel(nullptr),
              obj(obj),
              cancel(false),
              useLocalHostsFile(useLocalHostsFile),
              timeoutsTimer(common::makeShared<common::AsioDeadlineTimer>(thread)),
              closedSocketFd(ARES_SOCKET_BAD),
              m_resolver(resolver),
              m_thread(thread)
        {
        }

        ares_channel channel;
        CaResolverTraits* obj;
        bool cancel;
        bool useLocalHostsFile;

        struct Socket
        {
            Socket(boost::asio::io_context& asioContext,const boost::asio::ip::udp& udpProto,ares_socket_t sockID)
                : proto(IpProtocol::UDP),
                  sock(boost::asio::ip::udp::socket(asioContext,udpProto,sockID))
            {}

            Socket(boost::asio::io_context& asioContext,const boost::asio::ip::tcp& tcpProto,ares_socket_t sockID)
                : proto(IpProtocol::TCP),
                  sock(boost::asio::ip::tcp::socket(asioContext,tcpProto,sockID))
            {}

            IpProtocol proto;
            lib::variant<
                boost::asio::ip::udp::socket,
                boost::asio::ip::tcp::socket
            > sock;
        };

        std::map<ares_socket_t,Socket> sockets;
        common::SharedPtr<common::AsioDeadlineTimer> timeoutsTimer;
        ares_socket_t closedSocketFd;

        CaResolver* m_resolver;
        common::Thread* m_thread;

        ares_socket_t addSocket(int af, int type, int protocol)
        {
            ares_socket_t sockID=socket(af, type, protocol);
            if (sockID!=INVALID_SOCKET)
            {
                if (!setSocketBlockingMode(sockID,false))
                {
                    // DCS_DEBUG_LVL(dnsresolver,1,HATN_FORMAT("Failed to set socket non-blocking {}",sockID));
                    sclose(sockID);
                    return INVALID_SOCKET;
                }
                try
                {
                    IpProtocol proto=(type==SOCK_STREAM)?IpProtocol::TCP:IpProtocol::UDP;
                    if (proto==IpProtocol::TCP)
                    {
                        auto ipV=(af==AF_INET6)?boost::asio::ip::tcp::v6():boost::asio::ip::tcp::v4();
                        sockets.emplace(std::piecewise_construct,
                                        std::forward_as_tuple(sockID),
                                        std::forward_as_tuple(m_resolver->thread()->asioContextRef(),ipV,sockID));
                    }
                    else
                    {
                        auto ipV=(af==AF_INET6)?boost::asio::ip::udp::v6():boost::asio::ip::udp::v4();
                        sockets.emplace(std::piecewise_construct,
                                        std::forward_as_tuple(sockID),
                                        std::forward_as_tuple(m_resolver->thread()->asioContextRef(),ipV,sockID));
                    }
                }
                catch (const boost::system::system_error& ec)
                {
                    // DCS_WARN(dnsresolver,HATN_FORMAT("Failed to intialize BOOST ASIO socket in CaResolverTraits: ({}) {}",
                    //          ec.code().value(),ec.what()));
                    SET_SOCKERRNO(ec.code().value());
                    return INVALID_SOCKET;
                }
            }
            // DCS_DEBUG_LVL(dnsresolver,1,HATN_FORMAT("Add socket {}",sockID));
            return sockID;
        }
        int closeSocket(ares_socket_t sockID)
        {
            // DCS_DEBUG_LVL(dnsresolver,1,HATN_FORMAT("Close socket {}",sockID));

            closedSocketFd=sockID;
            auto it=sockets.find(sockID);
            if (it!=sockets.end())
            {
                boost::system::error_code ec;
                auto& s=it->second;
                if (s.proto==IpProtocol::TCP)
                {
                    lib::variantGet<boost::asio::ip::tcp::socket>(s.sock).cancel(ec);
                    lib::variantGet<boost::asio::ip::tcp::socket>(s.sock).release(ec);
                }
                else
                {
                    lib::variantGet<boost::asio::ip::udp::socket>(s.sock).cancel(ec);
                    lib::variantGet<boost::asio::ip::udp::socket>(s.sock).release(ec);
                }
                if (ec)
                {
                    // DCS_WARN(dnsresolver,HATN_FORMAT("Failed to release BOOST ASIO socket in CaResolverTraits: ({}) {}",
                    //          ec.value(),ec.message()));
                }
            }
            sockets.erase(sockID);
            return sclose(sockID);
        }

        int parseHostEnt(const common::SharedPtr<Query>& query,struct hostent* host)
        {
            int res=ARES_ENOTFOUND;
            if (host->h_addr_list!=NULL && (host->h_addrtype==AF_INET || host->h_addrtype==AF_INET6))
            {
                auto addrBegin=host->h_addr_list;
                while(*addrBegin!=NULL)
                {
                    if (host->h_addrtype==AF_INET6)
                    {
                        boost::asio::ip::address_v6::bytes_type bytes;
                        memcpy(bytes.data(),*addrBegin,bytes.size());
                        query->endpoints.emplace_back(boost::asio::ip::make_address_v6(bytes),query->port);
                        res=ARES_SUCCESS;
                    }
                    else if (host->h_addrtype==AF_INET)
                    {
                        boost::asio::ip::address_v4::bytes_type bytes;
                        memcpy(bytes.data(),*addrBegin,bytes.size());
                        query->endpoints.emplace_back(boost::asio::ip::make_address_v4(bytes),query->port);
                        res=ARES_SUCCESS;
                    }

                    // const auto& ep=query->endpoints.back();
                    // DCS_DEBUG(dnsresolver,HATN_FORMAT("Resolved endpoint {}:{} for {}"
                    //                                 ,ep.address().to_string(),ep.port(),query->name.c_str()
                    //           ));

                    ++addrBegin;
                }
            }
            ares_free_hostent(host);
            return res;
        }

        void processQuery(common::SharedPtr<Query> query)
        {
            auto ctx=query->context.lock();
            if (cancel || !ctx)
            {
                query->cyclicRefToSelf.reset();
                query->callback(Error(CommonError::ABORTED),std::vector<asio::IpEndpoint>{});
                return;
            }
            auto self=shared_from_this();
            m_thread->execAsync(
                [self,query{std::move(query)}]()
                {
                    self->processQueryAsync(query);
                }
            );
        }

        void processQueryAsync(const common::SharedPtr<Query>& query)
        {
            auto ctx=query->context.lock();
            if (cancel || !ctx)
            {
                query->cyclicRefToSelf.reset();
                query->callback(Error(CommonError::ABORTED),std::vector<asio::IpEndpoint>{});
                return;
            }

            switch (query->step)
            {
                case(QueryStep::LocalFile):
                {
                    query->step=QueryStep::RecordA;
                    if (useLocalHostsFile)
                    {
                        // DCS_DEBUG_LVL(dnsresolver,1,HATN_FORMAT("Step LocalFile query {} ...",query->name.c_str()));

                        int resultA=ARES_ENOTFOUND;
                        int resultAAAA=ARES_ENOTFOUND;

                        // try to query local host files
                        if (IpVersionHasV4(query->ipVersion))
                        {
                            struct hostent* host=nullptr;
                            resultA=ares_gethostbyname_file(channel,query->name.c_str(),AF_INET,&host);
                            if (resultA==ARES_SUCCESS)
                            {
                                resultA=parseHostEnt(query,host);
                            }
                        }
                        if (IpVersionHasV6(query->ipVersion))
                        {
                            struct hostent* host=nullptr;
                            resultAAAA=ares_gethostbyname_file(channel,query->name.c_str(),AF_INET6,&host);
                            if (resultAAAA==ARES_SUCCESS)
                            {
                                resultAAAA=parseHostEnt(query,host);
                            }
                        }
                        if (resultA==ARES_SUCCESS || resultAAAA==ARES_SUCCESS)
                        {
                            query->step=QueryStep::Done;
                        }
                    }
                    processQuery(query);
                }
                break;

                case(QueryStep::RecordA):
                {
                    if (!IpVersionHasV4(query->ipVersion))
                    {
                        // exclude IPv4 addresses
                        query->step=QueryStep::RecordAAAA;
                        processQuery(query);
                    }
                    else
                    {
                        // DCS_DEBUG_LVL(dnsresolver,1,HATN_FORMAT("Step RecordA query {} ...",query->name.c_str()));

                        // if IPv4 then try to read A records
                        ares_query(channel,query->name.c_str(),DNSCLASS_IN,NS_TYPE_A,queryCb,query.get());
                    }
                }
                break;

                case(QueryStep::RecordAAAA):
                {
                    if (!IpVersionHasV6(query->ipVersion))
                    {
                        // exclude IPv6 addresses
                        query->step=QueryStep::Next;
                        processQuery(query);
                    }
                    else
                    {
                        // DCS_DEBUG_LVL(dnsresolver,1,HATN_FORMAT("Step RecordAAAA query {} ...",query->name.c_str()));

                        // if IPv6 then try to read AAAA records
                        ares_query(channel,query->name.c_str(),DNSCLASS_IN,NS_TYPE_AAAA,queryCb,query.get());
                    }
                }
                break;

                case(QueryStep::Next):
                {
                    if (query->intermediateIndex>0)
                    {
                        // DCS_DEBUG_LVL(dnsresolver,1,HATN_FORMAT("Step RecordNext for intermediate endpoints for query {} for {} ...",query->name.c_str(),IpVersionStr(query->ipVersion)));

                        // intermediate endpoints are from SRV/MX records
                        auto& ep=query->intermediateEndpoints[--query->intermediateIndex];

                        // record needs to be further resolved to IP address(es)
                        query->name.load(ep.first.data(),ep.first.size());
                        query->port=ep.second;
                        query->step=QueryStep::RecordA;
                        processQuery(query);
                    }
                    else
                    {
                        query->step=QueryStep::Done;
                        processQuery(query);
                    }
                }
                break;

                case(QueryStep::RecordSRV):
                {
                    // DCS_DEBUG_LVL(dnsresolver,1,HATN_FORMAT("Step RecordSRV query {} for {} ...",query->name.c_str(),IpVersionStr(query->ipVersion)));

                    // try to read SRV records
                    ares_query(channel,query->name.c_str(),DNSCLASS_IN,NS_TYPE_SRV,queryCb,query.get());
                }
                break;

                case(QueryStep::RecordMX):
                {
                    // DCS_DEBUG_LVL(dnsresolver,1,HATN_FORMAT("Step RecordMX query {} for {} ...",query->name.c_str(),IpVersionStr(query->ipVersion)));

                    // try to read MX records
                    ares_query(channel,query->name.c_str(),DNSCLASS_IN,NS_TYPE_MX,queryCb,query.get());
                }
                break;

                case(QueryStep::Done):
                {
                    query->cyclicRefToSelf.reset();

                    // call query callback
                    Error err=query->endpoints.empty()?query->lastError:Error();

                    // DCS_DEBUG(dnsresolver,HATN_FORMAT("Done query {}:{} for {}, status={}, endpoints count={}"
                    //                                 ,query->name.c_str(),query->port,IpVersionStr(query->ipVersion),
                    //                                 err.value(),query->endpoints.size())
                    //           );

                    query->callback(err,std::move(query->endpoints));
                }
                break;
            }
        }
};

template <typename SocketClass>
static void waitSocketEvent(std::shared_ptr<CaResolverTraits_p> impl,CaResolverTraits_p::Socket& s, ares_socket_t socketFd, typename SocketClass::wait_type waitType)
{
    // DCS_DEBUG_LVL(dnsresolver,1,HATN_FORMAT("Wait socket event id={}, direction={}",socketFd,static_cast<int>(waitType)));

    auto cb=[socketFd,impl{std::move(impl)},waitType,&s](const boost::system::error_code& ec) mutable
    {
        // DCS_DEBUG_LVL(dnsresolver,1,HATN_FORMAT("Wait socket invoked id={}, direction={}, ec={}, cancelled={}",socketFd,ec.value(),static_cast<int>(waitType),impl->cancel));

        if (!impl->cancel && ec!=boost::asio::error::operation_aborted)
        {
            ares_socket_t readFd=(waitType==boost::asio::ip::udp::socket::wait_read)?socketFd:ARES_SOCKET_BAD;
            ares_socket_t writeFd=(waitType==boost::asio::ip::udp::socket::wait_write)?socketFd:ARES_SOCKET_BAD;

            // DCS_DEBUG_LVL(dnsresolver,1,"Before ares_process_fd");
            impl->closedSocketFd=ARES_SOCKET_BAD;
            ares_process_fd(impl->channel,readFd,writeFd);

            // DCS_DEBUG_LVL(dnsresolver,1,"After ares_process_fd");

            if (impl->closedSocketFd!=socketFd)
            {
                // DCS_DEBUG_LVL(dnsresolver,1,"Calling next waitSocketEvent");
                waitSocketEvent<SocketClass>(std::move(impl),s,socketFd,waitType);
            }
            else
            {
                // DCS_DEBUG_LVL(dnsresolver,1,"Skip calling next waitSocketEvent");
            }
        }
    };
    // async operation is safe and no guarded handler is needed because impl is shared_ptr
    lib::variantGet<SocketClass>(s.sock).async_wait(
                    waitType,
                    cb
                );
}

template <typename SocketClass>
static void socketStateChanged(std::shared_ptr<CaResolverTraits_p> impl,CaResolverTraits_p::Socket& s,ares_socket_t socketFd,int readable,int writable)
{
    // DCS_DEBUG_LVL(dnsresolver,1,HATN_FORMAT("Socket state changed id={}, readable={}, writable={}",socketFd,readable,writable));

    boost::system::error_code ec;
    lib::variantGet<SocketClass>(s.sock).cancel(ec);
    if (readable)
    {
        waitSocketEvent<SocketClass>(impl,s,socketFd,SocketClass::wait_read);
    }
    if (writable)
    {
        waitSocketEvent<SocketClass>(impl,s,socketFd,SocketClass::wait_write);
    }
}

static void socketStateCb(void *data, ares_socket_t socketFd, int readable, int writable)
{
    auto obj=reinterpret_cast<CaResolverTraits_p*>(data);
    auto it=obj->sockets.find(socketFd);
    if (it!=obj->sockets.end())
    {
        // DCS_DEBUG_LVL(dnsresolver,1,HATN_FORMAT("Socket state cb id={}, readable={}, writable={}",socketFd,readable,writable));

        auto& s=it->second;
        if (s.proto==IpProtocol::TCP)
        {
            socketStateChanged<boost::asio::ip::tcp::socket>(obj->shared_from_this(),s,socketFd,readable,writable);
        }
        else
        {
            socketStateChanged<boost::asio::ip::udp::socket>(obj->shared_from_this(),s,socketFd,readable,writable);
        }
    }
}

static ares_socket_t fSocket(int af, int type, int protocol, void *data)
{
    auto obj=reinterpret_cast<CaResolverTraits_p*>(data);
    return obj->addSocket(type,af,protocol);
}
static int fClose(ares_socket_t sockID, void *data)
{
    auto obj=reinterpret_cast<CaResolverTraits_p*>(data);
    return obj->closeSocket(sockID);
}

//---------------------------------------------------------------
static void queryCb(void *arg, int status, int, unsigned char *abuf, int alen)
{
    auto q=reinterpret_cast<Query*>(arg);
    auto query=q->cyclicRefToSelf;
    auto obj=query->impl.lock();
    auto ctx=query->context.lock();

    if (!obj || !ctx || obj->cancel || status==ARES_ECANCELLED)
    {
        query->callback(Error(CommonError::ABORTED),std::vector<asio::IpEndpoint>{});
        return;
    }

    auto checkResult=[](int result,Error& err)
    {        
        if (result==ARES_SUCCESS)
        {
            return true;
        }
        err=caresError(result);
        return false;
    };

    Error& err=query->lastError;
    err=caresError(status);
    struct hostent* host=nullptr;
    int naddrttl=MAX_ADDRTTL_RECORDS;
    switch (q->step)
    {
        case(QueryStep::RecordA):
        {
            std::array<ares_addrttl,MAX_ADDRTTL_RECORDS> addrttl;
            if (!err && checkResult(ares_parse_a_reply(abuf,alen,&host,addrttl.data(),&naddrttl),err))
            {
                // parse hostent
                if (obj->parseHostEnt(query,host)!=ARES_SUCCESS)
                {
                    // parse addrttl
                    for (int i=0;i<naddrttl;i++)
                    {
                        boost::asio::ip::address_v4::bytes_type bytes;
                        memcpy(bytes.data(),&addrttl[i].ipaddr,bytes.size());
                        query->endpoints.emplace_back(boost::asio::ip::make_address_v4(bytes),query->port);

                        // const auto& ep=query->endpoints.back();
                        // DCS_DEBUG(dnsresolver,HATN_FORMAT("Resolved endpoint {}:{} for {}"
                        //                                 ,ep.address().to_string(),ep.port(),query->name.c_str()
                        //           ));
                    }
                }
            }
            else
            {
                // DCS_DEBUG(dnsresolver,HATN_FORMAT("Failed to resolve A records for {} - {} ({})",q->name.c_str(),err.message(),err.value()));
            }

            // go to IPv6 step if not timeouted and not DNS server connection refused
            if (err.value()!=ARES_ETIMEOUT
                &&
                err.value()!=ARES_ECONNREFUSED
                )
            {
                q->step=QueryStep::RecordAAAA;
            }
            else
            {
                q->step=QueryStep::Next;
            }
        }
        break;

        case(QueryStep::RecordAAAA):
        {
            std::array<ares_addr6ttl,MAX_ADDRTTL_RECORDS> addrttl;
            if (!err && checkResult(ares_parse_aaaa_reply(abuf,alen,&host,addrttl.data(),&naddrttl),err))
            {
                // parse hostent
                if (obj->parseHostEnt(query,host)!=ARES_SUCCESS)
                {
                    // parse addrttl
                    for (int i=0;i<naddrttl;i++)
                    {
                        boost::asio::ip::address_v6::bytes_type bytes;
                        memcpy(bytes.data(),&addrttl[i].ip6addr,bytes.size());
                        query->endpoints.emplace_back(boost::asio::ip::make_address_v6(bytes),query->port);

                        // const auto& ep=query->endpoints.back();
                        // DCS_DEBUG(dnsresolver,HATN_FORMAT("Resolved endpoint {}:{} for {}"
                        //                                 ,ep.address().to_string(),ep.port(),query->name.c_str()
                        //           ));
                    }
                }
            }
            else
            {
                // DCS_DEBUG(dnsresolver,HATN_FORMAT("Failed to resolve AAAA records for {} - {} ({})",q->name.c_str(),err.message(),err.value()));
            }

            // go to the next step
            q->step=QueryStep::Next;
        }
        break;

        case(QueryStep::RecordSRV):
        {
            ares_srv_reply* srv=nullptr;
            if (!err && checkResult(ares_parse_srv_reply(abuf,alen,&srv),err))
            {
                std::vector<ares_srv_reply*> records;
                auto srvNext=srv;
                while (srvNext!=NULL)
                {
                    records.push_back(srvNext);
                    srvNext=srvNext->next;
                }
                std::sort(records.begin(),records.end(),
                            [](const ares_srv_reply* left,const ares_srv_reply* right)
                            {
                                if (left->priority<right->priority)
                                {
                                    return true;
                                }
                                if (left->priority==right->priority)
                                {
                                    return left->weight>right->weight;
                                }
                                return false;
                            }
                          );
                for (size_t i=0;i<records.size();i++)
                {
                    auto& record=records[i];
                    uint16_t port=(record->port==0)?q->port:record->port;
                    boost::system::error_code ec;
                    boost::asio::ip::address addr=boost::asio::ip::make_address(record->host,ec);
                    if (ec)
                    {
                        q->intermediateEndpoints.emplace_back(record->host,port);
                        ++q->intermediateIndex;

                        // DCS_DEBUG_LVL(dnsresolver,1,HATN_FORMAT("Resolved intermediate SRV endpoint {}:{} for {}"
                        //                                 ,record->host,port,query->name.c_str()
                        //           ));
                    }
                    else if (
                            (addr.is_v4() && IpVersionHasV4(q->ipVersion))
                            ||
                            (addr.is_v6() && IpVersionHasV6(q->ipVersion))
                           )
                    {
                        q->endpoints.emplace_back(std::move(addr),query->port);

                        // DCS_DEBUG_LVL(dnsresolver,1,HATN_FORMAT("Resolved final SRV endpoint {}:{} for {}"
                        //                                 ,record->host,port,query->name.c_str()
                        //           ));
                    }
                }

                if (srv!=NULL)
                {
                    ares_free_data(srv);
                }
            }
            else
            {
                // DCS_DEBUG(dnsresolver,HATN_FORMAT("Failed to resolve SRV records for {} - {} ({})",q->name.c_str(),err.message(),err.value()));
            }

            // go to the next step
            q->step=QueryStep::Next;
        }
        break;

        case(QueryStep::RecordMX):
        {
            ares_mx_reply* mx=nullptr;
            if (!err && checkResult(ares_parse_mx_reply(abuf,alen,&mx),err))
            {
                std::vector<ares_mx_reply*> records;
                auto mxNext=mx;
                while (mxNext!=NULL)
                {
                    records.push_back(mxNext);
                    mxNext=mxNext->next;
                }
                std::sort(records.begin(),records.end(),
                            [](const ares_mx_reply* left,const ares_mx_reply* right)
                            {
                                return left->priority<right->priority;
                            }
                          );
                for (size_t i=0;i<records.size();i++)
                {
                    auto& record=records[i];
                    boost::system::error_code ec;
                    boost::asio::ip::address addr=boost::asio::ip::make_address(record->host,ec);
                    if (ec)
                    {
                        q->intermediateEndpoints.emplace_back(record->host,query->port);
                        ++q->intermediateIndex;

                        // DCS_DEBUG_LVL(dnsresolver,1,HATN_FORMAT("Resolved intermediate MX endpoint {} for {}"
                        //                                 ,record->host,query->name.c_str()
                        //           ));
                    }
                    else if (
                            (addr.is_v4() && IpVersionHasV4(q->ipVersion))
                            ||
                            (addr.is_v6() && IpVersionHasV6(q->ipVersion))
                           )
                    {
                        q->endpoints.emplace_back(std::move(addr),query->port);

                        // DCS_DEBUG_LVL(dnsresolver,1,HATN_FORMAT("Resolved final MX endpoint {} for {}"
                        //                                 ,record->host,query->name.c_str()
                        //           ));
                    }
                }

                if (mx!=NULL)
                {
                    ares_free_data(mx);
                }
            }
            else
            {
                // DCS_DEBUG(dnsresolver,HATN_FORMAT("Failed to resolve MX records for {} - {} ({})",q->name.c_str(),err.message(),err.value()));
            }

            // go to the next step
            q->step=QueryStep::Next;
        }
        break;

        case(QueryStep::Next):
        {
            Assert(false,"Shouldn't reach QueryStep::Next branch in CaResolverTraits queryCb");
        }
        break;

        case(QueryStep::Done):
        {
            Assert(false,"Shouldn't reach QueryStep::Done branch in CaResolverTraits queryCb");
        }
        break;

        case(QueryStep::LocalFile):
        {
            Assert(false,"Shouldn't reach QueryStep::LocalFile branch in CaResolverTraits queryCb");
        }
        break;
    }
    obj->processQuery(query);
}

static ares_socket_functions SocketFunctions={
    fSocket,
    fClose,
    NULL,
    NULL,
    NULL
};

//---------------------------------------------------------------
CaResolverTraits::CaResolverTraits(
        CaResolver* resolver,
        const std::vector<NameServer> &nameServers,
        const std::string &resolvConfPath
    ) : d(std::make_shared<CaResolverTraits_p>(resolver,this,resolver->thread(),true))
{
    auto checkStatus=[](int status)
    {
        if (status!=ARES_SUCCESS)
        {
            throw common::ErrorException(caresError(status));
        }
    };

    // init ares channel
    int optmask=0;
    ares_options options;
    optmask|=ARES_OPT_SOCK_STATE_CB;
    optmask|=ARES_OPT_TIMEOUTMS;
    options.sock_state_cb=socketStateCb;
    options.sock_state_cb_data=d.get();
    options.timeout=300;
    if (!resolvConfPath.empty())
    {
        options.resolvconf_path=const_cast<char*>(resolvConfPath.c_str());
        optmask|=ARES_OPT_RESOLVCONF;
    }
    checkStatus(ares_init_options(&d->channel,&options,optmask));

    try
    {
        // set nameservers
        if (!nameServers.empty())
        {
            std::vector<struct ares_addr_port_node> servers(nameServers.size());
            for (size_t i=0;i<nameServers.size();i++)
            {
                const auto& nameServer=nameServers[i];
                auto& server=servers[i];
                server.tcp_port=nameServer.tcpPort;
                server.udp_port=nameServer.udpPort;
                if (nameServer.address.is_v4())
                {
                    server.family=AF_INET;
                    auto ipv4=nameServer.address.to_v4().to_bytes();
                    memcpy(&server.addr.addr4,ipv4.data(),ipv4.size());
                }
                else
                {
                    server.family=AF_INET6;
                    auto ipv6=nameServer.address.to_v6().to_bytes();
                    memcpy(&server.addr.addr6,ipv6.data(),ipv6.size());
                }
                server.next=(i==nameServers.size()-1)?nullptr:&servers[i+1];
            }
            checkStatus(ares_set_servers_ports(d->channel,servers.data()));
        }

        // set socket functions
        ares_set_socket_functions(d->channel,&SocketFunctions,d.get());
    }
    catch (const common::ErrorException&)
    {
        ares_destroy(d->channel);
        throw;
    }

    auto pimpl=d;
    d->timeoutsTimer->setSingleShot(false);
    d->timeoutsTimer->setPeriodUs(CHECK_TIMEOUTS_PERIOD*1000);
    d->timeoutsTimer->start(
        [pimpl{std::move(pimpl)}](common::AsioDeadlineTimer::Status status)
        {
            if (status==common::AsioDeadlineTimer::Status::Timeout)
            {
                ares_process_fd(pimpl->channel,ARES_SOCKET_BAD,ARES_SOCKET_BAD);
            }
        }
    );
}

//---------------------------------------------------------------
void CaResolverTraits::resolveName(
        const lib::string_view& hostName,
        std::function<void (const Error &, std::vector<asio::IpEndpoint>)> callback,
        const common::TaskContextShared& context,
        uint16_t port,
        IpVersion ipVersion
    )
{
    // DCS_DEBUG(dnsresolver,HATN_FORMAT("Resolving {}:{} for {} ...",hostName,port,IpVersionStr(ipVersion)));

    auto allocator=(CaresLib::allocatorFactory()==nullptr)?
                common::pmr::polymorphic_allocator<Query>():CaresLib::allocatorFactory()->objectAllocator<Query>();
    auto query=common::allocateShared<Query>(
            allocator,
            std::weak_ptr<CaResolverTraits_p>(d),
            std::move(callback),
            context,
            ipVersion,
            hostName,
            port
        );
    query->cyclicRefToSelf=query;
    d->processQuery(query);
}

//---------------------------------------------------------------
void CaResolverTraits::resolveService(
        const lib::string_view& name,
        std::function<void (const Error &, std::vector<asio::IpEndpoint>)> callback,
        const common::TaskContextShared& context,
        IpVersion ipVersion
    )
{
    // DCS_DEBUG(dnsresolver,HATN_FORMAT("Resolving SRV {} for {} ...",name,IpVersionStr(ipVersion)));

    auto allocator=(CaresLib::allocatorFactory()==nullptr)?
                common::pmr::polymorphic_allocator<Query>():CaresLib::allocatorFactory()->objectAllocator<Query>();
    auto query=common::allocateShared<Query>(
        allocator,
        std::weak_ptr<CaResolverTraits_p>(d),
        std::move(callback),
        context,
        ipVersion,
        name);
    query->step=QueryStep::RecordSRV;
    query->cyclicRefToSelf=query;
    d->processQuery(query);
}

//---------------------------------------------------------------
void CaResolverTraits::resolveMx(
        const lib::string_view& name,
        std::function<void (const Error &, std::vector<asio::IpEndpoint>)> callback,
        const common::TaskContextShared& context,
        IpVersion ipVersion
    )
{
    // DCS_DEBUG(dnsresolver,HATN_FORMAT("Resolving MX {} for {} ...",name,IpVersionStr(ipVersion)));

    auto allocator=(CaresLib::allocatorFactory()==nullptr)?
                common::pmr::polymorphic_allocator<Query>():CaresLib::allocatorFactory()->objectAllocator<Query>();
    auto query=common::allocateShared<Query>(
        allocator,
        std::weak_ptr<CaResolverTraits_p>(d),
        std::move(callback),
        context,
        ipVersion,
        name);
    query->step=QueryStep::RecordMX;
    query->cyclicRefToSelf=query;
    d->processQuery(query);
}

//---------------------------------------------------------------
void CaResolverTraits::cancel()
{
    d->cancel=true;
    ares_cancel(d->channel);
}

//---------------------------------------------------------------
void CaResolverTraits::cleanup()
{
    d->timeoutsTimer->reset();
    d->timeoutsTimer->stop();
    ares_destroy(d->channel);
}

//---------------------------------------------------------------
void CaResolverTraits::setUseLocalHostsFile(bool enable) noexcept
{
    d->useLocalHostsFile=enable;
}

//---------------------------------------------------------------
bool CaResolverTraits::isUseLocalHostsFile() const noexcept
{
    return d->useLocalHostsFile;
}

//---------------------------------------------------------------
} // namespace asio

HATN_NETWORK_NAMESPACE_END
