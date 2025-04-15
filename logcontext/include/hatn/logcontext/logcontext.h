/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file logcontext/logcontext.h
  *
  *  Hatn logcontext Library contains types and helpers to log stack contexts.
  *
  */

/****************************************************************************/

#ifndef HATNLOGCONTEXTDEF_H
#define HATNLOGCONTEXTDEF_H

#include <hatn/logcontext/config.h>

#include <hatn/common/visibilitymacros.h>

#ifndef HATN_LOGCONTEXT_EXPORT
#   ifdef BUILD_HATN_LOGCONTEXT
#       define HATN_LOGCONTEXT_EXPORT HATN_VISIBILITY_EXPORT
#   else
#       define HATN_LOGCONTEXT_EXPORT HATN_VISIBILITY_IMPORT
#   endif
#endif

#define HATN_LOGCONTEXT_NAMESPACE_BEGIN namespace hatn { namespace logcontext {
#define HATN_LOGCONTEXT_NAMESPACE_END }}

#define HATN_LOGCONTEXT_NAMESPACE hatn::logcontext
#define HATN_LOGCONTEXT_NS logcontext
#define HATN_LOGCONTEXT_USING using namespace hatn::logcontext;

#endif // HATNLOGCONTEXTDEF_H
