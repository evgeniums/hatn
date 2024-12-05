/*
   Copyright (c) 2019 - current, Evgeny Sidorov (esid1976@gmail.com), All rights reserved
    
    
  */

/****************************************************************************/
/*
    
*/
/** \file network/ipendpoint.h
  *
  *  Network IP endpoint
  *
  */

/****************************************************************************/

#ifndef HATNNETWORKIPENDPOINT_H
#define HATNNETWORKIPENDPOINT_H

#include <hatn/network/network.h>
#include <hatn/network/endpoint.h>

namespace hatn {
namespace network {

//! Version of IP protocol
enum class IpVersion : int
{
    V4=1,
    V6=2,
    ALL=V4|V6    
};
inline const char* IpVersionStr(IpVersion ipVersion) noexcept
{
    if (ipVersion==IpVersion::V4)
    {
        return "IPv4";
    }
    else if (ipVersion==IpVersion::V6)
    {
        return "IPv6";
    }
    return "IPv4 & IPv6";
}
inline bool IpVersionHasV4(IpVersion ipVersion) noexcept
{
    return static_cast<int>(ipVersion)&static_cast<int>(IpVersion::V4);
}
inline bool IpVersionHasV6(IpVersion ipVersion) noexcept
{
    return static_cast<int>(ipVersion)&static_cast<int>(IpVersion::V6);
}

//! Types of IP protocols
enum class IpProtocol : int
{
    TCP=0,
    UDP=1,
    ANY=2
};

//! IP endpoint
template <typename AddressT> using IpEndpoint=Endpoint<
                                                TypeWrapper<AddressT>,
                                                TypeWrapper<uint16_t>,
                                                TypeWithDefault<IpProtocol,IpProtocol::ANY>
                                            >;

}
}

#endif // HATNNETWORKIPENDPOINT_H
