/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)



  */

/****************************************************************************/

/** @file network/socks5client.cpp
  *
  *   Class to parse and construct messages of SOCKS5 protocol.
  *
  */

/****************************************************************************/

#include <hatn/network/networkerrorcodes.h>
#include <hatn/network/networkerror.h>
#include <hatn/network/socks5client.h>

HATN_NETWORK_NAMESPACE_BEGIN
HATN_COMMON_USING

namespace {

    constexpr static const uint8_t SOCKS5_VERSION=5;
    constexpr static const uint8_t SOCKS5_AUTH_VERSION=1;

    enum AuthMethod
    {
        METHOD_NO_AUTH=0,
        METHOD_GSSAPI=1,
        METHOD_USERNAME_PASSWORD=2,
        METHOD_NOT_ACCEPTED=0xFF
    };

    enum ConnectCommand
    {
        COMMAND_CONNECT=0x01,
        COMMAND_BIND=0x02,
        COMMAND_UDP_ASSOCIATE=0x03
    };

    enum ConnectResponse
    {
        RESP_SUCCESS=0x00
    };

    enum AddressType
    {
        ADDRESS_IP4=0x01,
        ADDRESS_DOMAIN=0x03,
        ADDRESS_IP6=0x04
    };

    struct Socks5NegRequest
    {
        constexpr static const size_t Length=5;
        std::array<uint8_t,Length> data;

        inline uint8_t& version() noexcept
        {
            return data[0];
        }
        inline uint8_t& authMethodsLength() noexcept
        {
            return data[1];
        }
        inline uint8_t& authNo() noexcept
        {
            return data[2];
        }
        inline uint8_t& authPasswd() noexcept
        {
            return data[3];
        }
    };

    struct Socks5NegResponse
    {
        constexpr static const size_t Length=2;
        std::array<uint8_t,Length> data;

        inline uint8_t& version() noexcept
        {
            return data[0];
        }
        inline uint8_t& method() noexcept
        {
            return data[1];
        }
    };

    struct Socks5ConnectHeaderPart
    {
        constexpr static const size_t Length=5;
        std::array<uint8_t,Length> data;

        inline uint8_t& version() noexcept
        {
            return data[0];
        }
        inline uint8_t& command() noexcept
        {
            return data[1];
        }
        inline uint8_t& reserved() noexcept
        {
            return data[2];
        }
        inline uint8_t& addressType() noexcept
        {
            return data[3];
        }
        inline uint8_t& addressLength() noexcept
        {
            return data[4];
        }
    };

    struct Socks5UdpHeader
    {
        constexpr static const size_t Length=4;
        constexpr static const size_t ATypeOffset=3;
        std::array<uint8_t,Length> data;

        inline uint16_t* reserved() noexcept
        {
            return reinterpret_cast<uint16_t*>(data.data());
        }

        inline uint8_t& fragmentation() noexcept
        {
            return data[2];
        }

        inline uint8_t& addressType() noexcept
        {
            return data[ATypeOffset];
        }
    };
}

/*********************** Socks5Client **************************/

//---------------------------------------------------------------
Socks5Client::Socks5Client(
        const common::pmr::AllocatorFactory* allocatorFactory
    ) : m_state(State::Idle),
        m_request(allocatorFactory->dataMemoryResource()),
        m_response(allocatorFactory->dataMemoryResource()),
        m_udp(false),
        m_addressType(0),
        m_addressLength(0)
{
}

