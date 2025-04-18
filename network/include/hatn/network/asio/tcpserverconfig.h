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
#include <hatn/common/taskcontext.h>

#include <hatn/network/network.h>
#include <hatn/network/asio/socket.h>

HATN_NETWORK_NAMESPACE_BEGIN
namespace asio {

//! Interface for ASIO TCP server configuration
class HATN_NETWORK_EXPORT TcpServerConfig
{
    public:

        //! Ctor
        TcpServerConfig(
            int listenBacklog=boost::asio::ip::tcp::socket::max_listen_connections
        )  noexcept : m_listenBacklog(listenBacklog)
        {
        }

        virtual ~TcpServerConfig()=default;
        TcpServerConfig(const TcpServerConfig&)=default;
        TcpServerConfig(TcpServerConfig&&) =default;
        TcpServerConfig& operator=(const TcpServerConfig&)=default;
        TcpServerConfig& operator=(TcpServerConfig&&) =default;

        //! Set acceptor options throwing exception on error
        inline void fillAcceptorOptions(boost::asio::ip::tcp::acceptor& acceptor) const
        {
            boost::system::error_code ec;
            fillAcceptorOptions(acceptor,ec);
            if (ec)
            {
                throw boost::system::system_error(ec);
            }
        }

        //! Set acceptor options without throwing exceptions
        virtual void fillAcceptorOptions(boost::asio::ip::tcp::acceptor& acceptor, boost::system::error_code& ec) const
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

HATN_TASK_CONTEXT_DECLARE(HATN_NETWORK_NAMESPACE::asio::TcpServerConfig,HATN_NETWORK_EXPORT)

#endif // HATNASIOTCPSERVERCONFIG_H
