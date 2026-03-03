/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/common.h
  *
  *  Export definitions of Common Library.
  *
  */

/****************************************************************************/

#ifndef HATNCOMMON_H
#define HATNCOMMON_H

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

#ifdef _MSC_VER
#define NOMINMAX
#endif

#include <cassert>
#include <hatn/common/visibilitymacros.h>

#ifndef HATN_COMMON_EXPORT
#   ifdef BUILD_HATN_COMMON
#       define HATN_COMMON_EXPORT HATN_VISIBILITY_EXPORT
#   else
#       define HATN_COMMON_EXPORT HATN_VISIBILITY_IMPORT
#   endif
#endif

#include <hatn/common/config.h>
#include <hatn/common/ignorewarnings.h>

#define HATN_COMMON_NAMESPACE_BEGIN namespace hatn { namespace common {
#define HATN_COMMON_NAMESPACE_END }}

HATN_COMMON_NAMESPACE_BEGIN
struct shared_pointer_tag{};
HATN_COMMON_NAMESPACE_END

#define HATN_COMMON_NAMESPACE hatn::common
#define HATN_COMMON_USING using namespace hatn::common;
#define HATN_COMMON_NS common
#define HATN_COMMON hatn::common

#define HATN_NAMESPACE hatn
#define HATN_NAMESPACE_BEGIN namespace hatn {
#define HATN_NAMESPACE_END }
#define HATN_USING using namespace hatn;

#define HATN_TEST_NAMESPACE hatn::test
#define HATN_TEST_NAMESPACE_BEGIN namespace hatn { namespace test {
#define HATN_TEST_NAMESPACE_END }}

#define HATN_TEST_USING using namespace hatn::test;

#define HATN_NO_EXPORT

#define Assert(condition,message) assert((condition) && message); if (!(condition)) throw std::runtime_error(message);
#define AssertThrow(condition,message) if (!(condition)) {throw std::runtime_error(message);}
#define AssertThrowEx(condition,message,ex) if (!(condition)) {throw ex(message);}

#endif // HATNCOMMON_H
