/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file network/asio/tcpserverconfig.h
  *
  *   Interface for Asio TCP server configuration
  *
  */

/****************************************************************************/

#ifndef HATNASIOTCPSERVERCONFIG_H
#define HATNASIOTCPSERVERCONFIG_H

#include <hatn/common/error.h>
#include <hatn/common/environment.h>
#include <hatn/common/interface.h>

#include <hatn/network/network.h>
#include <hatn/network/asio/socket.h>

HATN_NETWORK_NAMESPACE_BEGIN
namespace asio {

//! Interface for Asio TCP server configuration
class HATN_NETWORK_EXPORT TcpServerConfig : public common::Interface<TcpServerConfig>
{
    public:

        HATN_CUID_DECLARE()

        //! Ctor
        TcpServerConfig(
            int listenBacklog=boost::asio::ip::tcp::socket::max_connections
        )  noexcept : m_listenBacklog(listenBacklog)
        {
        }

        virtual ~TcpServerConfig()=default;
        TcpServerConfig(const TcpServerConfig&)=default;
        TcpServerConfig(TcpServerConfig&&) =default;
        TcpServerConfig& operator=(const TcpServerConfig&)=default;
        TcpServerConfig& operator=(TcpServerConfig&&) =default;

        //! Set acceptor options throwing exception on error
        inline void setOptions(boost::asio::ip::tcp::acceptor& acceptor)
        {
            boost::system::error_code ec;
            setOptions(acceptor,ec);
            if (ec)
            {
                throw boost::system::system_error(ec);
            }
        }

        //! Set acceptor options without throwing exceptions
        virtual void setOptions(boost::asio::ip::tcp::acceptor& acceptor, boost::system::error_code& ec) noexcept
        {
            std::ignore=acceptor;
            std::ignore=ec;
        }

        inline int listenBacklog() const noexcept
        {
            return m_listenBacklog;
        }

    private:

        int m_listenBacklog;
};

} // namespace asio

HATN_NETWORK_NAMESPACE_END

#endif // HATNASIOTCPSERVERCONFIG_H
