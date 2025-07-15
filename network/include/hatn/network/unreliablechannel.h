/*
 Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file network/unreliablechannel.h
  *
  *     Base classes for unreliable channels
  *
  */

/****************************************************************************/

#ifndef HATNUNRELIABLECHANNEL_H
#define HATNUNRELIABLECHANNEL_H

#include <functional>

#include <hatn/common/error.h>
#include <hatn/common/containerutils.h>
#include <hatn/common/stream.h>
#include <hatn/common/spanbuffer.h>
#include <hatn/common/taskcontext.h>

#include <hatn/logcontext/contextlogger.h>

#include <hatn/network/network.h>
#include <hatn/network/endpoint.h>

#include <hatn/network/detail/asynchandler.ipp>

HATN_NETWORK_NAMESPACE_BEGIN

//! Asynchronous unreliable channel
template <typename EndpointT, typename Traits>
class UnreliableChannel : public common::TaskSubcontext,
                          public common::WithThread,
                          public common::WithPrepareClose<Traits>,
                          public WithLocalEndpoint<EndpointT>
{
    public:

        template <typename ...Args>
        UnreliableChannel(
                common::Thread* thread,
                Args&& ...traitsArgs
        ) : common::WithThread(thread),
            common::WithPrepareClose<Traits>(std::forward<Args>(traitsArgs)...)
        {}

        ~UnreliableChannel()=default;
        UnreliableChannel(const UnreliableChannel&)=delete;
        UnreliableChannel(UnreliableChannel&&) =delete;
        UnreliableChannel& operator=(const UnreliableChannel&)=delete;
        UnreliableChannel& operator=(UnreliableChannel&&) =delete;

        inline common::WeakPtr<common::TaskContext> ctxWeakPtr() const
        {
            return common::toWeakPtr(sharedMainCtx());
        }

        inline void leaveAsyncHandler()
        {
            mainCtx().onAsyncHandlerExit();
        }
};

//! Asynchronous unreliable channel to multiple endpoints
template <typename EndpointT,typename Traits>
class UnreliableChannelMultiple : public UnreliableChannel<EndpointT,Traits>
{
    public:

        using UnreliableChannel<EndpointT,Traits>::UnreliableChannel;

        //! Receive from channel
        inline void receiveFrom(
            char* buf,
            size_t maxSize,
            std::function<void (const Error&,size_t,const EndpointT&)> callback
        )
        {
            this->traits().receiveFrom(buf,maxSize,std::move(callback));
        }

        //! Send to channel
        inline void sendTo(
            const char* buf,
            size_t size,
            const EndpointT& endpoint,
            std::function<void (const Error&,size_t)> callback
        )
        {
            this->traits().sendTo(buf,size,endpoint,std::move(callback));
        }

        /**
         * @brief Send data from container to channel
         */
        template <typename ContainerT>
        inline void sendTo(
            const ContainerT& container,
            const EndpointT& endpoint,
            std::function<void (const Error&,size_t)> callback,
            size_t offset=0,
            size_t size=0
        )
        {
            auto span=common::SpanBuffer::span(container,offset,size);
            if (span.first)
            {
                sendTo(span.second.data(),span.second.size(),endpoint,std::move(callback));
            }
            else
            {
                this->thread()->execAsync(
                    [callback{std::move(callback)},wptr{this->ctxWeakPtr()},this]()
                    {
                        if (detail::enterAsyncHandler(wptr,callback))
                        {
                            {
                                HATN_CTX_SCOPE("unrcontainersendto")
                                callback(Error(CommonError::INVALID_SIZE),0);
                            }
                            this->leaveAsyncHandler();
                        }
                    }
                );
            }
        }