//---------------------------------------------------------------
Socks5Client::StepStatus Socks5Client::nextRequest()
{
    Socks5Client::StepStatus ret=Socks5Client::StepStatus::Fail;

    switch (m_state)
    {
        case(State::Idle):
        {
            // DCS_DEBUG_ID(socks5client,"Sending negotiation request");

            if (!m_destination.isValid())
            {
                m_error=networkError(NetworkError::PROXY_INVALID_PARAMETERS);
                ret=Socks5Client::StepStatus::Fail;
#if 0
                DCS_WARN_ID(socks5client,HATN_FORMAT("Invalid destination address {} or domain {} or port {}",
                                                    m_destination.endpoint.address().to_string(),
                                                    m_destination.domain,
                                                    m_destination.endpoint.port()));
#endif
            }
            else
            {
                m_request.reserve(128);
                m_response.reserve(128);

                m_request.resize(Socks5NegRequest::Length);
                Socks5NegRequest* request=reinterpret_cast<Socks5NegRequest*>(m_request.data());
                request->version()=SOCKS5_VERSION;
                request->authMethodsLength()=2;
                request->authNo()=METHOD_NO_AUTH;
                request->authPasswd()=METHOD_USERNAME_PASSWORD;

                m_response.resize(Socks5NegResponse::Length);
                ret=Socks5Client::StepStatus::SendAndReceive;
                m_state=State::Negotiation;
            }
        }
            break;

        case(State::Negotiation):
        {
            Socks5NegResponse* response=reinterpret_cast<Socks5NegResponse*>(m_response.data());

            if (response->version()!=SOCKS5_VERSION)
            {
                m_error=networkError(NetworkError::PROXY_UNSUPPORTED_VERSION);
                ret=Socks5Client::StepStatus::Fail;
                // DCS_WARN_ID(socks5client,HATN_FORMAT("Negotiation response, unsupported version={}",static_cast<uint32_t>(response->version())));
            }
            else
            {
                if (response->method()==METHOD_NO_AUTH)
                {
                    // DCS_DEBUG_ID(socks5client,"Negotiation response received, no auth required");
                    sendConnectRequest();
                    ret=Socks5Client::StepStatus::SendAndReceive;
                }
                else if (response->method()==METHOD_USERNAME_PASSWORD)
                {
                    // DCS_DEBUG_ID(socks5client,"Negotiation response received, auth required with username and password");

                    auto requestSize=3+m_login.size()+m_password.size();
                    m_request.resize(requestSize);
                    m_request[0]=SOCKS5_AUTH_VERSION;
                    m_request[1]=static_cast<uint8_t>(m_login.size());
                    if (m_login.size()!=0)
                    {
                        memcpy(&m_request[2],m_login.data(),m_login.size());
                    }
                    m_request[2+m_password.size()]= static_cast<uint8_t>(m_password.size());
                    if (m_password.size()!=0)
                    {
                        memcpy(&m_request[2+m_password.size()+1],m_password.data(),m_password.size());
                    }
                    m_response.resize(2);
                    m_state=State::Auth;
                    ret=Socks5Client::StepStatus::SendAndReceive;
                }
                else
                {
                    m_error=networkError(NetworkError::PROXY_UNSUPPORTED_AUTH_METHOD);
                    ret=Socks5Client::StepStatus::Fail;
                    // DCS_WARN_ID(socks5client,HATN_FORMAT("Negotiation response, unsupported auth method={}",static_cast<uint32_t>(response->method())));
                }
            }
        }
            break;

        case(State::Auth):
        {
            if (static_cast<uint8_t>(m_response[0])!=SOCKS5_AUTH_VERSION)
            {
                m_error=networkError(NetworkError::PROXY_UNSUPPORTED_VERSION);
                ret=Socks5Client::StepStatus::Fail;
                // DCS_WARN_ID(socks5client,HATN_FORMAT("Unsupported auth version {}",static_cast<uint32_t>(m_response[0])));
            }
            else if (m_response[1]!=0)
            {
                m_error=networkError(NetworkError::PROXY_AUTH_FAILED);
                ret=Socks5Client::StepStatus::Fail;
                // DCS_WARN_ID(socks5client,HATN_FORMAT("Auth failed {}",static_cast<uint32_t>(m_response[1])));
            }
            else
            {
                // DCS_DEBUG_ID(socks5client,"Auth ok");
                sendConnectRequest();
                ret=Socks5Client::StepStatus::SendAndReceive;
            }
        }
            break;

        case(State::Connect):
        {
            m_request.clear();
            Socks5ConnectHeaderPart* headerPtr=reinterpret_cast<Socks5ConnectHeaderPart*>(m_response.data());
            if (headerPtr->version()!=SOCKS5_VERSION)
            {
                // DCS_WARN_ID(socks5client,HATN_FORMAT("Unsupported SOCKS proxy version {} when trying to connect",static_cast<uint32_t>(headerPtr->version())));
                m_error=networkError(NetworkError::PROXY_UNSUPPORTED_VERSION);
                ret=Socks5Client::StepStatus::Fail;
            }
            else if (headerPtr->command()!=RESP_SUCCESS)
            {
                // DCS_WARN_ID(socks5client,HATN_FORMAT("Proxy responed with error {}",static_cast<uint32_t>(headerPtr->command())));
                m_error=networkError(NetworkError::PROXY_REPORTED_ERROR);
                ret=Socks5Client::StepStatus::Fail;
            }
            else
            {
                // DCS_DEBUG_ID(socks5client,"Connection response part 1 recevied ok, addressType={}"<<static_cast<int>(headerPtr->addressType()));
                ret=Socks5Client::StepStatus::Receive;
                m_state=State::ReceiveAddress;
                m_addressType=headerPtr->addressType();
                m_addressLength=headerPtr->addressLength();
                if (headerPtr->addressType()==ADDRESS_IP4)
                {
                    // DCS_DEBUG_ID(socks5client,"Connection response part 1 - IPv4 address");
                    m_response.resize(3+2);
                }
                else if (headerPtr->addressType()==ADDRESS_IP6)
                {
                    // DCS_DEBUG_ID(socks5client,"Connection response part 1 - IPv6 address");
                    m_response.resize(3+2+2);
                }
                else
                {
                    // DCS_DEBUG_ID(socks5client,HATN_FORMAT("Connection response part 1 - domain name address type, length={}",static_cast<int>(m_addressLength)));
                    if (headerPtr->addressLength()==0)
                    {
                        // DCS_WARN_ID(socks5client,"Connection response part 1 - no address is set");
                        ret=Socks5Client::StepStatus::Fail;
                    }
                    else
                    {
                        m_response.resize(headerPtr->addressLength()+2);
                    }
                }
            }
        }
            break;

        case(State::ReceiveAddress):
        {
            if (m_addressType==ADDRESS_IP4)
            {
                auto& firstOctet=m_addressLength; // for IPv4 we kept first octet in address length field
                boost::asio::ip::address_v4::bytes_type bytes;
                bytes[0]=firstOctet;
                memcpy(bytes.data()+1,m_response.data(),3);

                m_result.endpoint.setAddress(boost::asio::ip::address_v4(bytes));

                uint16_t port=0;
                memcpy(&port,m_response.data()+3,2);
                port=htons(port);
                m_result.endpoint.setPort(port);
#if 0
                DCS_DEBUG_ID(socks5client,HATN_FORMAT("Receive address response for IPv4 {}:{}",
                                                     m_result.endpoint.address().to_string(),
                                                     m_result.endpoint.port()));
#endif
            }
            else if (m_addressType==ADDRESS_IP6)
            {
                auto& firstOctet=m_addressLength; // for IPv6 we kept first octet in address length field
                boost::asio::ip::address_v6::bytes_type bytes;
                bytes[0]=firstOctet;
                memcpy(bytes.data()+1,m_response.data(),5);

                m_result.endpoint.setAddress(boost::asio::ip::address_v6(bytes));

                uint16_t port=0;
                memcpy(&port,m_response.data()+5,2);
                port=htons(port);
                m_result.endpoint.setPort(port);
#if 0
                DCS_DEBUG_ID(socks5client,HATN_FORMAT("Receive address response for IPv6 {}:{}",
                                                     m_result.endpoint.address().to_string(),
                                                     m_result.endpoint.port()));
#endif
            }
            else if (m_addressType==ADDRESS_DOMAIN)
            {
                m_result.domain=std::string(reinterpret_cast<char*>(m_response.data()),m_addressLength);
                uint16_t port=0;
                memcpy(&port,m_response.data()+m_addressLength,2);
                port=htons(port);
                m_result.endpoint.setPort(port);
#if 0
                DCS_DEBUG_ID(socks5client,HATN_FORMAT("Receive address response for domain {}:{}",
                                        m_result.domain,
                                        m_result.endpoint.port()));
#endif
            }
            ret=Socks5Client::StepStatus::Done;
            m_state=State::Done;
            resetBuffers();
        }
            break;

        default:
            break;
    };

    return ret;
}

