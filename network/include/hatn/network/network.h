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

#include <hatn/common/visibilitymacros.h>

#ifndef HATN_NETWORK_EXPORT
#   ifdef BUILD_HATN_NETWORK
#       define HATN_NETWORK_EXPORT HATN_VISIBILITY_EXPORT
#   else
#       define HATN_NETWORK_EXPORT HATN_VISIBILITY_IMPORT
#   endif
#endif

#define HATN_NETWORK_NAMESPACE_BEGIN namespace hatn { namespace network {
#define HATN_NETWORK_NAMESPACE_END }}

#define HATN_NETWORK_NAMESPACE hatn::network
#define HATN_NETWORK_USING using namespace hatn::network;
#define HATN_NETWORK_NS network

DECLARE_LOG_MODULE_EXPORT(network,HATN_NETWORK_EXPORT)

#endif // HATNNETWORK_H
