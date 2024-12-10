/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file network/asio/ipendpoint.h
  *
  *  Network IP address and endpoint, uses BOOST ASIO as backend.
  *
  */

/****************************************************************************/

#ifndef HATNASIOIPENDPOINT_H
#define HATNASIOIPENDPOINT_H

#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/basic_endpoint.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>

#include <hatn/common/meta/enumint.h>

#include <hatn/network/network.h>
#include <hatn/network/ipendpoint.h>

HATN_NETWORK_NAMESPACE_BEGIN
namespace asio {

//! Wrapper class to convert hatn::network::Protocol to boost asio ip protocol
template <int Protocol> struct ToNativeProtocol
{
};
template <> struct ToNativeProtocol<0>
{
    using type=boost::asio::ip::tcp;
};
template <> struct ToNativeProtocol<1>
{
    using type=boost::asio::ip::udp;
};
//! Convertor of hatn::network::Protocol to boost asio ip protocol
template <IpProtocol proto> struct ToBoostProtocol
{
    using type=typename ToNativeProtocol<common::EnumInt(proto)>::type;
};

//! Wrapper class to convert boost asio ip protocol to hatn::network::Protocol
template <typename Protocol> struct FromBoostProtocol
{
};
template <> struct FromBoostProtocol<boost::asio::ip::tcp>
{
    static const IpProtocol protocol=IpProtocol::TCP;
};
template <> struct FromBoostProtocol<boost::asio::ip::udp>
{
    static const IpProtocol protocol=IpProtocol::UDP;
};

using IpAddress=boost::asio::ip::address;
using IpEndpoint=network::IpEndpoint<IpAddress>;

//! Template class for IP protocol endpoints using Boost ASIO
template <IpProtocol ProtoT>
class IpEndpointT final : public IpEndpoint
{
    public:

        constexpr static const IpProtocol proto=ProtoT;
        using asioProto=typename ToBoostProtocol<proto>::type;

        using IpEndpoint::IpEndpoint;

        IpEndpointT(
                    const boost::asio::ip::basic_endpoint<asioProto>& endpoint
                ) noexcept : IpEndpoint(endpoint.address(),endpoint.port(),proto)
        {}
        IpEndpointT(
                    boost::asio::ip::basic_endpoint<asioProto>&& endpoint
                ) noexcept : IpEndpoint(std::move(endpoint.address()),endpoint.port(),proto)
        {}

        IpEndpointT(
                    const char* address,
                    uint16_t port=0
                ) : IpEndpoint(boost::asio::ip::make_address(address),port,proto)
        {}

        IpEndpointT(
                    const std::string& address,
                    uint16_t port=0
                ) : IpEndpoint(boost::asio::ip::make_address(address),port,proto)
        {}

        IpEndpointT(
            lib::string_view address,
            uint16_t port=0
            ) : IpEndpoint(boost::asio::ip::make_address(address),port,proto)
        {}

        explicit IpEndpointT(
                    const boost::asio::ip::address_v4& address
                ) : IpEndpoint(address,0,proto)
        {}

        explicit IpEndpointT(
                    const boost::asio::ip::address_v6& address
                ) : IpEndpoint(address,0,proto)
        {}

        IpEndpointT(const IpEndpoint& ep) noexcept : IpEndpoint(ep.address(),ep.port())
        {}
        inline IpEndpointT& operator= (const IpEndpoint& other) noexcept
        {
            copy(other);
            return *this;
        }
        inline IpEndpointT& operator=(const boost::asio::ip::basic_endpoint<asioProto>& endpoint) noexcept
        {
            fromBoostEndpoint(endpoint);
            return *this;
        }
        inline IpEndpointT& operator=(boost::asio::ip::basic_endpoint<asioProto>&& endpoint) noexcept
        {
            fromBoostEndpoint(std::move(endpoint));
            return *this;
        }

        inline boost::asio::ip::basic_endpoint<asioProto> toBoostEndpoint() const noexcept
        {
            return boost::asio::ip::basic_endpoint<asioProto>(this->address(),this->port());
        }
        inline void fromBoostEndpoint(const boost::asio::ip::basic_endpoint<asioProto>& endpoint) noexcept
        {
            this->setAddress(endpoint.address());
            this->setPort(endpoint.port());
        }
        inline void fromBoostEndpoint(boost::asio::ip::basic_endpoint<asioProto>&& endpoint) noexcept
        {
            this->setAddress(endpoint.address());
            this->setPort(endpoint.port());
        }

        using IpEndpoint::setAddress;

        void setAddress(const char* address)
        {
            this->setAddress(boost::asio::ip::make_address(address));
        }

        void setAddress(lib::string_view address)
        {
            this->setAddress(boost::asio::ip::make_address(address));
        }

        void setAddress(const std::string& address)
        {
            this->setAddress(boost::asio::ip::make_address(address));
        }

        void setAddress(const char* address, boost::system::error_code& ec)
        {
            this->setAddress(boost::asio::ip::make_address(address,ec));
        }

        void setAddress(lib::string_view address, boost::system::error_code& ec)
        {
            this->setAddress(boost::asio::ip::make_address(address,ec));
        }

        void setAddress(const std::string& address, boost::system::error_code& ec)
        {
            this->setAddress(boost::asio::ip::make_address(address,ec));
        }
};

using UdpEndpoint=IpEndpointT<IpProtocol::UDP>;
using TcpEndpoint=IpEndpointT<IpProtocol::TCP>;

}
}
}

#endif // HATNASIOENDPOINT_H
