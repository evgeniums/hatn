/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file serverdmin/clientserver.h
  *
  */

/****************************************************************************/

#ifndef HATNCLIENTSERVER_H
#define HATNCLIENTSERVER_H

#include <hatn/clientserver/config.h>

#include <hatn/common/visibilitymacros.h>

#ifndef HATN_CLIENT_SERVER_EXPORT
#   ifdef BUILD_HATN_CLIENTSERVER
#       define HATN_CLIENT_SERVER_EXPORT HATN_VISIBILITY_EXPORT
#   else
#       define HATN_CLIENT_SERVER_EXPORT HATN_VISIBILITY_IMPORT
#   endif
#endif

#define HATN_CLIENT_SERVER_NAMESPACE_BEGIN namespace hatn { namespace clientserver {
#define HATN_CLIENT_SERVER_NAMESPACE_END }}

HATN_CLIENT_SERVER_NAMESPACE_BEGIN
HATN_CLIENT_SERVER_NAMESPACE_END

#define HATN_CLIENT_SERVER_NAMESPACE hatn::clientserver
#define HATN_CLIENT_SERVER_NS clientserver
#define HATN_CLIENT_SERVER_USING using namespace hatn::clientserver;

#endif // HATNCLIENTSERVER_H
