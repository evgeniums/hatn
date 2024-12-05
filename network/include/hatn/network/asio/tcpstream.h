/*
   Copyright (c) 2019 - current, Evgeny Sidorov (esid1976@gmail.com), All rights reserved
    
    
  */

/****************************************************************************/
/*
    
*/
/** \file network/asio/tcpstream.h
  *
  *   Stream over ASIO TCP socket
  *
  */

/****************************************************************************/

#ifndef HATNASIOTCPSTREAM_H
#define HATNASIOTCPSTREAM_H

#include <hatn/common/objectguard.h>

#include <hatn/network/network.h>
#include <hatn/network/reliablestream.h>

#include <hatn/network/asio/ipendpoint.h>
#include <hatn/network/asio/socket.h>

DECLARE_LOG_MODULE_EXPORT(asiotcpstream,HATN_NETWORK_EXPORT)

namespace hatn {
namespace network {
namespace asio {

class TcpStream;

//! TcpStream handler traits
class HATN_NETWORK_EXPORT TcpStreamTraits : public common::WithGuard,
                                            public WithSocket<TcpSocket>
{
    public:

        //! Ctor
        TcpStreamTraits(
            common::Thread* thread,
            TcpStream* stream
        );

        //! Check if stream is open
        bool isOpen() const
        {
            return rawSocket().is_open();
        }

        /**
         * @brief Prepare stream: open, bind and connect ASIO socket
         * @param callback Status of stream preparation
         */
        void prepare(
            std::function<void (const common::Error &)> callback
        );

        //! Close stream
        void close(const std::function<void (const common::Error &)>& callback, bool destroying=false);

        //! Write to stream
        void write(
            const char* data,
            std::size_t size,
            std::function<void (const common::Error&,size_t)> callback
        );

        void write(
            common::SpanBuffers buffers,
            std::function<void (const common::Error&,size_t,common::SpanBuffers)> callback
        );

        //! Read from stream
        void read(
            char* data,
            std::size_t maxSize,
            std::function<void (const common::Error&,size_t)> callback
        );

        const char* idStr() const noexcept;

        void cancel();

    private:

        TcpStream* m_stream;
};

//! Stream over ASIO TCP socket
class HATN_NETWORK_EXPORT TcpStream final :
            public ReliableStreamWithEndpoints<TcpEndpoint,TcpStreamTraits>
{
    public:

        //! Constructor
        TcpStream(
            common::Thread* thread, //!< Thread the socket lives in
            common::STR_ID_TYPE id=common::STR_ID_TYPE() //!< Socket ID
        );

        //! Constructor
        TcpStream(
            common::STR_ID_TYPE id=common::STR_ID_TYPE() //!< Socket ID
        ) : TcpStream(common::Thread::currentThread(),std::move(id))
        {}

        ~TcpStream()=default;
        TcpStream(const TcpStream&)=delete;
        TcpStream(TcpStream&&) =delete;
        TcpStream& operator=(const TcpStream&)=delete;
        TcpStream& operator=(TcpStream&&) =delete;

        //! Update endpoints
        inline void updateEndpoints()
        {
            try
            {
                localEndpoint().fromBoostEndpoint(socket().socket().local_endpoint());
                remoteEndpoint().fromBoostEndpoint(socket().socket().remote_endpoint());
            }
            catch (const boost::system::system_error&)
            {
            }
        }

        //! Get socket
        inline TcpSocket& socket() noexcept
        {
            return traits().socket();
        }
};

} // namespace asio
} // namespace network
} // namespace hatn

#endif // HATNASIOTCPSTREAM_H
