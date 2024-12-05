/*
   Copyright (c) 2019 - current, Evgeny Sidorov (esid1976@gmail.com), All rights reserved
    
    
  */

/****************************************************************************/
/*
    
*/
/** \file resolverinterface.сpp
  *
  *     Resolver interface to use in environment.
  *
  */

#include <hatn/network/resolverinterface.h>

namespace hatn {
namespace network {

DCS_CUID_INIT(TcpResolverInterface)

TcpResolverInterface::~TcpResolverInterface()
{}

DCS_CUID_INIT(UdpResolverInterface)

UdpResolverInterface::~UdpResolverInterface()
{}

//---------------------------------------------------------------
    } // namespace network
} // namespace hatn
