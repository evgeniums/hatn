/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file network/asio/tcpserver.cpp
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

#include <hatn/common/error.h>
#include <hatn/common/logger.h>
#include <hatn/common/thread.h>
#include <hatn/common/weakptr.h>

#include <hatn/network/asio/tcpserverconfig.h>
#include <hatn/network/asio/tcpserver.h>

#include <hatn/common/loggermoduleimp.h>
INIT_LOG_MODULE(asiotcpserver,HATN_NETWORK_EXPORT)

HATN_TASK_CONTEXT_DEFINE(HATN_NETWORK_NAMESPACE::asio::TcpServer,TcpServer)

HATN_NETWORK_NAMESPACE_BEGIN
HATN_COMMON_USING

namespace asio {

//! @todo Refactor TcpServer logging

/*********************** TcpServer **************************/

class TcpServer_p
{
    public:

        TcpServer_p(const TcpServerConfig* config,
                    boost::asio::io_context& asioContext)
            : config(config),
              acceptor(asioContext),
              closed(false)
        {}

        const TcpServerConfig* config;
        boost::asio::ip::tcp::acceptor acceptor;
        bool closed;
};

//---------------------------------------------------------------
TcpServer::TcpServer(
        common::Thread* thread,
        const TcpServerConfig* config
    ) : WithThread(thread),
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
            d->config->fillAcceptorOptions(d->acceptor);
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
        TcpSocket& socket,
        TcpServer::Callback callback
    )
{
    auto serverMainCtx=mainCtx().sharedFromThis();
    auto serverWptr=toWeakPtr(serverMainCtx);

    auto cb=[callback{std::move(callback)},
             serverWptr{std::move(serverWptr)},
             this](const boost::system::error_code &ec)
    {
        auto serverMainCtx=serverWptr.lock();
        if (!serverMainCtx)
        {
            //! @todo callback with cancelled error
            return;
        }

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

    d->acceptor.async_accept(socket.socket(),cb);
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

HATN_NETWORK_NAMESPACE_END
