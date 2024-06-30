/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/saveuniquekey.h
  *
  *   RocksDB merge operator for unique keys.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBUNIQUEKEY_H
#define HATNROCKSDBUNIQUEKEY_H

#include <rocksdb/db.h>
#include <rocksdb/merge_operator.h>

#include <hatn/db/namespace.h>
#include <hatn/db/objectid.h>
#include <hatn/db/dberror.h>

#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

class HATN_ROCKSDB_SCHEMA_EXPORT RocksdbOpError
{
    public:

        static const Error& ec();

        static void setEc(const Error& ec);

        static void resetEc();
};

class HATN_ROCKSDB_SCHEMA_EXPORT SaveUniqueKey : public ROCKSDB_NAMESPACE::AssociativeMergeOperator
{
    public:

        virtual bool Merge(
            const ROCKSDB_NAMESPACE::Slice& key,
            const ROCKSDB_NAMESPACE::Slice* existing_value,
            const ROCKSDB_NAMESPACE::Slice& value,
            std::string* new_value,
            ROCKSDB_NAMESPACE::Logger*) const override;

        virtual const char* Name() const override
        {
            return "SaveUniqueKey";
        }
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBUNIQUEKEY_H
