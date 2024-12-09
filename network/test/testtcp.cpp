#include <boost/test/unit_test.hpp>

#include <hatn/common/locker.h>

#include <hatn/common/logger.h>
#include <hatn/test/multithreadfixture.h>

#include <hatn/logcontext/logcontext.h>
#include <hatn/logcontext/contextlogger.h>
#include <hatn/logcontext/streamlogger.h>
#include <hatn/logcontext/buflogger.h>

#include <hatn/network/asio/tcpserverconfig.h>
#include <hatn/network/asio/tcpserver.h>
#include <hatn/network/asio/tcpstream.h>

// #define HATN_TEST_LOG_CONSOLE

HATN_COMMON_USING
HATN_NETWORK_USING

namespace {

#if 0
static void setLogHandler()
{
    auto handler=[](const ::hatn::common::FmtAllocatedBufferChar &s)
    {
        #ifdef HATN_TEST_LOG_CONSOLE
            std::cout<<::hatn::common::lib::toStringView(s)<<std::endl;
        #else
            std::ignore=s;
        #endif
    };

    ::hatn::common::Logger::setOutputHandler(handler);
    ::hatn::common::Logger::setFatalLogHandler(handler);
    // ::hatn::common::Logger::setDefaultVerbosity(::hatn::common::LoggerVerbosity::DEBUG);
}
#endif
struct Env : public ::hatn::test::MultiThreadFixture
{
    Env()
    {
#if 0
        if (!::hatn::common::Logger::isRunning())
        {
            ::hatn::common::Logger::setFatalTracing(false);

            setLogHandler();
            ::hatn::common::Logger::start();
        }
#endif
    }

    ~Env()
    {
#if 0
        ::hatn::common::Logger::stop();
#endif
    }

    Env(const Env&)=delete;
    Env(Env&&) =delete;
    Env& operator=(const Env&)=delete;
    Env& operator=(Env&&) =delete;
};
}

BOOST_AUTO_TEST_SUITE(Tcp)

BOOST_FIXTURE_TEST_CASE(TcpServerCtor,Env)
{
    auto server1Ctx=asio::makeTcpServerCtx(mainThread().get(),"server1");
    auto& server1=server1Ctx->get<asio::TcpServer>();

    asio::TcpServerConfig config(10);
    auto server2Ctx=asio::makeTcpServerCtx(mainThread().get(),&config,"server1");
    auto& server2=server2Ctx->get<asio::TcpServer>();

    BOOST_CHECK_EQUAL(server1.thread(),server2.thread());

    {
        createThreads(2);
        hatn::common::Thread* thread0=thread(0).get();

        thread0->start();
        auto ec=thread0->execSync(
                [&config,thread0]()
                {
                    auto server3Ctx=asio::makeTcpServerCtx("server3");
                    auto& server3=server3Ctx->get<asio::TcpServer>();
                    auto server4Ctx=asio::makeTcpServerCtx(&config,"server4");
                    auto& server4=server4Ctx->get<asio::TcpServer>();
                    auto server5Ctx=asio::makeTcpServerCtx(thread0,&config,"server5");
                    auto& server5=server5Ctx->get<asio::TcpServer>();

                    BOOST_CHECK_EQUAL(server3.thread(),thread0);
                    BOOST_CHECK_EQUAL(server3.thread(),server4.thread());
                    BOOST_CHECK_EQUAL(server4.thread(),server5.thread());
                }
        );
        BOOST_REQUIRE(!ec);
        thread0->stop();
    }
}

