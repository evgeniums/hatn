/*
   Copyright (c) 2019 - current, Evgeny Sidorov (esid1976@gmail.com), All rights reserved
    
    
  */

/****************************************************************************/
/*
    
*/
/** \file network/endpoint.h
  *
  *  Network IP address and endpoint
  *
  */

/****************************************************************************/

#ifndef HATNNETWORKENDPOINT_H
#define HATNNETWORKENDPOINT_H

#include <hatn/network/network.h>

namespace hatn {
namespace network {

//! Base class for endpoints of channels and streams
template <typename AddressT, typename PortT, typename TypeT> class Endpoint
{
    public:

        //! Default ctor
        explicit Endpoint(
                typename TypeT::type type=TypeT::defaultValue()
            ) noexcept
                : m_address(AddressT::defaultValue()),
                  m_port(PortT::defaultValue()),
                  m_type(type)
        {}

        //! Ctor
        Endpoint(
                typename AddressT::type address,
                typename PortT::type port,
                typename TypeT::type type
            ) noexcept
                : m_address(std::move(address)),
                  m_port(std::move(port)),
                  m_type(std::move(type))
        {}

        //! Ctor
        Endpoint(
                typename AddressT::type address,
                typename PortT::type port
            ) noexcept
                : m_address(std::move(address)),
                  m_port(std::move(port)),
                  m_type(TypeT::defaultValue())
        {}

        //! Ctor
        Endpoint(
                const char* address,
                typename PortT::type port
            ) : m_address(AddressT::type::from_string(address)),
                  m_port(std::move(port)),
                  m_type(TypeT::defaultValue())
        {}

        //! Ctor
        explicit Endpoint(
                typename AddressT::type address
            ) noexcept
                : m_address(std::move(address)),
                  m_port(PortT::defaultValue()),
                  m_type(TypeT::defaultValue())
        {}

        template <typename T> explicit Endpoint(const T& other) noexcept
            : Endpoint(other.address(),other.port(),other.type())
        {
        }
        template <typename T> inline Endpoint& operator= (const T& other) noexcept
        {
            copy(other);
            m_type=other.type();
            return *this;
        }

        template <typename T> void copy(const T& other) noexcept
        {
            m_address=other.address();
            m_port=other.port();
        }

        //! Get type
        inline typename TypeT::type type() const noexcept
        {
            return m_type;
        }

        inline typename AddressT::type address() const noexcept
        {
            return m_address;
        }
        inline void setAddress(typename AddressT::type address) noexcept
        {
            m_address=std::move(address);
        }

        inline typename PortT::type port() const noexcept
        {
            return m_port;
        }
        inline void setPort(typename PortT::type port) noexcept
        {
            m_port=std::move(port);
        }

        inline void clear() noexcept
        {
            m_port=PortT::defaultValue();
            m_address=AddressT::defaultValue();
        }

    private:

        typename AddressT::type m_address;
        typename PortT::type m_port;
        typename TypeT::type m_type;
};

template <typename AddressT, typename PortT, typename TypeT>
inline bool operator < (const Endpoint<AddressT,PortT,TypeT>& left,
            const Endpoint<AddressT,PortT,TypeT>& right
           ) noexcept
{
    if (left.type()==right.type())
    {
        if (left.address()==right.address())
        {
            return left.port()<right.port();
        }
        return left.address()<right.address();
    }
    return left.type()<right.type();
}
template <typename AddressT, typename PortT, typename TypeT>
inline bool operator == (const Endpoint<AddressT,PortT,TypeT>& left,
            const Endpoint<AddressT,PortT,TypeT>& right
           ) noexcept
{
    return left.type()==right.type()
            && left.address()==right.address()
            && left.port()<right.port();
}

//! Type wrapper to use with Endpoint template
template <typename Type,typename=void> struct TypeWrapper
{
    using type=Type;
    constexpr static const type defaultValue() noexcept
    {
        return type();
    }
};
template <typename Type> struct TypeWrapper<Type,std::enable_if_t<std::is_arithmetic<Type>::value>>
{
    using type=Type;
    constexpr static const type defaultValue() noexcept
    {
        return static_cast<type>(0);
    }
};
template <typename Type,Type defaultV> struct TypeWithDefault
{
    using type=Type;
    constexpr static const type defaultValue() noexcept
    {
        return defaultV;
    }
};

//! Class with local endpoints
template <typename EndpointT> class WithLocalEndpoint
{
    public:

        template <typename T> inline void setLocalEndpoint(const T& endpoint) noexcept
        {
            m_localEndpoint.copy(endpoint);
        }
        inline void setLocalEndpoint(EndpointT endpoint) noexcept
        {
            m_localEndpoint=std::move(endpoint);
        }

        inline const EndpointT& localEndpoint() const noexcept
        {
            return m_localEndpoint;
        }
        inline EndpointT& localEndpoint() noexcept
        {
            return m_localEndpoint;
        }

    private:

        EndpointT m_localEndpoint;
};

//! Class with remote endpoints
template <typename EndpointT> class WithRemoteEndpoint
{
    public:

        template <typename T> inline void setRemoteEndpoint(const T& endpoint) noexcept
        {
            m_remoteEndpoint.copy(endpoint);
        }
        inline void setRemoteEndpoint(EndpointT endpoint) noexcept
        {
            m_remoteEndpoint=std::move(endpoint);
        }

        inline const EndpointT& remoteEndpoint() const noexcept
        {
            return m_remoteEndpoint;
        }
        inline EndpointT& remoteEndpoint() noexcept
        {
            return m_remoteEndpoint;
        }

    private:

        EndpointT m_remoteEndpoint;
};

}
}

#endif // HATNNETWORKENDPOINT_H
