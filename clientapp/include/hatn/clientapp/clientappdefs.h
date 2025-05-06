/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file clientapp/cliantappdefs.h
  *
  */

/****************************************************************************/

#ifndef HATNCLIENTAPPDEFS_H
#define HATNCLIENTAPPDEFS_H

#include <hatn/clientapp/config.h>

#include <hatn/common/visibilitymacros.h>

#ifndef HATN_CLIENTAPP_EXPORT
#   ifdef BUILD_HATN_CLIENTAPP
#       define HATN_CLIENTAPP_EXPORT HATN_VISIBILITY_EXPORT
#   else
#       define HATN_CLIENTAPP_EXPORT HATN_VISIBILITY_IMPORT
#   endif
#endif

#define HATN_CLIENTAPP_NAMESPACE_BEGIN namespace hatn { namespace clientapp {
#define HATN_CLIENTAPP_NAMESPACE_END }}

#define HATN_CLIENTAPP_NAMESPACE hatn::clientapp
#define HATN_CLIENTAPP_NS clientapp
#define HATN_CLIENTAPP_USING using namespace hatn::clientapp;

#define HATN_CLIENTAPP_MOBILE_NAMESPACE_BEGIN namespace hatn { namespace clientapp { namespace mobile {
#define HATN_CLIENTAPP_MOBILE_NAMESPACE_END }}}
#define HATN_CLIENTAPP_MOBILE_NAMESPACE hatn::clientapp::mobile

#endif // HATNCLIENTAPPDEFS_H