BOOST_FIXTURE_TEST_CASE(TcpServerListen,Env)
{
    uint16_t portNumber1=11511;
    uint16_t portNumber2=11712;
    uint16_t portNumber3=11513;
    uint16_t portNumber4=11714;

    createThreads(2);
    hatn::common::Thread* thread0=thread(0).get();
    hatn::common::Thread* thread1=thread(1).get();

    {
        asio::TcpServerSharedCtx server1,server2,server3,server4;
        asio::TcpServerSharedCtx server5,server6,server7,server8;

        thread0->start();
        thread1->start();

        HATN_CHECK_EXEC_SYNC(
        thread0->execSync(
                        [&server1,&server2,&server3,&server4,portNumber1,portNumber2,portNumber3,portNumber4]()
                        {
                            server1=asio::makeTcpServerCtx("server1");
                            hatn::network::asio::TcpEndpoint ep1("127.0.0.1",portNumber1);
                            auto ec=(*server1)->listen(ep1);
                            BOOST_CHECK(!ec);

                            server2=asio::makeTcpServerCtx("server2");
                            hatn::network::asio::TcpEndpoint ep2(boost::asio::ip::address_v4::any(),portNumber2);
                            ec=(*server2)->listen(ep2);
                            BOOST_CHECK(!ec);

                            server3=asio::makeTcpServerCtx("server3");
                            hatn::network::asio::TcpEndpoint ep3(boost::asio::ip::address_v6::loopback(),portNumber3);
                            ec=(*server3)->listen(ep3);
                            BOOST_CHECK(!ec);

                            server4=asio::makeTcpServerCtx("server4");
                            hatn::network::asio::TcpEndpoint ep4(boost::asio::ip::address_v6::any(),portNumber4);
                            ec=(*server4)->listen(ep4);
                            BOOST_CHECK(!ec);
                        }
                    ));

        HATN_CHECK_EXEC_SYNC(
        thread1->execSync(
                        [&server5,&server6,&server7,&server8,portNumber1,portNumber2,portNumber3,portNumber4]()
                        {
                            server5=asio::makeTcpServerCtx("server5");
                            hatn::network::asio::TcpEndpoint ep1("127.0.0.1",portNumber1);
                            auto ec=(*server5)->listen(ep1);
                            BOOST_CHECK(ec);
                            BOOST_CHECK_EQUAL(ec.errorCondition(),boost::system::errc::address_in_use);

                            server6=asio::makeTcpServerCtx("server6");
                            hatn::network::asio::TcpEndpoint ep2(boost::asio::ip::address_v4::any(),portNumber2);
                            ec=server6->get<asio::TcpServer>().listen(ep2);
                            BOOST_CHECK(ec);
                            BOOST_CHECK_EQUAL(ec.errorCondition(),boost::system::errc::address_in_use);

                            server7=asio::makeTcpServerCtx("server7");
                            hatn::network::asio::TcpEndpoint ep3(boost::asio::ip::address_v6::loopback(),portNumber3);
                            ec=server7->get<asio::TcpServer>().listen(ep3);
                            BOOST_CHECK(ec);
                            BOOST_CHECK_EQUAL(ec.errorCondition(),boost::system::errc::address_in_use);

                            server8=asio::makeTcpServerCtx("server8");
                            hatn::network::asio::TcpEndpoint ep4(boost::asio::ip::address_v6::any(),portNumber4);
                            ec=server8->get<asio::TcpServer>().listen(ep4);
                            BOOST_CHECK(ec);
                            BOOST_CHECK_EQUAL(ec.errorCondition(),boost::system::errc::address_in_use);
                        }
                    ));

        HATN_CHECK_EXEC_SYNC(
        thread0->execSync(
                        [&server1,&server2,&server3,&server4]()
                        {
                            auto ec=server1->get<asio::TcpServer>().close();
                            BOOST_CHECK(!ec);

                            ec=server2->get<asio::TcpServer>().close();
                            BOOST_CHECK(!ec);

                            ec=server3->get<asio::TcpServer>().close();
                            BOOST_CHECK(!ec);

                            ec=server4->get<asio::TcpServer>().close();
                            BOOST_CHECK(!ec);
                        }
                    ));

        thread0->stop();
        thread1->stop();
    }
}

static uint16_t genPortNumber()
{
    srand(static_cast<unsigned int>(time(NULL)));
    uint16_t minPort=9124;
    uint16_t maxPort=31000;
    return (minPort + (rand() % static_cast<uint16_t>(maxPort - minPort + 1)));
}

