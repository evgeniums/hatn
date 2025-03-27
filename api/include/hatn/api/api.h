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

// define export symbols for windows platform
#ifdef WIN32
#    ifndef HATN_API_EXPORT
#        ifdef BUILD_HATN_API
#            define HATN_API_EXPORT __declspec(dllexport)
#        else
#            define HATN_API_EXPORT __declspec(dllimport)
#        endif
#    endif
#else
#    define HATN_API_EXPORT
#endif

#define HATN_API_NAMESPACE_BEGIN namespace hatn { namespace api {
#define HATN_API_NAMESPACE_END }}

#define HATN_API_NAMESPACE hatn::api
#define HATN_API_NS api
#define HATN_API_USING using namespace hatn::api;

#endif // HATNAPI_H
