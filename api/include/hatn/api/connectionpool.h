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
#include <hatn/common/allocatoronstack.h>
#include <hatn/common/format.h>

#include <hatn/dataunit/wirebufsolid.h>

#include <hatn/api/api.h>
#include <hatn/api/apiliberror.h>
#include <hatn/api/router.h>
#include <hatn/api/priority.h>
#include <hatn/api/apiconstants.h>
#include <hatn/api/protocol.h>

HATN_API_NAMESPACE_BEGIN

template <typename RouterT>
class ConnectionPool
{
    //! @todo Move definitions to ipp

    using RouterConnectionCtx=typename RouterT::ConnectionContext;
    using Connection=typename RouterT::Connection;

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
        bool defaultConnection=true;
        Priority priority=Priority::Normal;

        size_t sendTriesCount=0;
    };

    using Connections=common::pmr::map<common::TaskContextId,common::SharedPtr<ConnectionContext>,std::less<>>;
    using ConnectionCtxShared=common::SharedPtr<ConnectionContext>;

    public:        

        ConnectionPool(
                common::SharedPtr<RouterT> router,
                common::Thread* thread,
                const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
            ) : m_router(router),
                m_allocatorFactory(factory),
                m_maxConnectionsPerPriority(DefaultMaxPoolPriorityConnections),
                m_closed(false),
                m_singleConnection(true),
                m_maxMessageSize(protocol::DEFAULT_MAX_MESSAGE_SIZE),
                m_defaultConnection(common::allocateShared(factory->objectAllocator<ConnectionContext>())),
                m_thread(thread),
                m_autoReconnect(true),
                m_totalConnectionCount(0)
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

        bool isSingleConnectionMode() const noexcept
        {
            return m_singleConnection;
        }

        void setAutoReconnect(bool enable) noexcept
        {
            m_autoReconnect=enable;
        }

        bool isAutoReconnectMode() const noexcept
        {
            return m_autoReconnect;
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
                cb(commonError(CommonError::ABORTED),ConnectionCtxShared{});
                return;
            }

            // process single connection mode
            if (m_singleConnection)
            {
                auto connectionCtx=m_defaultConnection;

                if (connectionCtx->state==ConnectionContext::State::Busy)
                {
                    cb(apiLibError(ApiLibError::CONNECTION_BUSY),ConnectionCtxShared{});
                    return;
                }

                auto connectCb=[this,ctx,cb,buffers(std::move(buffers)),connectionCtx](const Error& ec)
                {
                    if (ec)
                    {
                        cb(ec,ConnectionCtxShared{});
                        return;
                    }
                    sendToConnection(std::move(ctx),std::move(cb),std::move(connectionCtx),std::move(buffers));
                };

                if (connectionCtx->state==ConnectionContext::State::Disconnected)
                {
                    connect(std::move(ctx),std::move(connectCb));
                }
                else if (connectionCtx->state==ConnectionContext::State::Error)
                {
                    auto closeCb=[ctx,this,connectCb{std::move(connectCb)}](const Error&)
                    {
                        connect(std::move(ctx),std::move(connectCb));
                    };
                    closeDefaultConnection(ctx,closeCb);
                }

                return;
            }


            cb(commonError(CommonError::NOT_IMPLEMENTED),ConnectionCtxShared{});
            return;

            std::ignore=priority;
            //! @todo implement pool operations
            // auto it=m_connections.find(priority);

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

            if (connectionCtx->state!=ConnectionContext::State::Ready)
            {
                cb(apiLibError(ApiLibError::CONNECTION_NOT_READY_RECV));
                return;
            }

            connectionCtx->state=ConnectionContext::State::Busy;

            auto recvMessageCb=[cb,connectionCtx](auto, const Error& ec)
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
            auto recvHeaderCb=[cb,connectionCtx,&buf,recvMessageCb{std::move(recvMessageCb)},this](auto ctx, const Error& ec)
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
                    cb(apiLibError(ApiLibError::TOO_BIG_RX_MESSAGE));
                    return;
                }

                buf.mainContainer()->resize(messageSize);
                buf.setSize(messageSize);
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
            m_thread->execAsync(
                [this,ctx{std::move(ctx)},cb{std::move(cb)}]()
                {
                    if (m_defaultConnection->state==ConnectionContext::State::Busy)
                    {
                        cb(apiLibError(ApiLibError::CONNECTION_BUSY));
                        return;
                    }

                    m_defaultConnection->state=ConnectionContext::State::Busy;
                    auto makeCb=[this,cb{std::move(cb)}](const common::Error& ec, common::SharedPtr<RouterConnectionCtx> connectionCtx)
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
                    makeConnection(ctx,makeCb,true);
                }
            );
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

        void setName(lib::string_view name)
        {
            m_name=name;
        }

        const std::string& name() const noexcept
        {
            return m_name;m_name;
        }

    private:

        template <typename ContextT, typename CallbackT>
        void closeDefaultConnection(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb
            )
        {
            //! @todo Do we need to reset default connection?
#if 0
            auto cb1=[cb{std::move(cb)},this](const Error& ec)
            {
                m_defaultConnection=common::allocateShared(m_allocatorFactory->objectAllocator<ConnectionContext>());
                cb(ec);
            };
#endif
            closeConnection(std::move(ctx),m_defaultConnection,std::move(cb));
        }

        template <typename ContextT, typename CallbackT>
        void closeConnection(
            common::SharedPtr<ContextT> ctx,
            ConnectionCtxShared connection,
            CallbackT cb
            )
        {
            auto cb1=[ctx,cb{std::move(cb)},connection,this](const Error& ec)
            {
                connection->ctx->resetParentCtx();
                connection->state=ConnectionContext::State::Disconnected;
                connection->sendTriesCount=0;

                m_thread->execAsync(
                    [cb{std::move(cb)},ec]()
                    {
                        cb(ec);
                    }
                );
            };
            connection->state=ConnectionContext::State::Busy;
            connection->ctx->resetParentCtx(ctx);
            connection->connection().close(std::move(cb1));
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
                [ctx{std::move(ctx)},it,cb{std::move(cb)},this]()
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
            closeConnection(
                ctx,
                connectionCtx,
                [ctx,it,&connections,cb{std::move(cb)},this](const Error&)
                {
                    connections.erase(it);
                    closeNextConnection(std::move(ctx),connections,std::move(cb));
                }
            );
        }

        void resetPriority(Priority priority)
        {
            m_connections[priority]=Connections{m_allocatorFactory->objectMemoryResource()};
        }

        template <typename ContextT>
        void makeConnection(
            common::SharedPtr<ContextT> ctx,
            std::function<void (const Error& ec, common::SharedPtr<RouterConnectionCtx> connectionCtx)> cb,
            bool defaultConnection=false,
            Priority priority=Priority::Normal
            )
        {
            auto makeCb=[ctx,cb{std::move(cb)}](const common::Error& ec, common::SharedPtr<RouterConnectionCtx> connectionCtx)
            {
                if (ec)
                {
                    cb(ec,std::move(connectionCtx));
                    return;
                }

                connectionCtx->resetParentCtx(std::move(ctx));
                auto& connection=connectionCtx->template get<Connection>();
                auto connectCb=[connectionCtx{std::move(connectionCtx)},ctx{std::move(ctx)},cb{std::move(cb)}](const common::Error& ec)
                {
                    connectionCtx->resetParentCtx();
                    cb(ec,std::move(connectionCtx));
                };
                connection.connect(std::move(connectCb));
            };
            common::StringOnStack name;
            ++m_totalConnectionCount;
            if (defaultConnection)
            {
                fmt::format_to(std::back_inserter(name),"{}_default",m_name);
            }
            else
            {
                fmt::format_to(std::back_inserter(name),"{}_{}_{}",m_name,priorityName(priority),++m_totalConnectionCount);
            }
            m_router->makeConnection(ctx,makeCb,name);
        }

        template <typename ContextT, typename CallbackT>
        void sendToConnection(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            ConnectionCtxShared connectionCtx,
            common::SpanBuffers buffers
            )
        {
            connectionCtx->state=ConnectionContext::State::Busy;

            // set message size
            auto messageSize=common::SpanBufferTraits::size(buffers);
            if (messageSize>m_maxMessageSize)
            {
                cb(apiLibError(ApiLibError::TOO_BIG_TX_MESSAGE),ConnectionCtxShared{});
                return;
            }
            connectionCtx->header.setMessageSize(messageSize);

            // send
            auto sendMessageCb=[connectionCtx,cb{std::move(cb)}](auto, const Error& ec, size_t, common::SpanBuffers)
            {
                connectionCtx->ctx->resetParentCtx();
                if (ec)
                {
                    //! @todo handle failed pool connection
                    connectionCtx->state=ConnectionContext::State::Error;
                    cb(ec,ConnectionCtxShared{});
                    return;
                }
                connectionCtx->state=ConnectionContext::State::Ready;
                cb(Error{},std::move(connectionCtx));
            };
            auto sendHeaderCb=[this,sendMessageCb{std::move(sendMessageCb)},connectionCtx,cb{std::move(cb)},buffers{std::move(buffers)}](
                                        auto ctx, const Error& ec, size_t
                                    )
            {
                connectionCtx->ctx->resetParentCtx();

                // handle error
                if (ec)
                {
                    connectionCtx->state=ConnectionContext::State::Error;

                    // check if auto reconnect can be invoked
                    if (connectionCtx->sendTriesCount>0 || !m_autoReconnect || m_closed)
                    {
                        connectionCtx->sendTriesCount=0;
                        cb(ec,ConnectionCtxShared{});
                        return;
                    }

                    // try to reconnect connection
                    auto reconnectCb=[this,connectionCtx,cb,ctx,buffers{std::move(buffers)}](const Error& ec, ConnectionCtxShared connection)
                    {
                        if (ec)
                        {
                            cb(ec,ConnectionCtxShared{});
                            return;
                        }

                        connection->sendTriesCount++;
                        this->sendToConnection(std::move(ctx),std::move(cb),std::move(connection),std::move(buffers));
                    };

                    // close connection
                    closeConnection(ctx,connectionCtx,
                        [this,connectionCtx,reconnectCb,ctx,cb](const common::Error&)
                        {
                            // reconnect connection
                            if (connectionCtx->defaultConnection)
                            {
                                connect(ctx,
                                    [connectionCtx,reconnectCb{std::move(reconnectCb)},cb](const Error& ec)
                                    {
                                        if (ec)
                                        {
                                            cb(ec,ConnectionCtxShared{});
                                            return;
                                        }
                                        reconnectCb(Error{},std::move(connectionCtx));
                                    }
                                );
                            }
                            else
                            {
                                newPoolConnection(ctx,connectionCtx->priority,std::move(reconnectCb));
                            }
                        }
                    );

                    return;
                }

                // send message
                connectionCtx->ctx->resetParentCtx(ctx);
                connectionCtx->connection().send(std::move(ctx),std::move(buffers),std::move(sendMessageCb));
            };
            // send header
            connectionCtx->ctx->resetParentCtx(ctx);
            connectionCtx->connection().send(std::move(ctx),connectionCtx->header.data(),connectionCtx->header.size(),std::move(sendHeaderCb));
        }

        template <typename ContextT>
        void newPoolConnection(common::SharedPtr<ContextT> ctx, Priority priority, std::function<void (const Error& ec, ConnectionCtxShared)> cb)
        {
            //! @todo Implement newPoolConnection
            std::ignore=ctx;
            std::ignore=priority;
            cb(commonError(CommonError::NOT_IMPLEMENTED),ConnectionCtxShared{});
        }

        common::SharedPtr<RouterT> m_router;
        const common::pmr::AllocatorFactory* m_allocatorFactory;

        common::FlatMap<Priority,Connections> m_connections;

        size_t m_maxConnectionsPerPriority;
        bool m_closed;

        bool m_singleConnection;
        uint32_t m_maxMessageSize;

        ConnectionCtxShared m_defaultConnection;
        common::Thread* m_thread;
        bool m_autoReconnect;

        std::string m_name;
        size_t m_totalConnectionCount;
};

HATN_API_NAMESPACE_END

#endif // HATNAPISCONNECTIONPOOL_H
