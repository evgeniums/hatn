/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file utility/utility.h
  *
  *  Hatn UTILITY Library contains types and methods to use for base utilitylications.
  *
  */

/****************************************************************************/

#ifndef HATNUTILITY_H
#define HATNUTILITY_H

#include <hatn/utility/config.h>

#include <hatn/common/visibilitymacros.h>

#ifndef HATN_UTILITY_EXPORT
#   ifdef BUILD_HATN_UTILITY
#       define HATN_UTILITY_EXPORT HATN_VISIBILITY_EXPORT
#   else
#       define HATN_UTILITY_EXPORT HATN_VISIBILITY_IMPORT
#   endif
#endif

#define HATN_UTILITY_NAMESPACE_BEGIN namespace hatn { namespace utility {
#define HATN_UTILITY_NAMESPACE_END }}

#define HATN_UTILITY_NAMESPACE hatn::utility
#define HATN_UTILITY_NS utility
#define HATN_UTILITY_USING using namespace hatn::utility;

#endif // HATNUTILITY_H
