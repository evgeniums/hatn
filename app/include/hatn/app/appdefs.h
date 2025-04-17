/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file app/app.h
  *
  *  Hatn APP Library contains types and methods to use for base applications.
  *
  */

/****************************************************************************/

#ifndef HATNAPP_H
#define HATNAPP_H

#include <hatn/app/config.h>

#include <hatn/common/visibilitymacros.h>

#ifndef HATN_APP_EXPORT
#   ifdef BUILD_HATN_APP
#       define HATN_APP_EXPORT HATN_VISIBILITY_EXPORT
#   else
#       define HATN_APP_EXPORT HATN_VISIBILITY_IMPORT
#   endif
#endif

#define HATN_APP_NAMESPACE_BEGIN namespace hatn { namespace app {
#define HATN_APP_NAMESPACE_END }}

#define HATN_APP_NAMESPACE hatn::app
#define HATN_APP_NS app
#define HATN_APP_USING using namespace hatn::app;

#endif // HATNAPP_H
