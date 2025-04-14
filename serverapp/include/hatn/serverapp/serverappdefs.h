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

// define export symbols for windows platform
#ifdef WIN32
#    ifndef HATN_SERVERAPP_EXPORT
#        ifdef BUILD_HATN_SERVERAPP
#            define HATN_SERVERAPP_EXPORT __declspec(dllexport)
#        else
#            define HATN_SERVERAPP_EXPORT __declspec(dllimport)
#        endif
#    endif
#else
#    define HATN_SERVERAPP_EXPORT
#endif

#define HATN_SERVERAPP_NAMESPACE_BEGIN namespace hatn { namespace serverapp {
#define HATN_SERVERAPP_NAMESPACE_END }}

#define HATN_SERVERAPP_NAMESPACE hatn::serverapp
#define HATN_SERVERAPP_NS serverapp
#define HATN_SERVERAPP_USING using namespace hatn::serverapp;

#endif // HATNSERVERAPPDEFS_H