//---------------------------------------------------------------
void Socks5Client::reset() noexcept
{
    m_state=State::Idle;
    m_error=common::Error();
    m_result.clear();
    m_addressType=0;
    m_addressLength=0;
}

//---------------------------------------------------------------
void Socks5Client::sendConnectRequest() noexcept
{
    Socks5ConnectHeaderPart* headerPtr=nullptr;
    if (m_destination.domain.empty())
    {
        size_t headerSize=(m_destination.endpoint.address().is_v4()?4:6) + sizeof(Socks5ConnectHeaderPart)+-1+2;
        m_request.resize(headerSize);

        headerPtr=reinterpret_cast<Socks5ConnectHeaderPart*>(m_request.data());

        headerPtr->addressType()=m_destination.endpoint.address().is_v4()?ADDRESS_IP4:ADDRESS_IP6;
        headerPtr->addressLength()=0;
        size_t portOffset=0;
        if (m_destination.endpoint.address().is_v4())
        {
            auto bytes=m_destination.endpoint.address().to_v4().to_bytes();
            memcpy(m_request.data()+sizeof(Socks5ConnectHeaderPart)-1,bytes.data(),bytes.size());
            portOffset=bytes.size();
        }
        else
        {
            auto bytes=m_destination.endpoint.address().to_v6().to_bytes();
            memcpy(m_request.data()+sizeof(Socks5ConnectHeaderPart)-1,bytes.data(),bytes.size());
            portOffset=bytes.size();
        }
        uint16_t port=htons(m_destination.endpoint.port());
        memcpy(m_request.data()+sizeof(Socks5ConnectHeaderPart)-1+portOffset,&port,2);
#if 0
        DCS_DEBUG_ID(socks5client,HATN_FORMAT("Connecting to address {}:{}",
                                             m_destination.endpoint.address().to_string(),
                                             m_destination.endpoint.port()));
#endif
    }
    else
    {
        size_t headerSize=sizeof(Socks5ConnectHeaderPart)+m_destination.domain.size()+2;
        m_request.resize(headerSize);

        headerPtr=reinterpret_cast<Socks5ConnectHeaderPart*>(m_request.data());

        headerPtr->addressType()=ADDRESS_DOMAIN;
        headerPtr->addressLength()= static_cast<uint8_t>(m_destination.domain.size());
        uint16_t port=htons(m_destination.endpoint.port());
        memcpy(m_request.data()+sizeof(Socks5ConnectHeaderPart),m_destination.domain.data(),m_destination.domain.size());
        memcpy(m_request.data()+sizeof(Socks5ConnectHeaderPart)+m_destination.domain.size(),&port,2);

        // DCS_DEBUG_ID(socks5client,HATN_FORMAT("Connecting to domain {}:{}",m_destination.domain,m_destination.endpoint.port()));
    }

    headerPtr->version()=SOCKS5_VERSION;
    headerPtr->reserved()=0;
    if (m_udp)
    {
        headerPtr->command()=COMMAND_UDP_ASSOCIATE;
        // DCS_DEBUG_ID(socks5client,"Sending UDP associate");
    }
    else
    {
        headerPtr->command()=COMMAND_CONNECT;
        // DCS_DEBUG_ID(socks5client,"Sending TCP connect");
    }

    m_response.resize(sizeof(Socks5ConnectHeaderPart::Length));
    m_state=State::Connect;
}

