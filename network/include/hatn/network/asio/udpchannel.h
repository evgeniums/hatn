/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file network/asio/asioudpchannel.h
  *
  *     UDP channels using boost.asio backend.
  */

/****************************************************************************/

#ifndef HATNUDPCHANNEL_H
#define HATNUDPCHANNEL_H

#include <hatn/network/network.h>
#include <hatn/network/unreliablechannel.h>

#include <hatn/network/asio/ipendpoint.h>
#include <hatn/network/asio/socket.h>

HATN_NETWORK_NAMESPACE_BEGIN

namespace asio {

//! Asynchronous UDP channel
template <typename UdpChannelT>
class UdpChannelTraits : public WithSocket<UdpSocket>
{
    public:

        //! Constructor
        UdpChannelTraits(
            UdpChannelT* channel
        );

        //! Open and bind ASIO UDP socket
        void bind(
            UdpEndpoint endpoint,
            std::function<void (const common::Error &)> callback
        );

        //! Open and bind ASIO UDP socket
        void bind(
            std::function<void (const common::Error &)> callback
        );

        //! Close channel
        void close(const std::function<void (const common::Error &)>& callback, bool destroying=false);

        //! Check if channel is open
        inline bool isOpen() const noexcept
        {
            return rawSocket().is_open();
        }

        void cancel();

    protected:

        UdpChannelT* channel() const noexcept
        {
            return m_channel;
        }

    private:

        UdpChannelT* m_channel;
};

class UdpChannelMultiple;
class UdpChannelSingle;

#ifdef _WIN32
template class HATN_NETWORK_EXPORT UdpChannelTraits<UdpChannelMultiple>;
template class HATN_NETWORK_EXPORT UdpChannelTraits<UdpChannelSingle>;
#endif

class HATN_NETWORK_EXPORT UdpChannelMultipleTraits : public UdpChannelTraits<UdpChannelMultiple>
{
    public:

        using UdpChannelTraits<UdpChannelMultiple>::UdpChannelTraits;

        /**
         * @brief Prepare channel: open and bind ASIO socket, connect to remote socket.
         * @param callback Callback to call with result of operation.
         */
        inline void prepare(
            std::function<void (const common::Error &)> callback
        )
        {
            bind(std::move(callback));
        }

        //! Receive from channel
        void receiveFrom(
            char* buf,
            size_t maxSize,
            std::function<void (const common::Error&,size_t,const UdpEndpoint&)> callback
        );

        //! Send to channel
        void sendTo(
            const char* buf,
            size_t size,
            const UdpEndpoint& endpoint,
            std::function<void (const common::Error&,size_t)> callback
        );

        //! Write scattered buffers to channel
        void sendTo(
            common::SpanBuffers buffers,
            const UdpEndpoint& endpoint,
            std::function<void (const common::Error&,size_t,common::SpanBuffers)> callback
        );

    private:

        //! Temporary endpoint to put source endpoint from received packets
        boost::asio::ip::udp::endpoint m_rxBoostEndpoint;

};

class HATN_NETWORK_EXPORT UdpChannelSingleTraits final : public UdpChannelTraits<UdpChannelSingle>
{
    public:

        using UdpChannelTraits<UdpChannelSingle>::UdpChannelTraits;

        /**
         * @brief Prepare channel: open and bind ASIO socket, connect to remote socket.
         * @param callback Callback to call with result of operation.
         */
        void prepare(
            std::function<void (const common::Error &)> callback
        );

        //! Receive from channel
        void receive(
            char* buf,
            size_t maxSize,
            std::function<void (const common::Error&,size_t)> callback
        );

        //! Send to channel
        void send(
            const char* buf,
            size_t size,
            std::function<void (const common::Error&,size_t)> callback
        );

        //! Send scattered buffers to channel
        void send(
            common::SpanBuffers buffers,
            std::function<void (const common::Error&,size_t,common::SpanBuffers)> callback
        );
};

//! Asynchronous UDP channel to multiple endpoints
class HATN_NETWORK_EXPORT UdpChannelMultiple : public UnreliableChannelMultiple<UdpEndpoint,UdpChannelMultipleTraits>
{
    public:

        UdpChannelMultiple(
            common::Thread* thread=common::Thread::currentThread()
        );

        //! Open and bind ASIO UDP socket
        void bind(
            UdpEndpoint endpoint,
            std::function<void (const common::Error &)> callback
        )
        {
            traits().bind(std::move(endpoint),std::move(callback));
        }

        //! Open and bind ASIO UDP socket
        void bind(
            std::function<void (const common::Error &)> callback
        )
        {
            traits().bind(std::move(callback));
        }
};

//! Asynchronous UDP channel to single endpoint
class HATN_NETWORK_EXPORT UdpChannelSingle : public UnreliableChannelSingle<UdpEndpoint,UdpChannelSingleTraits>
{
    public:

        UdpChannelSingle(
            common::Thread* thread=common::Thread::currentThread()
        );

        //! Open and bind ASIO UDP socket
        void bind(
            UdpEndpoint endpoint,
            std::function<void (const common::Error &)> callback
        )
        {
            traits().bind(std::move(endpoint),std::move(callback));
        }

        //! Open and bind ASIO UDP socket
        void bind(
            std::function<void (const common::Error &)> callback
        )
        {
            traits().bind(std::move(callback));
        }
};

using UdpClient=UdpChannelSingle;
using UdpServer=UdpChannelMultiple;

template <typename T>
struct makeTypeCtxT
{
    template <typename ...BaseArgs>
    auto operator()(common::Thread* thread, BaseArgs&&... args) const
    {
        return common::makeTaskContext<T,HATN_LOGCONTEXT_NAMESPACE::LogCtxType>(
            common::subcontexts(
                common::subcontext(thread)
                ),
            common::subcontexts(
                common::subcontext()
                ),
            std::forward<BaseArgs>(args)...
            );
    }

    template <typename ...BaseArgs>
    auto operator()(BaseArgs&&... args) const
    {
        return common::makeTaskContext<T,HATN_LOGCONTEXT_NAMESPACE::LogCtxType>(
            common::subcontexts(
                common::subcontext()
                ),
            common::subcontexts(
                common::subcontext()
                ),
            std::forward<BaseArgs>(args)...
            );
    }

    auto operator()() const
    {
        return common::makeTaskContext<T,HATN_LOGCONTEXT_NAMESPACE::LogCtxType>();
    }
};

constexpr makeTypeCtxT<UdpServer> makeUdpServerCtx{};
using UdpServerSharedCtx=decltype(makeUdpServerCtx(""));

constexpr makeTypeCtxT<UdpClient> makeUdpClientCtx{};
using UdpClientSharedCtx=decltype(makeUdpClientCtx(""));

//---------------------------------------------------------------
} // namespace asio

HATN_NETWORK_NAMESPACE_END

HATN_TASK_CONTEXT_DECLARE(HATN_NETWORK_NAMESPACE::asio::UdpServer,HATN_NETWORK_EXPORT)
HATN_TASK_CONTEXT_DECLARE(HATN_NETWORK_NAMESPACE::asio::UdpClient,HATN_NETWORK_EXPORT)

#endif // HATNUDPCHANNEL_H
