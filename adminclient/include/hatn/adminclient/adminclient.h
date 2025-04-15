/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file serverdmin/adminclient.h
  *
  */

/****************************************************************************/

#ifndef HATNADMINCLIENT_H
#define HATNADMINCLIENT_H

#include <hatn/app/config.h>

#include <hatn/common/visibilitymacros.h>

#ifndef HATN_ADMIN_CLIENT_EXPORT
#   ifdef BUILD_HATN_ADMIN_CLIENT
#       define HATN_ADMIN_CLIENT_EXPORT HATN_VISIBILITY_EXPORT
#   else
#       define HATN_ADMIN_CLIENT_EXPORT HATN_VISIBILITY_IMPORT
#   endif
#endif

#define HATN_ADMIN_CLIENT_NAMESPACE_BEGIN namespace hatn { namespace adminclient {
#define HATN_ADMIN_CLIENT_NAMESPACE_END }}

HATN_ADMIN_CLIENT_NAMESPACE_BEGIN
HATN_ADMIN_CLIENT_NAMESPACE_END

#define HATN_ADMIN_CLIENT_NAMESPACE hatn::adminclient
#define HATN_ADMIN_CLIENT_NS adminclient
#define HATN_ADMIN_CLIENT_USING using namespace hatn::adminclient;

#endif // HATNADMINCLIENT_H