BOOST_FIXTURE_TEST_CASE(TcpServerAccept,Env)
{
    auto handler=std::make_shared<HATN_LOGCONTEXT_NAMESPACE::StreamLogger>();
    HATN_LOGCONTEXT_NAMESPACE::ContextLogger::init(std::static_pointer_cast<HATN_LOGCONTEXT_NAMESPACE::LoggerHandler>(handler));
    HATN_LOGCONTEXT_NAMESPACE::ContextLogger::instance().setDefaultLogLevel(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Debug);

    {
        createThreads(2);
        hatn::common::Thread* thread0=thread(0).get();
        hatn::common::Thread* thread1=thread(1).get();

        hatn::network::asio::TcpEndpoint serverEndpoint;
        serverEndpoint.setPort(9123);

        hatn::network::asio::TcpEndpoint noServerEndpoint;
        noServerEndpoint.setPort(9124);

        hatn::network::asio::TcpEndpoint client2Endpoint;
        client2Endpoint.setPort(genPortNumber());

        thread0->start();
        thread1->start();

        for (int i=0;i<1;i++)
        {
            bool isIpv6=i==1;
            if (!isIpv6)
            {
                BOOST_TEST_MESSAGE("Begin IPv4");
                serverEndpoint.setAddress(boost::asio::ip::address_v4::loopback());
                client2Endpoint.setAddress(boost::asio::ip::address_v4::loopback());
            }
            else
            {
                BOOST_TEST_MESSAGE("Begin IPv6");
                serverEndpoint.setAddress(boost::asio::ip::address_v6::loopback());
                client2Endpoint.setAddress(boost::asio::ip::address_v6::loopback());
            }
            asio::TcpServerSharedCtx server;

            asio::TcpStreamSharedCtx client1,client2,serverClient1,serverClient2;
            asio::TcpStreamSharedCtx client3;

            std::atomic<int> acceptCount(0);
            std::atomic<int> connectCount(0);
            std::atomic<int> closeCount(0);
            std::atomic<int> failedConnectCount(0);

            auto acceptCb2=[&thread0,&acceptCount](const hatn::common::Error& ec)
            {
                HATN_CTX_SCOPE("acceptCb2")

                HATN_CHECK_TS(!ec);
                HATN_CHECK_EQUAL_TS(thread0,hatn::common::Thread::currentThread());

                ++acceptCount;

                HATN_CTX_DEBUG_RECORDS("accepted",{"accept-count",acceptCount})
            };

            auto acceptCb1=[&thread0,&server,&serverClient2,&acceptCb2,&acceptCount,isIpv6](const hatn::common::Error& ec)
            {
                HATN_CTX_SCOPE("acceptCb1")

                HATN_REQUIRE_TS(!ec);

                ++acceptCount;
                HATN_CTX_DEBUG_RECORDS("accepted",{"accept-count",acceptCount})

                serverClient2=asio::makeTcpStreamCtx(isIpv6?"serverClient2-6":"serverClient2-4");
                (*server)->accept((*serverClient2)->socket(),acceptCb2);

                HATN_CHECK_EQUAL_TS(thread0,hatn::common::Thread::currentThread());
                HATN_CHECK_EQUAL_TS(thread0,(*serverClient2)->thread());
            };

            BOOST_TEST_MESSAGE("Run server");

            auto ec=thread0->execSync(
                    [&server,&serverEndpoint,&serverClient1,&acceptCb1,isIpv6]()
                    {
                        server=asio::makeTcpServerCtx(isIpv6?"server-6":"server-4");
                        server->beginTaskContext();
                        HATN_CTX_SCOPE("server-listen-accept")
                        auto ec=(*server)->listen(serverEndpoint);
                        HATN_REQUIRE_TS(!ec);

                        serverClient1=asio::makeTcpStreamCtx(isIpv6?"serverClient1-6":"serverClient1-4");                        
                        (*server)->accept((*serverClient1)->socket(),acceptCb1);
                    }
            );
            BOOST_REQUIRE(!ec);

            auto client1ConnectCb=[&thread1,&connectCount](const hatn::common::Error& ec)
            {
                HATN_CTX_SCOPE("client1ConnectCb")

                HATN_CHECK_TS(!ec);
                HATN_CHECK_EQUAL_TS(thread1,hatn::common::Thread::currentThread());
                ++connectCount;

                HATN_CTX_DEBUG_RECORDS("client1 connected",{"connect-count",connectCount})
            };

            auto client2ConnectCb=[&thread1,&connectCount](const hatn::common::Error& ec)
            {
                HATN_CTX_SCOPE("client2ConnectCb")

                HATN_CHECK_TS(!ec);
                HATN_CHECK_EQUAL_TS(thread1,hatn::common::Thread::currentThread());
                ++connectCount;

                HATN_CTX_DEBUG_RECORDS("client2 connected",{"connect-count",connectCount})
            };

            auto client3ConnectCb=[&thread1,&failedConnectCount](const hatn::common::Error& ec)
            {
                HATN_CTX_SCOPE("client3ConnectCb")

                HATN_CHECK_TS(ec);
                HATN_CHECK_EQUAL_TS(thread1,hatn::common::Thread::currentThread());
                ++failedConnectCount;

                HATN_CTX_DEBUG_RECORDS("client3 failed to connect",{"failed-connect-count",failedConnectCount})
                HATN_CTX_ERROR(ec,"expected failed to connect by client3")
            };

            BOOST_TEST_MESSAGE("Connect clients");

            thread1->execAsync(
                [&client1,&client2,&client3,&serverEndpoint,&noServerEndpoint,
                 &client2Endpoint,&client1ConnectCb,&client2ConnectCb,&client3ConnectCb,isIpv6]()
                {
                    {
                        client1=asio::makeTcpStreamCtx(isIpv6?"client1-6":"client1-4");
                        client1->beginTaskContext();
                        HATN_CTX_SCOPE("client1-connect")
                        (*client1)->setRemoteEndpoint(serverEndpoint);
                        (*client1)->prepare(client1ConnectCb);
                    }

                    {
                        client2=asio::makeTcpStreamCtx(isIpv6?"client2-6":"client2-4");
                        client2->beginTaskContext();
                        HATN_CTX_SCOPE("client2-context")
                        HATN_CTX_SCOPE_PUSH("client-id",2)
                        client2->enterLoop();
                        {
                            HATN_CTX_SCOPE("client2-connect")
                            (*client2)->setRemoteEndpoint(serverEndpoint);
                            (*client2)->setLocalEndpoint(client2Endpoint);
                            (*client2)->prepare(client2ConnectCb);
                        }
                    }

                    {
                        // client3=asio::makeTcpStreamCtx(isIpv6?"client3-6":"client3-4");
                        // client3->beginTaskContext();
                        // HATN_CTX_SCOPE("client3-context")
                        // HATN_CTX_SCOPE_PUSH("client-id",3)
                        // client3->enterLoop();
                        // {
                        //     HATN_CTX_SCOPE("client3-connect")
                        //     (*client3)->setRemoteEndpoint(noServerEndpoint);
                        //     (*client3)->prepare(client3ConnectCb);
                        // }
                    }
                }
            );

            BOOST_TEST_MESSAGE("Exec for 1 second");
            exec(1);

            auto closeCb=[&closeCount](const hatn::common::Error& ec)
            {
                HATN_CTX_SCOPE("closeCb")

                HATN_CHECK_TS(!ec);
                ++closeCount;

                HATN_CTX_DEBUG_RECORDS("closed",{"close-count",closeCount})
            };

            BOOST_TEST_MESSAGE("Close clients");

            ec=thread1->execSync(
                    [&client1,&client2,&closeCb]()
                    {
                        {
                            client1->beginTaskContext();
                            HATN_CTX_SCOPE("client1-close")
                            (*client1)->close(closeCb);
                        }

                        {
                            client2->beginTaskContext();
                            HATN_CTX_SCOPE("client2-close")
                            (*client2)->close(closeCb);
                        }
                    }
            );
            BOOST_REQUIRE(!ec);

            BOOST_TEST_MESSAGE("Waiting for 1 second before closing server");
            exec(1);

            BOOST_TEST_MESSAGE("Close server");

            ec=thread0->execSync(
                    [&serverClient1,&serverClient2,&closeCb,&server]()
                    {
                        {
                            serverClient1->beginTaskContext();
                            HATN_CTX_SCOPE("serverclient1-close")
                            (*serverClient1)->close(closeCb);
                        }

                        if (serverClient2)
                        {
                            serverClient2->beginTaskContext();
                            HATN_CTX_SCOPE("serverclient2-close")
                            (*serverClient2)->close(closeCb);
                        }

                        {
                            server->beginTaskContext();
                            HATN_CTX_SCOPE("server-close")
                            auto ec=(*server)->close();
                            HATN_CHECK_TS(!ec);
                        }
                    }
            );
            BOOST_REQUIRE(!ec);

            BOOST_TEST_MESSAGE("Waiting for 1 second before check");
            exec(1);

            {
                HATN_CHECK_EQUAL_TS(acceptCount,2);
                HATN_CHECK_EQUAL_TS(connectCount,2);
                HATN_CHECK_EQUAL_TS(closeCount,4);
                //! @todo Uncomment after fixing client3
                // HATN_CHECK_EQUAL_TS(failedConnectCount,1);
            }

            if (!isIpv6)
            {
                BOOST_TEST_MESSAGE("End IPv4");
            }
            else
            {
                BOOST_TEST_MESSAGE("End IPv6");
            }            
        }

        thread0->stop();
        thread1->stop();
    }
}

