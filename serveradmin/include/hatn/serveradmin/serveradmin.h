/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file serverdmin/serveradmin.h
  *
  */

/****************************************************************************/

#ifndef HATNSERVERADMIN_H
#define HATNSERVERADMIN_H

#include <hatn/app/config.h>

// define export symbols for windows platform
#ifdef WIN32
#    ifndef HATN_SERVER_ADMIN_EXPORT
#        ifdef BUILD_HATN_SERVER_ADMIN
#            define HATN_SERVER_ADMIN_EXPORT __declspec(dllexport)
#        else
#            define HATN_SERVER_ADMIN_EXPORT __declspec(dllimport)
#        endif
#    endif
#else
#    define HATN_SERVER_ADMIN_EXPORT __attribute__((visibility("default")))
#endif

#define HATN_SERVER_ADMIN_NAMESPACE_BEGIN namespace hatn { namespace serveradmin {
#define HATN_SERVER_ADMIN_NAMESPACE_END }}

#define HATN_SERVER_ADMIN_NAMESPACE hatn::serveradmin
#define HATN_SERVER_ADMIN_NS serverapp
#define HATN_SERVER_ADMIN_USING using namespace hatn::serveradmin;

#endif // HATNSERVERADMIN_H
