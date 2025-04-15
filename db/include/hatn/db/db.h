/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/

/** @file db/db.h
  *
  *  Hatn Database Library implements types and routines to work with database using dataunit objects.
  *
  */

/****************************************************************************/

#ifndef HATNDB_H
#define HATNDB_H

#include <hatn/db/config.h>

#include <hatn/dataunit/dataunit.h>
#include <hatn/validator/config.hpp>

#include <hatn/common/visibilitymacros.h>

#ifndef HATN_DB_EXPORT
#   ifdef BUILD_HATN_DB
#       define HATN_DB_EXPORT HATN_VISIBILITY_EXPORT
#   else
#       define HATN_DB_EXPORT HATN_VISIBILITY_IMPORT
#   endif
#endif

#define HATN_DB_NAMESPACE_BEGIN namespace hatn { namespace db {
#define HATN_DB_NAMESPACE_END }}

#define HATN_DB_NAMESPACE hatn::db
#define HATN_DB_NS base
#define HATN_DB_USING using namespace hatn::db;

HATN_DB_NAMESPACE_BEGIN
constexpr const char* DB_MODULE_NAME="db";
HATN_DB_NAMESPACE_END

namespace vld=HATN_VALIDATOR_NAMESPACE;
namespace du=HATN_DATAUNIT_NAMESPACE;

#endif // HATNDB_H
