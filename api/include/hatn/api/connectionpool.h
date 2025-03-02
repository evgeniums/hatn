/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/connectionpool.h
  *
  */

/****************************************************************************/

#ifndef HATNAPISCONNECTIONPOOL_H
#define HATNAPISCONNECTIONPOOL_H

#include <functional>

#include <hatn/common/sharedptr.h>
#include <hatn/common/pmr/allocatorfactory.h>
#include <hatn/common/flatmap.h>
#include <hatn/common/result.h>

#include <hatn/dataunit/wirebufsolid.h>

#include <hatn/api/api.h>
#include <hatn/api/apierror.h>
#include <hatn/api/router.h>
#include <hatn/api/priority.h>
#include <hatn/api/apiconstants.h>
#include <hatn/api/protocol.h>

HATN_API_NAMESPACE_BEGIN

template <typename RouterTraits>
class ConnectionPool
{
    using Connection=typename RouterTraits::Connection;
    using RouterConnectionCtx=typename RouterTraits::ConnectionContext;

    struct ConnectionContext
    {
        enum class State : uint8_t
        {
            Disconnected,
            Ready,
            Busy,
            Error
        };

        auto& connection()
        {
            return ctx->template get<Connection>();
        }

        common::SharedPtr<RouterConnectionCtx> ctx;
        State state=State::Disconnected;

        protocol::Header header;
    };

    using Connections=common::pmr::map<common::TaskContextId,common::SharedPtr<ConnectionContext>,std::less<>>;

    using ConnectionCtxShared=common::SharedPtr<ConnectionContext>;

    public:        

        ConnectionPool(
                common::SharedPtr<Router<RouterTraits>> router,
                const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
            ) : m_router(router),
                m_allocatorFactory(factory),
                m_maxConnectionsPerPriority(DefaultMaxPoolPriorityConnections),
                m_closed(false),
                m_singleConnection(true),
                m_maxMessageSize(protocol::DEFAULT_MAX_MESSAGE_SIZE),
                m_defaultConnection(common::allocateShared(factory->objectAllocator<ConnectionContext>()))
        {
            handlePriorities(
                [this](Priority priority)
                {
                    resetPriority(priority);
                },
                [this](size_t count)
                {
                    m_connections.reserve(count);
                }
            );
        }

        void setMaxConnectionsPerPriority(size_t maxConnectionsPerPriority) noexcept
        {
            m_maxConnectionsPerPriority=maxConnectionsPerPriority;
        }

        void setMaxMessageSize(uint32_t val) noexcept
        {
            m_maxMessageSize=val;
        }

        uint32_t maxMessageSize() const noexcept
        {
            return m_maxMessageSize;
        }

        void setSingleConnectionMode(bool enable) noexcept
        {
            m_singleConnection=enable;
        }

        bool isSingleCOnnectionMode() const noexcept
        {
            return m_singleConnection;
        }

        bool canSend(Priority priority) const
        {
            if (m_singleConnection)
            {
                return m_defaultConnection->state!=ConnectionContext::State::Busy;
            }

            //! @todo Check states and fallback mode
            auto it=m_connections.find(priority);
            Assert(it!=m_connections.end(),"Invalid priority");
            return it->second.size()<m_maxConnectionsPerPriority;
        }

        template <typename ContextT>
        void send(
                common::SharedPtr<ContextT> ctx,
                Priority priority,
                common::SpanBuffers buffers,
                std::function<void (const Error& ec, ConnectionCtxShared connection)> cb
            )
        {
            // check if pool is closed
            if (m_closed)
            {
                cb(commonError(CommonError::ABORTED),nullptr);
                return;
            }

            // process single connection mode
            if (m_singleConnection)
            {
                auto& connectionCtx=m_defaultConnection;
                ConnectionCtxShared connection;

                if (connectionCtx.state==ConnectionContext::State::Busy)
                {
                    cb(apiError(ApiLibError::CONNECTION_BUSY));
                    return;
                }

                auto connectCb=[this,ctx,cb,buffers(std::move(buffers)),connection{std::move(connection)}](const Error& ec)
                {
                    if (ec)
                    {
                        cb(ec);
                        return;
                    }
                    sendToConnection(std::move(ctx),std::move(buffers),std::move(cb),std::move(connection));
                };

                if (connectionCtx.state==ConnectionContext::State::Disconnected)
                {
                    connect(std::move(ctx),std::move(connectCb));
                }
                else if (connectionCtx.state==ConnectionContext::State::Error)
                {
                    auto closeCb=[ctx,this,connectCb{std::move(connectCb)}](const Error&)
                    {
                        connect(std::move(ctx),std::move(connectCb));
                    };
                    closeDefaultConnection(ctx,closeCb);
                }

                return;
            }


            cb(commonError(CommonError::NOT_IMPLEMENTED),nullptr);
            return;

            //! @todo implement pool operations
            auto it=m_connections.find(priority);

            //! @todo find not busy connection
            //! check if connection is closed or failed, destroy closed and look for next
            //! create new connection if ready connection not found

            //! destroy connection in case of error
            //! try to reconnect/create new connection/switch to fallback in case of error when sending header
            //! try to send via default connection if new connection can not be established
        }

