/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file serverapp/serverappdefs.h
  *
  */

/****************************************************************************/

#ifndef HATNSERVERAPPDEFS_H
#define HATNSERVERAPPDEFS_H

#include <hatn/app/config.h>

#include <hatn/common/visibilitymacros.h>

#include <hatn/base/base.h>
#include <hatn/api/api.h>
#include <hatn/clientserver/clientserver.h>

#ifndef HATN_SERVERAPP_EXPORT
#   ifdef BUILD_HATN_SERVERAPP
#       define HATN_SERVERAPP_EXPORT HATN_VISIBILITY_EXPORT
#   else
#       define HATN_SERVERAPP_EXPORT HATN_VISIBILITY_IMPORT
#   endif
#endif

#define HATN_SERVERAPP_NAMESPACE_BEGIN namespace hatn { namespace serverapp {
#define HATN_SERVERAPP_NAMESPACE_END }}

#define HATN_SERVERAPP_NAMESPACE hatn::serverapp
#define HATN_SERVERAPP_NS serverapp
#define HATN_SERVERAPP_USING using namespace hatn::serverapp;

HATN_SERVERAPP_NAMESPACE_BEGIN
HATN_CLIENT_SERVER_USING
namespace api=HATN_API_NAMESPACE;
namespace serverapi=HATN_API_SERVER_NAMESPACE;
namespace base=HATN_BASE_NAMESPACE;
HATN_SERVERAPP_NAMESPACE_END

#endif // HATNSERVERAPPDEFS_H
