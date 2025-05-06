/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file api/api.h
  *
  *  Hatn API Library contains types and methods to use for client-server APIs.
  *
  */

/****************************************************************************/

#ifndef HATNAPI_H
#define HATNAPI_H

#include <hatn/api/config.h>

#include <hatn/common/visibilitymacros.h>

#ifndef HATN_API_EXPORT
#   ifdef BUILD_HATN_API
#       define HATN_API_EXPORT HATN_VISIBILITY_EXPORT
#   else
#       define HATN_API_EXPORT HATN_VISIBILITY_IMPORT
#   endif
#endif

#define HATN_API_NAMESPACE_BEGIN namespace hatn { namespace api {
#define HATN_API_NAMESPACE_END }}

#define HATN_API_NAMESPACE hatn::api
#define HATN_API_NS api
#define HATN_API_USING using namespace hatn::api;

#define HATN_API_SERVER_NAMESPACE hatn::api::server

#define HATN_API_CLIENT_BRIDGE_NAMESPACE_BEGIN namespace hatn { namespace api { namespace client { namespace bridge {
#define HATN_API_CLIENT_BRIDGE_NAMESPACE_END }}}}
#define HATN_API_CLIENT_BRIDGE_NAMESPACE hatn::api::client::bridge

#define HATN_API_MOBILECLIENT_NAMESPACE_BEGIN namespace hatn { namespace api { namespace mobileclient {
#define HATN_API_MOBILECLIENT_NAMESPACE_END }}}
#define HATN_API_MOBILECLIENT_NAMESPACE hatn::api::mobileclient

#endif // HATNAPI_H
