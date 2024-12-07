/*
 Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file network/asio/tcpserver.h
  *
  *   TCP server built on boost asio
  *
  */

/****************************************************************************/

#ifndef HATNASIOTCPSERVER_H
#define HATNASIOTCPSERVER_H

#include <hatn/common/error.h>
#include <hatn/common/taskcontext.h>

#include <hatn/network/network.h>
#include <hatn/network/asio/ipendpoint.h>
#include <hatn/network/asio/socket.h>

DECLARE_LOG_MODULE_EXPORT(asiotcpserver,HATN_NETWORK_EXPORT)

HATN_NETWORK_NAMESPACE_BEGIN

namespace asio {

class TcpServerConfig;
class TcpServer_p;

//! TCP server built on boost asio
class HATN_NETWORK_EXPORT TcpServer : public common::TaskSubcontext, public common::WithThread
{
    public:

        using Callback=std::function<void (const common::Error&)>;

        //! Constructor
        TcpServer(
            common::Thread* thread,
            const TcpServerConfig* config=nullptr
        );

        //! Constructor
        TcpServer(
            const TcpServerConfig* config=nullptr
            ) : TcpServer(common::Thread::currentThread(),config)
        {}

        //! Destructor is intentionally not virtual as is is a final class
        ~TcpServer();

        TcpServer(const TcpServer&)=delete;
        TcpServer(TcpServer&&) =default;
        TcpServer& operator=(const TcpServer&)=delete;
        TcpServer& operator=(TcpServer&&) =default;

        //! Listen for incoming connections
        common::Error listen(
            const TcpEndpoint& endpoint
        );

        //! Accept connection
        void accept(
            TcpSocket& socket,
            Callback callback
        );

        //! Close
        common::Error close();

    private:

        std::unique_ptr<TcpServer_p> d;
};

struct makeTcpServerCtxT
{
    auto operator()(const lib::string_view& id, common::Thread* thread,const TcpServerConfig* config=nullptr) const
    {
        return common::makeTaskContext<TcpServer>(
            common::basecontext(id),
            common::subcontexts(
                common::subcontext(thread,config)
                )
            );
    }

    auto operator()(const lib::string_view& id, const TcpServerConfig* config=nullptr) const
    {
        return common::makeTaskContext<TcpServer>(
            common::basecontext(id),
            common::subcontexts(
                common::subcontext(config)
                )
            );
    }
};
constexpr makeTcpServerCtxT makeTcpServerCtx{};
using TcpServerSharedCtx=decltype(makeTcpServerCtx(""));

} // namespace asio
HATN_NETWORK_NAMESPACE_END

HATN_TASK_CONTEXT_DECLARE(HATN_NETWORK_NAMESPACE::asio::TcpServer,HATN_NETWORK_EXPORT)

#endif // HATNASIOTCPSERVER_H
