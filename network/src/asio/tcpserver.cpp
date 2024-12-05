/*
   Copyright (c) 2019 - current, Evgeny Sidorov (esid1976@gmail.com), All rights reserved
    
    
  */

/****************************************************************************/
/*
    
*/
/** \file network/asio/tcpserver.cpp
  *
  *   TCP server built on boost asio
  *
  */

/****************************************************************************/

#include <memory>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"

#endif

#include <boost/asio/ip/tcp.hpp>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#endif

// #include <hatn/common/types.h>
#include <hatn/common/error.h>
#include <hatn/common/logger.h>
#include <hatn/common/thread.h>

#include <hatn/network/asio/tcpserverconfig.h>
#include <hatn/network/asio/tcpserver.h>

#include <hatn/common/loggermoduleimp.h>
INIT_LOG_MODULE(asiotcpserver,HATN_NETWORK_EXPORT)

namespace hatn {

using namespace common;

namespace network {
namespace asio {

/********************* TcpServerConfig **********************/

HATN_CUID_INIT(TcpServerConfig)

/*********************** TcpServer **************************/

class TcpServer_p
{
    public:

        TcpServer_p(TcpServerConfig* config,boost::asio::io_context& asioContext)
            : config(config),
              acceptor(asioContext),
              closed(false)
        {}

        TcpServerConfig* config;
        boost::asio::ip::tcp::acceptor acceptor;
        bool closed;
};

//---------------------------------------------------------------
TcpServer::TcpServer(
        STR_ID_TYPE id
    ) : TcpServer(nullptr,Thread::currentThread(),std::move(id))
{}

//---------------------------------------------------------------
TcpServer::TcpServer(
        Thread* thread,
        STR_ID_TYPE id
    ) : TcpServer(nullptr,thread,std::move(id))
{}

//---------------------------------------------------------------
TcpServer::TcpServer(
        TcpServerConfig* config,
        STR_ID_TYPE id
    ) : TcpServer(config,Thread::currentThread(),std::move(id))
{}

//---------------------------------------------------------------
TcpServer::TcpServer(
        TcpServerConfig* config,
        Thread* thread,
        STR_ID_TYPE id
    ) : WithIDThread(thread,std::move(id)),
        d(std::make_unique<TcpServer_p>(config,thread->asioContextRef()))
{
}

//---------------------------------------------------------------
TcpServer::~TcpServer()
{
    std::ignore=close();
}

//---------------------------------------------------------------
Error TcpServer::listen(
        const TcpEndpoint &endpoint
    )
{
    Error res;
    auto ep=endpoint.toBoostEndpoint();

    try
    {
        // open acceptor
        d->acceptor.open(ep.protocol());

        // set options on acceptor
        int listenBacklog=boost::asio::ip::tcp::socket::max_connections;
        if (d->config!=nullptr)
        {
            d->config->setOptions(d->acceptor);
            listenBacklog=d->config->listenBacklog();
        }

        // bind acceptor
        d->acceptor.bind(ep);

        // listen for incoming connections
        d->acceptor.listen(listenBacklog);

        // DCS_DEBUG_ID(asiotcpserver,HATN_FORMAT("TCP server is listening on {}:{}",endpoint.address().to_string(),endpoint.port()));
    }
    catch (boost::system::system_error& ec)
    {
        // DCS_WARN_ID(asiotcpserver,HATN_FORMAT("TCP server failed to initialize for {}:{} ({}) {}",endpoint.address().to_string(),endpoint.port(),ec.code().value(),ec.what()));

        // fill error
        res=makeBoostError(ec.code());
    }

    // return result
    return res;
}

//---------------------------------------------------------------
void TcpServer::accept(
        TcpSocket &socket,
        TcpServer::Callback callback
    )
{
    auto sPtr=&socket;
    auto cb=[callback{std::move(callback)},sPtr,this](const boost::system::error_code &ec)
    {
        if (!d->closed)
        {
            if (ec && ec!=boost::system::errc::operation_canceled)
            {
                // DCS_WARN_ID(asiotcpserver,HATN_FORMAT("TCP server failed to accept ({}) {}",ec.value(),ec.message()));
            }
            else
            {
                // DCS_DEBUG_ID(asiotcpserver,HATN_FORMAT("New connection to TCP server from {}:{}",sPtr->socket().remote_endpoint().address().to_string(),sPtr->socket().remote_endpoint().port()));
            }
            callback(makeBoostError(ec));
        }
    };

    d->acceptor.async_accept(socket.socket(),
        guardedAsyncHandler(cb)
    );
}

//---------------------------------------------------------------
Error TcpServer::close()
{
    boost::system::error_code ec;
    if (!d->closed&&d->acceptor.is_open())
    {
        d->closed=true;
        d->acceptor.close(ec);
        if (ec)
        {
            // DCS_WARN_ID(asiotcpserver,HATN_FORMAT("TCP server failed to close ({}) {}",ec.value(),ec.message()));
        }
        else
        {
            // DCS_DEBUG_ID(asiotcpserver,"TCP server closed");
        }
    }
    return makeBoostError(ec);
}

//---------------------------------------------------------------
} // namespace asio
} // namespace network
} // namespace hatn