#if 0

constexpr static const size_t dataSize=0x20000;

template <typename T>
static void readNext(const hatn::common::Error& ec,
                     size_t size,
                     T* client,
                     size_t& recvSize,
                     std::array<char,dataSize>& recvBuf,
                     const std::function<void()>& checkDone,
                     hatn::common::MutexLock& mutex
                     )
{
    if (ec)
    {
        checkDone();
        return;
    }

    recvSize+=size;
    if (recvSize>=dataSize)
    {
        // G_DEBUG("Read done "<<client->id().c_str()<<" recvSize="<<recvSize);

        checkDone();
        return;
    }
    size_t space=0x1000;
    if ((recvSize+space)>dataSize)
    {
        space=dataSize-recvSize;
    }
    // G_DEBUG("Read "<<client->id().c_str()<<" space="<<space<<" recvSize="<<recvSize);

    client->read(recvBuf.data()+recvSize,space,
                       [client,&recvSize,&recvBuf,checkDone,&mutex](const hatn::common::Error& ec1, size_t rxSize)
                       {
                            // G_DEBUG("Read cb recvSize="<<rxSize<<" ec1="<<ec1.value());

                            readNext(
                                        ec1,rxSize,client,recvSize,recvBuf,checkDone,mutex
                                    );
                       }
                );
}