//---------------------------------------------------------------
void Socks5Client::wrapUdpPacket(const Target &destination, ByteArray &packet)
{
    int ipAddrLength=destination.endpoint.address().is_v4()?4:6;
    size_t headerLength=Socks5UdpHeader::Length+(destination.domain.empty()?ipAddrLength:(destination.domain.size()+1)) + 2;
    packet.rresize(packet.size()+headerLength);
    packet[0]=0;
    packet[1]=0;
    packet[2]=0;

    uint8_t aType=destination.endpoint.address().is_v4()?ADDRESS_IP4:ADDRESS_IP6;
    if (!destination.domain.empty())
    {
        aType=ADDRESS_DOMAIN;
    }
    packet[Socks5UdpHeader::ATypeOffset]=aType;

    size_t portOffset=Socks5UdpHeader::Length;
    if (!destination.domain.empty())
    {
        packet[4]= static_cast<uint8_t>(destination.domain.size());
        memcpy(packet.data()+Socks5UdpHeader::Length+1,destination.domain.data(),destination.domain.size());
        portOffset+=destination.domain.size()+1;
    }
    else if (destination.endpoint.address().is_v4())
    {
        auto bytes=destination.endpoint.address().to_v4().to_bytes();
        memcpy(packet.data()+Socks5UdpHeader::Length,bytes.data(),bytes.size());
        portOffset+=bytes.size();
    }
    else
    {
        auto bytes=destination.endpoint.address().to_v6().to_bytes();
        memcpy(packet.data()+Socks5UdpHeader::Length,bytes.data(),bytes.size());
        portOffset+=bytes.size();
    }
    auto port=htons(destination.endpoint.port());
    memcpy(packet.data()+portOffset,&port,2);
}

