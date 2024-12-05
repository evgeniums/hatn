/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file network/network.h
  *
  *  Hatn Network Library.
  *
  */

/****************************************************************************/

#ifndef HATNNETWORK_H
#define HATNNETWORK_H

#include <hatn/common/logger.h>
#include <hatn/network/config.h>

#define HATN_NETWORK_NAMESPACE_BEGIN namespace hatn { namespace network {
#define HATN_NETWORK_NAMESPACE_END }}

#define HATN_NETWORK_NAMESPACE hatn::network
#define HATN_NETWORK_USING using namespace hatn::network;
#define HATN_NETWORK_NS network

HATN_NETWORK_NAMESPACE_BEGIN

// define export symbols for windows platform
#ifdef WIN32
#    ifndef HATN_NETWORK_EXPORT
#        ifdef BUILD_HATN_NETWORK
#            define HATN_NETWORK_EXPORT __declspec(dllexport)
#        else
#            define HATN_NETWORK_EXPORT __declspec(dllimport)
#        endif
#    endif
#else
#    define HATN_NETWORK_EXPORT
#endif

HATN_NETWORK_NAMESPACE_END

DECLARE_LOG_MODULE_EXPORT(network,HATN_NETWORK_EXPORT)

#endif // HATNNETWORK_H