        template <typename ContextT>
        void recv(
                common::SharedPtr<ContextT> ctx,
                const ConnectionCtxShared& connectionCtx,
                du::WireBufSolidShared& buf,
                std::function<void (const Error& ec)> cb
            )
        {
            // check if pool is closed
            if (m_closed)
            {
                cb(commonError(CommonError::ABORTED));
                return;
            }

            if (connectionCtx.state!=ConnectionContext::State::Ready)
            {
                cb(apiError(ApiLibError::CONNECTION_NOT_READY_RECV));
                return;
            }

            connectionCtx->state=ConnectionContext::State::Busy;

            auto recvMessageCb=[cb,connectionCtx,this](auto, const Error& ec, size_t)
            {
                connectionCtx->ctx->resetParentCtx();

                if (ec)
                {
                    //! @todo destroy failed pool connection
                    connectionCtx->state=ConnectionContext::State::Error;
                    cb(ec);
                    return;
                }

                connectionCtx->state=ConnectionContext::State::Ready;
                cb(Error{});
            };
            auto recvHeaderCb=[cb,connectionCtx,&buf,recvMessageCb{std::move(recvMessageCb)},this](auto ctx, const Error& ec, size_t)
            {
                connectionCtx->ctx->resetParentCtx();

                if (ec)
                {
                    //! @todo destroy failed pool connection
                    connectionCtx->state=ConnectionContext::State::Error;
                    cb(ec);
                    return;
                }

                auto messageSize=connectionCtx->header.messageSize();
                if(messageSize==0)
                {
                    cb(Error{});
                    return;
                }

                if (messageSize>m_maxMessageSize)
                {
                    //! @todo destroy failed pool connection
                    connectionCtx->state=ConnectionContext::State::Error;
                    cb(apiError(ApiLibError::TOO_BIG_RX_MESSAGE));
                    return;
                }

                buf.mainContainer()->resize(messageSize);
                connectionCtx->ctx->resetParentCtx(ctx);
                connectionCtx->connection().recv(
                    std::move(ctx),
                    buf.mainContainer()->data(),
                    buf.mainContainer()->size(),
                    std::move(recvMessageCb)
                );
            };
            connectionCtx->ctx->resetParentCtx(ctx);
            connectionCtx->connection().recv(
                std::move(ctx),
                connectionCtx->header.data(),
                connectionCtx->header.size(),
                std::move(recvHeaderCb)
            );
        }

        template <typename ContextT>
        void connect(
            common::SharedPtr<ContextT> ctx,
            std::function<void (const Error& ec)> cb
        )
        {
            if (m_defaultConnection->state!=ConnectionContext::State::Disconnected)
            {
                cb(apiError(ApiLibError::CONNECTION_BUSY));
                return;
            }

            m_defaultConnection->state=ConnectionContext::State::Busy;
            auto makeCb=[this,cb{std::move(cb)}](const common::Error& ec, RouterConnectionCtx connectionCtx)
            {
                if (ec)
                {
                    m_defaultConnection->state=ConnectionContext::State::Disconnected;
                    cb(ec);
                    return;
                }

                m_defaultConnection->ctx=std::move(connectionCtx);
                m_defaultConnection->state=ConnectionContext::State::Ready;
                cb(Error{});
            };
            makeConnection(ctx,makeCb);
        }

        template <typename ContextT>
        void close(
            common::SharedPtr<ContextT> ctx,
            std::function<void (const Error&)> callback
            )
        {
            m_closed=true;

            if (m_defaultConnection)
            {
                auto cb=[ctx,callback{std::move(callback)},this](const Error&)
                {
                    closeNextPriority(std::move(ctx),std::move(callback));
                };
                closeDefaultConnection(ctx,std::move(cb));
            }
            else
            {
                closeNextPriority(std::move(ctx),std::move(callback));
            }
        }

    private:

        template <typename ContextT>
        void closeDefaultConnection(
            common::SharedPtr<ContextT> ctx,
            std::function<void (const Error&)> cb
            )
        {
            m_defaultConnection->ctx->resetParentCtx(ctx);
            auto cb1=[ctx,cb{std::move(cb)},this](const Error& ec)
            {
                m_defaultConnection->ctx->resetParentCtx();
                m_defaultConnection->state=ConnectionContext::State::Disconnected;
                m_defaultConnection->ctx.reset();
                cb(ec);
            };
            m_defaultConnection->connection().close(std::move(cb1));
        }

