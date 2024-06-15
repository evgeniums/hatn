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

#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>
#include <hatn/db/plugins/rocksdb/rocksdbhandler.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

namespace dataunit=HATN_DATAUNIT_NAMESPACE;

class HATN_ROCKSDB_SCHEMA_EXPORT RocksdbModel
{
    public:

        RocksdbModel(db::ModelInfo info);

        const db::ModelInfo& info() const noexcept
        {
            return m_modelInfo;
        }

        std::function<Error (RocksdbHandler& handler, const db::Namespace& ns, dataunit::Unit* object)> createObject;

    private:

        db::ModelInfo m_modelInfo;
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBMODEL_H
