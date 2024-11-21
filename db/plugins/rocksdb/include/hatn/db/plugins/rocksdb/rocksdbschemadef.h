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

#ifdef _WIN32
#    ifndef HATN_ROCKSDB_SCHEMA_EXPORT
#        ifdef BUILD_HATN_ROCKSDB_SCHEMA
#            define HATN_ROCKSDB_SCHEMA_EXPORT __declspec(dllexport)
#        else
#            define HATN_ROCKSDB_SCHEMA_EXPORT __declspec(dllimport)
#        endif
#    endif
#else
#    define HATN_ROCKSDB_SCHEMA_EXPORT
#endif

HATN_ROCKSDB_NAMESPACE_BEGIN

namespace common=HATN_COMMON_NAMESPACE;
namespace db=HATN_DB_NAMESPACE;

using AllocatorFactory=common::pmr::AllocatorFactory;

constexpr const size_t PreallocatedKeySize=256;
using KeyBuf=common::StringOnStackT<PreallocatedKeySize>;

constexpr static const char SeparatorCharC=0x0;
constexpr static const char* SeparatorChar=&SeparatorCharC;
constexpr lib::string_view SeparatorCharStr{SeparatorChar,1};

constexpr static const char SeparatorCharPlusC=0x1;
constexpr static const char* SeparatorCharPlus=&SeparatorCharPlusC;
constexpr lib::string_view SeparatorCharPlusStr{SeparatorCharPlus,1};

constexpr static const char EmptyCharC=0x2;
constexpr static const char* EmptyChar=&EmptyCharC;
constexpr lib::string_view EmptyCharStr{EmptyChar,1};

constexpr static const char SpaceCharC=32;
constexpr static const char BackSlashCharC=92;

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBSCHEMADEF_H
