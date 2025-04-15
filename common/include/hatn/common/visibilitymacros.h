/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/visibilitymacros.h
  *
  *  Export definitions of Common Library.
  *
  */

/****************************************************************************/

#ifndef HATNVISIBILITYMACROS_H
#define HATNVISIBILITYMACROS_H

#if defined(WIN32)
#   define HATN_VISIBILITY_EXPORT __declspec(dllexport)
#   define HATN_VISIBILITY_IMPORT __declspec(dllimport)
#else
#   define HATN_VISIBILITY_EXPORT __attribute__((visibility("default")))
#   define HATN_VISIBILITY_IMPORT
#endif

#endif // HATNVISIBILITYMACROS_H