BOOST_FIXTURE_TEST_CASE(TcpSendRecv,Env)
{
    hatn::common::MutexLock mutex;

    createThreads(2);
    hatn::common::Thread* thread0=thread(0).get();
    hatn::common::Thread* thread1=thread(1).get();

    hatn::network::asio::TcpEndpoint serverEndpoint;
    serverEndpoint.setPort(19123);

    thread0->start();
    thread1->start();

    for (int i=0;i<2;i++)
    {
        std::array<char,dataSize> clientSendBuf,serverSendBuf;
        std::array<char,dataSize> clientRecvBuf,serverRecvBuf;

        size_t clientRxSize=0;
        size_t serverRxSize=0;
        randBuf(clientSendBuf);
        randBuf(serverSendBuf);

        hatn::common::ByteArrayShared clientSendBuf1(
                        new hatn::common::ByteArrayManaged(clientSendBuf.data(),dataSize/4)
                    );
        hatn::common::ByteArrayShared clientSendBuf2(
                        new hatn::common::ByteArrayManaged(clientSendBuf.data(),clientSendBuf.size())
                    );

        bool isIpv6=i==1;
        if (!isIpv6)
        {
            BOOST_TEST_MESSAGE("Begin IPv4");
            serverEndpoint.setAddress(boost::asio::ip::address_v4::loopback());
        }
        else
        {
            BOOST_TEST_MESSAGE("Begin IPv6");
            serverEndpoint.setAddress(boost::asio::ip::address_v6::loopback());
        }

        std::atomic<int> rxDoneCount(0);
        auto checkDone=[&rxDoneCount,this]()
        {
            if (++rxDoneCount==2)
            {
                quit();
            }
        };

        asio::TcpServerSharedCtx server;
        asio::TcpStreamSharedCtx client,serverClient;

        auto acceptCb=[&mutex,&serverClient,&serverSendBuf,&serverRecvBuf,&checkDone,&serverRxSize](const hatn::common::Error& ec)
        {
            {
                hatn::common::MutexScopedLock l(mutex);
                BOOST_REQUIRE(!ec);
            }
            (*serverClient)->write(serverSendBuf.data(),serverSendBuf.size(),
                                [&mutex](const hatn::common::Error& ec1, size_t size)
                                {
                                    hatn::common::MutexScopedLock l(mutex);
                                    BOOST_REQUIRE(!ec1);
                                    BOOST_CHECK_EQUAL(size,dataSize);
                                }
                        );
            readNext(ec,0,&serverClient->get(),serverRxSize,serverRecvBuf,checkDone,mutex);
        };

        BOOST_TEST_MESSAGE("Run server");

        auto ec=thread0->execSync(
                        [&server,&mutex,&serverEndpoint,&serverClient,&acceptCb,isIpv6]()
                        {
                            server=asio::makeTcpServerCtx(isIpv6?"server-6":"server-4");
                            auto ec=(*server)->listen(serverEndpoint);
                            {
                                hatn::common::MutexScopedLock l(mutex);
                                BOOST_REQUIRE(!ec);
                            }

                            serverClient=asio::makeTcpStreamCtx(isIpv6?"serverClient-6":"serverClient-4");
                            (*server)->accept((*serverClient)->socket(),acceptCb);
                        }
        );
        BOOST_REQUIRE(!ec);

        auto connectCb=[&thread1,&client,&checkDone,&mutex,
                        clientSendBuf1,clientSendBuf2,
                        &clientRecvBuf,&clientRxSize](const hatn::common::Error& ec)
        {
            {
                hatn::common::MutexScopedLock l(mutex);
                BOOST_REQUIRE(!ec);
                BOOST_CHECK_EQUAL(thread1,hatn::common::Thread::currentThread());
            }

            hatn::common::SpanBuffers bufs;
            bufs.push_back(
                            hatn::common::SpanBuffer(clientSendBuf1)
                        );
            size_t offset=dataSize/4;
            bufs.push_back(
                            hatn::common::SpanBuffer(clientSendBuf2,offset,dataSize/4)
                        );
            offset+=dataSize/4;
            bufs.push_back(
                            hatn::common::SpanBuffer(clientSendBuf2,offset,dataSize/4)
                        );
            offset+=dataSize/4;

            (*client)->write(std::move(bufs),
                                [&mutex,client,offset,clientSendBuf2](const hatn::common::Error& ec1, size_t size, const hatn::common::SpanBuffers&)
                                {
                                    {
                                        hatn::common::MutexScopedLock l(mutex);
                                        BOOST_REQUIRE(!ec1);
                                        BOOST_CHECK_EQUAL(size,dataSize-dataSize/4);
                                    }
                                    auto&& cb1=[&mutex](const hatn::common::Error& ec2, size_t size2, const hatn::common::SpanBuffer&)
                                    {
                                        hatn::common::MutexScopedLock l(mutex);
                                        BOOST_REQUIRE(!ec2);
                                        BOOST_CHECK_EQUAL(size2,dataSize/4);
                                    };

                                    hatn::common::SpanBuffer buf(clientSendBuf2,offset,dataSize/4);
                                    (*client)->write(std::move(buf),cb1);
                                }
                        );
            readNext(ec,0,&client->get(),clientRxSize,clientRecvBuf,checkDone,mutex);
        };

        BOOST_TEST_MESSAGE("Connect clients");

        thread1->execAsync(
                        [&client,&serverEndpoint,&connectCb,isIpv6]()
                        {
                            client=asio::makeTcpStreamCtx(isIpv6?"client-6":"client-4");
                            (*client)->setRemoteEndpoint(serverEndpoint);
                            (*client)->prepare(connectCb);
                        }
        );

        BOOST_TEST_MESSAGE("Exec 10 seconds...");
        exec(10);

        auto closeCb=[&mutex](const hatn::common::Error& ec)
        {
            {
                hatn::common::MutexScopedLock l(mutex);
                BOOST_CHECK(!ec);
            }
        };

        BOOST_TEST_MESSAGE("Close clients");

        ec=thread1->execSync(
                        [&client,&closeCb]()
                        {
                            (*client)->close(closeCb);
                        }
        );
        BOOST_REQUIRE(!ec);

        BOOST_TEST_MESSAGE("Close server");

        ec=thread0->execSync(
                        [&serverClient,&mutex,&closeCb,&server]()
                        {
                            (*serverClient)->close(closeCb);
                            auto ec=(*server)->close();
                            {
                                hatn::common::MutexScopedLock l(mutex);
                                BOOST_CHECK(!ec);
                            }
                        }
        );
        BOOST_REQUIRE(!ec);

        BOOST_TEST_MESSAGE("Exec 1 second...");
        exec(1);

        if (!isIpv6)
        {
            BOOST_TEST_MESSAGE("End IPv4");
        }
        else
        {
            BOOST_TEST_MESSAGE("End IPv6");
        }

        {
            hatn::common::MutexScopedLock l(mutex);
            BOOST_REQUIRE_EQUAL(serverRxSize,dataSize);
            BOOST_REQUIRE_EQUAL(clientRxSize,dataSize);
            BOOST_CHECK(clientSendBuf==serverRecvBuf);
            BOOST_CHECK(clientRecvBuf==serverSendBuf);
        }

    }

    thread0->stop();
    thread1->stop();
}
#endif
BOOST_AUTO_TEST_SUITE_END()
