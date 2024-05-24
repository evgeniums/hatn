/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/rocksdbschema.h
  *
  *   RocksDB database schema handling header.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBSCHEMA_H
#define HATNROCKSDBSCHEMA_H

#include <map>
#include <memory>
#include <functional>

#include <hatn/common/singleton.h>

#include <hatn/dataunit/unit.h>

#include <hatn/db/namespace.h>
#include <hatn/db/schema.h>

#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>
#include <hatn/db/plugins/rocksdb/rocksdbhandler.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

namespace dataunit=HATN_DATAUNIT_NAMESPACE;

class HATN_ROCKSDB_SCHEMA_EXPORT RocksdbModel
{
    public:

        RocksdbModel();

        CUID_TYPE cuid() const noexcept
        {
            return m_cuid;
        }

        std::function<Error (RocksdbHandler& handler, const db::Namespace& ns, const dataunit::Unit& object)> createObject;

    private:

        CUID_TYPE m_cuid;
};

class HATN_ROCKSDB_SCHEMA_EXPORT RocksdbSchema
{
    public:

        RocksdbSchema()=default;
        RocksdbSchema(const RocksdbSchema&)=delete;
        RocksdbSchema& operator=(const RocksdbSchema&)=delete;
        RocksdbSchema(RocksdbSchema&&)=default;
        RocksdbSchema& operator=(RocksdbSchema&&)=default;

        void addModel(std::shared_ptr<RocksdbModel> model);

        Result<std::shared_ptr<RocksdbModel>> model(CUID_TYPE) const noexcept;

    private:

        std::map<CUID_TYPE,std::shared_ptr<RocksdbModel>> m_models;
};

class HATN_ROCKSDB_SCHEMA_EXPORT RocksdbSchemas : public common::Singleton
{
    HATN_SINGLETON_DECLARE()

    public:

        static RocksdbSchemas& instance();
        static void free() noexcept;

        template <typename ...Models>
        static Error Register(const db::Schema<Models...>& schema);

    private:

        RocksdbSchemas();

        std::map<std::string,std::unique_ptr<RocksdbSchema>> m_schemas;
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBSCHEMA_H
