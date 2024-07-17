/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/rocksdbmodel.h
  *
  *   RocksDB database model.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBMODEL_H
#define HATNROCKSDBMODEL_H

#include <memory>
#include <functional>

#include <hatn/dataunit/unit.h>

#include <hatn/db/namespace.h>
#include <hatn/db/model.h>
#include <hatn/db/indexquery.h>

#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

namespace dataunit=HATN_DATAUNIT_NAMESPACE;

class RocksdbHandler;

class HATN_ROCKSDB_SCHEMA_EXPORT RocksdbModel
{
    public:

        RocksdbModel(ModelInfo& info);

        const ModelInfo& info() const noexcept
        {
            return m_modelInfo;
        }

        std::function<Error (
            RocksdbHandler& handler,
            const Namespace& ns,
            const dataunit::Unit* object)> createObject;

        std::function<Result<HATN_COMMON_NAMESPACE::SharedPtr<dataunit::Unit>> (
            RocksdbHandler& handler,
            const Namespace& ns,
            const ObjectId& objectId)> readObject;

        std::function<Result<HATN_COMMON_NAMESPACE::SharedPtr<dataunit::Unit>> (
            RocksdbHandler& handler,
            const Namespace& ns,
            const ObjectId& objectId,
            const HATN_COMMON_NAMESPACE::Date& date)> readObjectWithDate;

        std::function<Result<HATN_COMMON_NAMESPACE::pmr::vector<UnitWrapper>> (
            RocksdbHandler& handler,
            IndexQuery& query
            )> find;

        std::function<Error (
            RocksdbHandler& handler,
            const Namespace& ns,
            const ObjectId& objectId)> deleteObject;

        std::function<Error (
            RocksdbHandler& handler,
            const Namespace& ns,
            const ObjectId& objectId,
            const HATN_COMMON_NAMESPACE::Date& date
            )> deleteObjectWithDate;

    private:

        ModelInfo& m_modelInfo;
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBMODEL_H
