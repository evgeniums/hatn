/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/rocksdbschemadef.h
  *
  *   Macros for RocksDB database schema lib.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBSCHEMADEF_H
#define HATNROCKSDBSCHEMADEF_H

#include <hatn/common/pmr/allocatorfactory.h>
#include <hatn/common/allocatoronstack.h>
#include <hatn/common/bytearray.h>

#include <hatn/db/db.h>

#include <hatn/dataunit/dataunit.h>

#include <hatn/db/plugins/rocksdb/rocksdbdriver.h>

#include <hatn/common/visibilitymacros.h>

#ifndef HATN_ROCKSDB_SCHEMA_EXPORT
#   ifdef BUILD_HATN_ROCKSDB_SCHEMA
#       define HATN_ROCKSDB_SCHEMA_EXPORT HATN_VISIBILITY_EXPORT
#   else
#       define HATN_ROCKSDB_SCHEMA_EXPORT HATN_VISIBILITY_IMPORT
#   endif
#endif

HATN_ROCKSDB_NAMESPACE_BEGIN

    namespace common=HATN_COMMON_NAMESPACE;
namespace db=HATN_DB_NAMESPACE;

using AllocatorFactory=common::pmr::AllocatorFactory;

constexpr const size_t PreallocatedKeySize=256;
using KeyBuf=common::StringOnStackT<PreallocatedKeySize>;


constexpr static const char SeparatorCharC=static_cast<char>(0x00);
constexpr static const char* SeparatorChar=&SeparatorCharC;
constexpr lib::string_view SeparatorCharStr{SeparatorChar,1};

constexpr static const char SeparatorCharPlusC=static_cast<char>(0x01);
constexpr static const char* SeparatorCharPlus=&SeparatorCharPlusC;
constexpr lib::string_view SeparatorCharPlusStr{SeparatorCharPlus,1};

constexpr static const char DbNullCharC=static_cast<char>(0x01);
constexpr static const char* DbNullChar=&DbNullCharC;
constexpr lib::string_view DbNullCharStr{DbNullChar,1};

constexpr static const char EmptyCharC=static_cast<char>(0x02);
constexpr static const char* EmptyChar=&EmptyCharC;
constexpr lib::string_view EmptyCharStr{EmptyChar,1};

//! @todo Implement UTF-8 indexes using separate column families with custom UTF-8 comparators
//! @todo Implement case insensitive indexes

//! @todo Validate that topics do not start with internal prefix
constexpr static const char InternalPrefixC=static_cast<char>(0xFF);

constexpr static const char NullCharC=static_cast<char>(0x00);
constexpr static const char SpaceCharC=static_cast<char>(32);
constexpr static const char BackSlashCharC=static_cast<char>(92);
constexpr static const char AsciiMaxPrintableCharC=static_cast<char>(126);

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBSCHEMADEF_H
