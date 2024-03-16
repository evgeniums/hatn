/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file base/base.h
  *
  *  Hatn Base Library contains common types and helper functions that
  *  are not part of hatncommon library because hatnbase depends on hatnvalidator and hatndataunit.
  *
  */

/****************************************************************************/

#ifndef HATNBASE_H
#define HATNBASE_H

#include <hatn/base/config.h>

// define export symbols for windows platform
#ifdef WIN32
#    ifndef HATN_BASE_EXPORT
#        ifdef BUILD_HATN_BASE
#            define HATN_BASE_EXPORT __declspec(dllexport)
#        else
#            define HATN_BASE_EXPORT __declspec(dllimport)
#        endif
#    endif
#else
#    define HATN_BASE_EXPORT
#endif

#define HATN_BASE_NAMESPACE_BEGIN namespace hatn { namespace base {
#define HATN_BASE_NAMESPACE_END }}

#define HATN_BASE_NAMESPACE hatn::base
#define HATN_BASE_NS base
#define HATN_BASE_USING using namespace hatn::base;

#endif // HATNBASE_H
