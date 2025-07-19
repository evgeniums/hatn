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
#include <hatn/common/thread.h>
#include <hatn/common/weakptr.h>

#include <hatn/logcontext/contextlogger.h>

#include <hatn/network/asio/tcpserverconfig.h>
#include <hatn/network/asio/tcpserver.h>

HATN_TASK_CONTEXT_DEFINE(HATN_NETWORK_NAMESPACE::asio::TcpServerConfig,TcpServerConfig)
HATN_TASK_CONTEXT_DEFINE(HATN_NETWORK_NAMESPACE::asio::TcpServer,TcpServer)

HATN_NETWORK_NAMESPACE_BEGIN
HATN_COMMON_USING

namespace asio {

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
    std::ignore=close(true);
}

//---------------------------------------------------------------
Error TcpServer::listen(
        const TcpEndpoint &endpoint
    )
{
    HATN_CTX_SCOPE("tcpserverlisten")

    Error res;
    auto ep=endpoint.toBoostEndpoint();

    try
    {
        // open acceptor
        d->acceptor.open(ep.protocol());

        // set options on acceptor
        int listenBacklog=boost::asio::ip::tcp::socket::max_listen_connections;
        if (d->config!=nullptr)
        {
            d->config->fillAcceptorOptions(d->acceptor);
            listenBacklog=d->config->listenBacklog();
        }

        d->acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));

        // bind acceptor
        d->acceptor.bind(ep);

        // listen for incoming connections
        d->acceptor.listen(listenBacklog);

        HATN_CTX_INFO_RECORDS_M("TCP server is listening",HatnAsioLog,{"local_ip",endpoint.address().to_string()},{"local_port",endpoint.port()})
    }
    catch (boost::system::system_error& ec)
    {        
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
    auto serverMainCtx=sharedMainCtx();
    auto serverWptr=toWeakPtr(serverMainCtx);

    auto cb=[callback{std::move(callback)},
             serverWptr{std::move(serverWptr)},
             &socket,
             this](const boost::system::error_code &ec)
    {
        auto serverMainCtx=serverWptr.lock();
        if (!serverMainCtx)
        {
            callback(commonError(CommonError::ABORTED));
            return;
        }
        mainCtx().onAsyncHandlerEnter();

        {
            HATN_CTX_SCOPE("tcpserveraccept")

            if (d->closed)
            {
                callback(commonError(CommonError::ABORTED));
                mainCtx().onAsyncHandlerExit();
                return;
            }
            if (!ec)
            {
                HATN_CTX_DEBUG_RECORDS_M(DoneDebugVerbosity,"new connection to TCP server",HatnAsioLog,{"remote_ip",socket.socket().remote_endpoint().address().to_string()},{"remote_port",socket.socket().remote_endpoint().port()})

                //! @todo Configure socket options
                callback(Error{});
            }
            else
            {
                callback(makeBoostError(ec));
            }
        }

        mainCtx().onAsyncHandlerExit();
    };

    d->acceptor.async_accept(socket.socket(),cb);
}

//---------------------------------------------------------------
Error TcpServer::close(bool destroying)
{
    auto doClose=[destroying,this]()
    {
        boost::system::error_code ec;
        if (!d->closed&&d->acceptor.is_open())
        {
            d->closed=true;
            d->acceptor.close(ec);
            if (!destroying && !ec)
            {
                HATN_CTX_DEBUG(DoneDebugVerbosity,"TCP server closed",HatnAsioLog)
            }
        }
        return makeBoostError(ec);
    };

    if (destroying)
    {
        return doClose();
    }

    HATN_CTX_SCOPE("tcpserverclose")
    return doClose();
}

//---------------------------------------------------------------
void TcpServer::setConfig(const HATN_NETWORK_NAMESPACE::asio::TcpServerConfig* config)
{
    d->config=config;
}

//---------------------------------------------------------------
} // namespace asio

HATN_NETWORK_NAMESPACE_END
