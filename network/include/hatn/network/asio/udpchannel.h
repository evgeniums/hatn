/*
   Copyright (c) 2019 - current, Evgeny Sidorov (esid1976@gmail.com), All rights reserved
    
    
  */

/****************************************************************************/
/*
    
*/
/** \file network/asio/asioudpchannel.h
  *
  *     UDP channels
  *
  */

/****************************************************************************/

#ifndef HATNUDPCHANNEL_H
#define HATNUDPCHANNEL_H

#include <hatn/common/objectguard.h>

#include <hatn/network/network.h>
#include <hatn/network/unreliablechannel.h>

#include <hatn/network/asio/ipendpoint.h>
#include <hatn/network/asio/socket.h>

DECLARE_LOG_MODULE_EXPORT(asioudpchannel,HATN_NETWORK_EXPORT)

namespace hatn {
namespace network {
namespace asio {

//! Asynchronous UDP channel
template <typename UdpChannelT>
class UdpChannelTraits
        : public common::WithGuard,
          public WithSocket<UdpSocket>
{
    public:

        //! Constructor
        UdpChannelTraits(
            UdpChannelT* channel,
            common::Thread* thread
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

        const char* idStr() const noexcept;
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

class HATN_NETWORK_EXPORT UdpChannelMultipleTraits final : public UdpChannelTraits<UdpChannelMultiple>
{
    public:

        using UdpChannelTraits<UdpChannelMultiple>::UdpChannelTraits;

        /**
         * @brief Prepare channel: open and bind ASIO UDP socket
         * @param callback Status of preparation
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
         * @brief Prepare channel: open and bind ASIO socket, connect to remote socket
         * @param callback Status of preparation
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
class HATN_NETWORK_EXPORT UdpChannelMultiple final
        :  public UnreliableChannelMultiple<UdpEndpoint,UdpChannelMultipleTraits>
{
    public:

        UdpChannelMultiple(
            common::Thread* thread,
            common::STR_ID_TYPE id=common::STR_ID_TYPE()
        );

        UdpChannelMultiple(
            common::STR_ID_TYPE id=common::STR_ID_TYPE()
        ) : UdpChannelMultiple(common::Thread::currentThread(),std::move(id))
        {}

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
class HATN_NETWORK_EXPORT UdpChannelSingle final
        :  public UnreliableChannelSingle<UdpEndpoint,UdpChannelSingleTraits>
{
    public:

        UdpChannelSingle(
            common::Thread* thread,
            common::STR_ID_TYPE id=common::STR_ID_TYPE()
        );

        UdpChannelSingle(
            common::STR_ID_TYPE id=common::STR_ID_TYPE()
        ) : UdpChannelSingle(common::Thread::currentThread(),std::move(id))
        {}

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

//---------------------------------------------------------------
} // namespace asio
} // namespace network
} // namespace hatn
#endif // HATNUDPCHANNEL_H
