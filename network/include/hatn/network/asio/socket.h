/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file network/asio/socket.h
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

HATN_LOG_MODULE_DECLARE_EXP(asio,HATN_NETWORK_EXPORT)

#define HatnAsioLog HLOG_MODULE(asio)

HATN_NETWORK_NAMESPACE_BEGIN
namespace asio {

// constexpr const uint8_t StreamDebugVerbosity=5;
constexpr const uint8_t DoneDebugVerbosity=0;

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
template <typename SocketT>
class WithSocket
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

HATN_NETWORK_NAMESPACE_END

#endif // HATNASIOSOCKET_H