        template <typename ContextT>
        void closeNextPriority(common::SharedPtr<ContextT> ctx, std::function<void (const Error&)> cb)
        {
            auto it=m_connections.begin();
            if (it==it->m_connections.end())
            {
                cb(Error{});
                return;
            }

            closeNextConnection(
                std::move(ctx),
                it->second,
                [ctx{std::move(ctx)},&it,cb{std::move(cb)},this]()
                {
                    m_connections.erase(it);
                    closeNextPriority(std::move(ctx),std::move(cb));
                }
            );
        }

        template <typename ContextT>
        void closeNextConnection(common::SharedPtr<ContextT> ctx, Connections& connections, std::function<void (const Error&)> cb)
        {
            auto it=connections.begin();
            if (it==connections.end())
            {
                cb(Error{});
                return;
            }
            auto& connectionCtx=it->second;
            connectionCtx->ctx->resetParentCtx(ctx);
            connectionCtx->connection().close(
                [connectionCtx{std::move(connectionCtx)},ctx{std::move(ctx)},&it,&connections,cb{std::move(cb)},this](const Error&)
                {
                    connectionCtx->ctx->resetParentCtx();
                    connections.erase(it);
                    closeNextConnection(std::move(ctx),connections,std::move(cb));
                }
            );
        }

        void resetPriority(Priority priority)
        {
            m_connections.emplace(priority,m_allocatorFactory->objectMemoryResource());
        }

        template <typename ContextT>
        void makeConnection(
            common::SharedPtr<ContextT> ctx,
            std::function<void (const Error& ec, common::SharedPtr<RouterConnectionCtx> connectionCtx)> cb
            )
        {
            auto makeCb=[this,ctx,cb{std::move(cb)}](const common::Error& ec, common::SharedPtr<RouterConnectionCtx> connectionCtx)
            {
                if (ec)
                {
                    cb(ec,{});
                    return;
                }

                connectionCtx->resetParentCtx(std::move(ctx));
                auto& connection=connectionCtx->template get<Connection>();
                auto connectCb=[connectionCtx{std::move(connectionCtx)},ctx{std::move(ctx)},cb{std::move(cb)}](const common::Error& ec)
                {
                    connectionCtx->resetParentCtx();
                    cb(ec);
                };
                connection.connect(std::move(connectCb));
            };
            m_router->makeConnection(ctx,makeCb);
        }

        template <typename ContextT>
        void sendToConnection(
            common::SharedPtr<ContextT> ctx,
            ConnectionCtxShared connectionCtx,
            common::SpanBuffers buffers,
            std::function<void (const Error& ec, ConnectionCtxShared connection)> cb
            )
        {
            connectionCtx->state==ConnectionContext::State::Busy;

            // set message size
            auto messageSize=common::SpanBufferTraits::size(buffers);
            if (messageSize>m_maxMessageSize)
            {
                cb(apiError(ApiLibError::TOO_BIG_TX_MESSAGE),nullptr);
                return;
            }
            connectionCtx->header.setMessageSize(messageSize);

            // send
            auto sendMessageCb=[connectionCtx{std::move(connectionCtx)},cb{std::move(cb)}](auto, const Error& ec, size_t)
            {
                connectionCtx->ctx->resetParentCtx();
                if (ec)
                {
                    //! @todo handle failed pool connection
                    connectionCtx->state==ConnectionContext::State::Error;
                    cb(ec,nullptr);
                    return;
                }
                connectionCtx->state==ConnectionContext::State::Ready;
                cb(Error{},std::move(connectionCtx));
            };
            auto sendHeaderCb=[sendMessageCb{std::move(sendMessageCb)},connectionCtx{std::move(connectionCtx)},cb{std::move(cb)},buffers{std::move(buffers)}](
                                        auto ctx, const Error& ec, size_t
                                    )
            {
                connectionCtx->ctx->resetParentCtx();
                if (ec)
                {
                    //! @todo try to reconnect

                    //! @todo handle failed pool connection
                    connectionCtx->state==ConnectionContext::State::Error;
                    cb(ec,nullptr);
                    return;
                }

                connectionCtx->ctx->resetParentCtx(ctx);
                connectionCtx->connection().send(std::move(ctx),std::move(buffers),std::move(sendMessageCb));
            };
            connectionCtx->ctx->resetParentCtx(ctx);
            connectionCtx->connection().send(std::move(ctx),connectionCtx->header.data(),connectionCtx->header.size(),std::move(sendHeaderCb));
        }

        common::SharedPtr<Router<RouterTraits>> m_router;
        const common::pmr::AllocatorFactory* m_allocatorFactory;

        common::FlatMap<Priority,Connections> m_connections;

        size_t m_maxConnectionsPerPriority;
        bool m_closed;

        bool m_singleConnection;
        uint32_t m_maxMessageSize;

        ConnectionCtxShared m_defaultConnection;
};

HATN_API_NAMESPACE_END

#endif // HATNAPISCONNECTIONPOOL_H