//---------------------------------------------------------------
bool Socks5Client::unwrapUdpPacket(Target &source,ByteArray &packet)
{
    bool ok=packet.size()>Socks5UdpHeader::Length+2;
    if (ok)
    {
        size_t portOffset=Socks5UdpHeader::Length;
        uint8_t aType=packet[3];
        switch (aType)
        {
            case (ADDRESS_IP4):
            {
                ok=packet.size()>Socks5UdpHeader::Length+2+4;
                if (ok)
                {
                    boost::asio::ip::address_v4::bytes_type bytes;
                    memcpy(bytes.data(),packet.data()+Socks5UdpHeader::Length,bytes.size());
                    source.endpoint.setAddress(boost::asio::ip::address_v4(bytes));
                    portOffset+=bytes.size();
                }
            }
                break;

            case (ADDRESS_IP6):
            {
                ok=packet.size()>Socks5UdpHeader::Length+2+6;
                if (ok)
                {
                    boost::asio::ip::address_v6::bytes_type bytes;
                    memcpy(bytes.data(),packet.data()+Socks5UdpHeader::Length,bytes.size());
                    source.endpoint.setAddress(boost::asio::ip::address_v6(bytes));
                    portOffset+=bytes.size();
                }
            }
                break;

            case (ADDRESS_DOMAIN):
            {
                uint8_t domainLength=*(packet.data()+Socks5UdpHeader::Length);
                ok=packet.size()>(Socks5UdpHeader::Length+2+1+domainLength);
                if (ok)
                {
                    source.domain=std::string(packet.data()+Socks5UdpHeader::Length+1,domainLength);
                    portOffset+=domainLength+1;
                }
            }
                break;

            default:
            {
                ok=false;
            }
                break;
        }
        if (ok)
        {
            uint16_t port=0;
            memcpy(&port,packet.data()+portOffset,2);
            port=htons(port);
            source.endpoint.setPort(port);

            packet.rresize(packet.size()+portOffset+2);
        }
    }
    return ok;
}

//---------------------------------------------------------------
HATN_NETWORK_NAMESPACE_END


