/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file serverdmin/adminserver.h
  *
  */

/****************************************************************************/

#ifndef HATNADMINSERVER_H
#define HATNADMINSERVER_H

#include <hatn/app/config.h>

#include <hatn/clientserver/clientserver.h>
#include <hatn/serverapp/serverappdefs.h>
#include <hatn/adminclient/adminclient.h>

#include <hatn/common/visibilitymacros.h>

#ifndef HATN_ADMIN_SERVER_EXPORT
#   ifdef BUILD_HATN_ADMIN_SERVER
#       define HATN_ADMIN_SERVER_EXPORT HATN_VISIBILITY_EXPORT
#   else
#       define HATN_ADMIN_SERVER_EXPORT HATN_VISIBILITY_IMPORT
#   endif
#endif

#define HATN_ADMIN_SERVER_NAMESPACE_BEGIN namespace hatn { namespace adminserver {
#define HATN_ADMIN_SERVER_NAMESPACE_END }}

#define HATN_ADMIN_SERVER_NAMESPACE hatn::adminserver
#define HATN_ADMIN_SERVER_NS serverapp
#define HATN_ADMIN_SERVER_USING using namespace hatn::adminserver;

HATN_ADMIN_SERVER_NAMESPACE_BEGIN
HATN_CLIENT_SERVER_USING
HATN_SERVERAPP_USING
HATN_ADMIN_CLIENT_USING
HATN_ADMIN_SERVER_NAMESPACE_END

HATN_ADMIN_CLIENT_USING

#endif // HATNADMINSERVER_H