        //! Send managed buffer to channel
        inline void sendTo(
            common::SpanBuffer buffer,
            const EndpointT& endpoint,
            std::function<void (const Error&,size_t,common::SpanBuffer)> callback
        )
        {
            auto span=buffer.span();
            if (!span.first)
            {
                this->thread()->execAsync(
                    [buffer{std::move(buffer)},callback{std::move(callback)},wptr{this->ctxWeakPtr()},this]()
                    {
                        if (detail::enterAsyncHandler(wptr,callback,buffer))
                        {
                            {
                                HATN_CTX_SCOPE("unrbufsendto")
                                callback(Error(common::CommonError::INVALID_SIZE),0,std::move(buffer));
                            }
                            this->leaveAsyncHandler();
                        }
                    }
                );

                return;
            }
            auto cb=[buffer{std::move(buffer)},callback{std::move(callback)}](const Error& ec,size_t size)
            {
                callback(ec,size,std::move(buffer));
            };
            sendTo(span.second.data(),span.second.size(),endpoint,std::move(cb));
        }

        //! Write scattered buffers to channel
        inline void sendTo(
            common::SpanBuffers buffers,
            const EndpointT& endpoint,
            std::function<void (const Error&,size_t,common::SpanBuffers)> callback
        )
        {
            this->traits().sendTo(std::move(buffers),endpoint,std::move(callback));
        }
};

//! Asynchronous unreliable channel to single endpoint
template <typename EndpointT,typename Traits>
class UnreliableChannelSingle : public UnreliableChannel<EndpointT,Traits>,
                                public WithRemoteEndpoint<EndpointT>
{
    public:

        using UnreliableChannel<EndpointT,Traits>::UnreliableChannel;

        //! Receive from channel
        inline void receive(
            char* buf,
            size_t maxSize,
            std::function<void (const Error&,size_t)> callback
        )
        {
            this->traits().receive(buf,maxSize,std::move(callback));
        }

        //! Send to channel
        inline void send(
            const char* buf,
            size_t size,
            std::function<void (const Error&,size_t)> callback
        )
        {
            this->traits().send(buf,size,std::move(callback));
        }

        /**
         * @brief Send container to channel
         * @param container Container with data
         * @param callback
         */
        template <typename ContainerT>
        inline void send(
            const ContainerT& container,
            std::function<void (const Error&,size_t)> callback,
            size_t offset=0,
            size_t size=0
        )
        {
            auto span=common::SpanBuffer::span(container,offset,size);
            if (span.first)
            {
                send(span.second.data(),span.second.size(),std::move(callback));
            }
            else
            {
                this->thread()->execAsync(
                    [callback{std::move(callback)},wptr{this->ctxWeakPtr()},this]()
                    {
                        if (detail::enterAsyncHandler(wptr,callback))
                        {
                            {
                                HATN_CTX_SCOPE("unrsinglelcontainersend")
                                callback(Error(CommonError::INVALID_SIZE),0);
                            }
                            this->leaveAsyncHandler();
                        }
                    }
                );
            }
        }

        //! Send managed buffer to channel
        inline void send(
            common::SpanBuffer buffer,
            std::function<void (const Error&,size_t,common::SpanBuffer)> callback
        )
        {
            auto span=buffer.span();
            if (!span.first)
            {
                this->thread()->execAsync(
                    [buffer{std::move(buffer)},callback{std::move(callback)},wptr{this->ctxWeakPtr()},this]()
                    {
                        if (detail::enterAsyncHandler(wptr,callback,buffer))
                        {
                            {
                                HATN_CTX_SCOPE("unrsinglelbufsendto")
                                callback(Error(common::CommonError::INVALID_SIZE),0,std::move(buffer));
                            }
                            this->leaveAsyncHandler();
                        }
                    }
                );

                return;
            }
            auto cb=[buffer{std::move(buffer)},callback{std::move(callback)},wptr{this->ctxWeakPtr()},this](const Error& ec,size_t size)
            {
                if (detail::enterAsyncHandler(wptr,callback,buffer))
                {
                    {
                        HATN_CTX_SCOPE("unrlbufsendto")
                        callback(ec,size,std::move(buffer));
                    }
                    this->leaveAsyncHandler();
                }
            };
            send(span.second.data(),span.second.size(),std::move(cb));
        }

        //! Send scattered buffers to channel
        inline void send(
            common::SpanBuffers buffers,
            std::function<void (const Error&,size_t,common::SpanBuffers)> callback
        )
        {
            this->traits().send(std::move(buffers),std::move(callback));
        }
};

//---------------------------------------------------------------

HATN_NETWORK_NAMESPACE_END
#endif // HATNUNRELIABLECHANNEL_H
