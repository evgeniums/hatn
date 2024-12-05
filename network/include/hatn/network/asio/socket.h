/*
   Copyright (c) 2019 - current, Evgeny Sidorov (esid1976@gmail.com), All rights reserved
    
    
  */

/****************************************************************************/
/*
    
*/
/** \file network/asio/socket.h
  *
  *   Sockets based on boost asio
  *
  */

/****************************************************************************/

#ifndef HATNASIOSOCKET_H
#define HATNASIOSOCKET_H

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif

#include <boost/system/error_code.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#endif

#include <hatn/common/interface.h>
#include <hatn/common/stream.h>
#include <hatn/common/pmr/pmrtypes.h>

#include <hatn/network/network.h>

namespace hatn {
namespace network {
namespace asio {

//! Base class template for raw sockets over boost asio sockets
template <typename T> class Socket final
{
    public:

        using type=T;

        //! Ctor
        Socket(boost::asio::io_context& asioContext)
            : m_socket(asioContext)
        {}

        //! Get socket
        inline T& socket() noexcept
        {
            return m_socket;
        }

        //! Get socket
        inline const T& socket() const noexcept
        {
            return m_socket;
        }

    private:

        T m_socket;
};

using TcpSocket=Socket<boost::asio::ip::tcp::socket>;
using UdpSocket=Socket<boost::asio::ip::udp::socket>;

//! Base template for classes containing socket
template <typename SocketT> class WithSocket
{
    public:

        //! Ctor
        WithSocket(boost::asio::io_context& asioContext):m_socket(asioContext)
        {}

        //! Get asio UDP socket
        inline typename SocketT::type& rawSocket() noexcept
        {
            return m_socket.socket();
        }

        //! Get asio UDP socket
        inline const typename SocketT::type& rawSocket() const noexcept
        {
            return m_socket.socket();
        }

        //! Get socket
        inline SocketT& socket() noexcept
        {
            return m_socket;
        }

    private:

        SocketT m_socket;
};

inline bool fillAsioBuffers(
        const common::SpanBuffers& buffers,
        common::pmr::vector<boost::asio::const_buffer>& asioBuffers
    )
{
    asioBuffers.reserve(buffers.size());
    for (size_t i=0;i<buffers.size();i++)
    {
        auto span=buffers[i].span();
        if (span.first)
        {
            asioBuffers.emplace_back(span.second.data(),span.second.size());
        }
        else
        {
            return false;
        }
    }
    return true;
}

} // namespace asio
} // namespace network
} // namespace hatn

#endif // HATNASIOSOCKET_H
