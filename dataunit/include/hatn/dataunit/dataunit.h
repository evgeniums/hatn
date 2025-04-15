/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/dataunit.h
  *
  *  Hatn DataUnit Library for units definition and serialization/deserialization
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNIT_H
#define HATNDATAUNIT_H

#include <hatn/common/logger.h>
#include <hatn/dataunit/config.h>

#include <hatn/common/visibilitymacros.h>

#ifndef HATN_DATAUNIT_EXPORT
#   ifdef BUILD_HATN_DATAUNIT
#       define HATN_DATAUNIT_EXPORT HATN_VISIBILITY_EXPORT
#   else
#       define HATN_DATAUNIT_EXPORT HATN_VISIBILITY_IMPORT
#   endif
#endif

#define HATN_DATAUNIT_NAMESPACE_BEGIN namespace hatn { namespace du {
#define HATN_DATAUNIT_NAMESPACE_END }}
#define HATN_DATAUNIT_NAMESPACE hatn::du
#define HATN_DATAUNIT_NS du
#define HATN_DATAUNIT_USING using namespace hatn::du;

HATN_DATAUNIT_NAMESPACE_BEGIN
HATN_DATAUNIT_NAMESPACE_END

HATN_NAMESPACE_BEGIN
namespace dataunit=du;
HATN_NAMESPACE_END

#define HATN_DATAUNIT_META_NAMESPACE_BEGIN namespace hatn { namespace du { namespace m {
#define HATN_DATAUNIT_META_NAMESPACE hatn::du::meta
#define HATN_DATAUNIT_META_NAMESPACE_END }}}

HATN_DATAUNIT_META_NAMESPACE_BEGIN
HATN_DATAUNIT_META_NAMESPACE_END

HATN_DATAUNIT_NAMESPACE_BEGIN
namespace meta=m;
HATN_DATAUNIT_NAMESPACE_END

DECLARE_LOG_MODULE_EXPORT(dataunit,HATN_DATAUNIT_EXPORT)

#endif // HATNDATAUNIT_H
