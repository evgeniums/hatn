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

#include <hatn/common/singleton.h>
#include <hatn/common/stdwrappers.h>
#include <hatn/common/pmr/allocatorfactory.h>

#include <hatn/db/model.h>
#include <hatn/db/schema.h>

#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>
#include <hatn/db/plugins/rocksdb/rocksdbmodel.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

namespace dataunit=HATN_DATAUNIT_NAMESPACE;

class HATN_ROCKSDB_SCHEMA_EXPORT RocksdbSchema
{
    public:

        RocksdbSchema(std::shared_ptr<Schema> dbSchema) : m_dbSchema(std::move(dbSchema))
        {}

        RocksdbSchema(const RocksdbSchema&)=delete;
        RocksdbSchema& operator=(const RocksdbSchema&)=delete;
        RocksdbSchema(RocksdbSchema&&)=default;
        RocksdbSchema& operator=(RocksdbSchema&&)=default;

        void addModel(std::shared_ptr<RocksdbModel> model);

        std::shared_ptr<RocksdbModel> findModel(const ModelInfo &info) const
        {
            auto it=m_models.find(info.modelId());
            if (it==m_models.end())
            {
                return std::shared_ptr<RocksdbModel>{};
            }
            return it->second;
        }

        std::shared_ptr<RocksdbModel> findModel(const std::shared_ptr<ModelInfo>& info) const
        {
            return findModel(*info);
        }

        std::shared_ptr<Schema> dbSchema() const noexcept
        {
            return m_dbSchema;
        }

    private:

        std::map<uint32_t,std::shared_ptr<RocksdbModel>> m_models;
        std::shared_ptr<Schema> m_dbSchema;
};

class HATN_ROCKSDB_SCHEMA_EXPORT RocksdbSchemas : public common::Singleton
{
    HATN_SINGLETON_DECLARE()

    public:

        static RocksdbSchemas& instance();
        static void free() noexcept;

        void registerSchema(std::shared_ptr<Schema> schema);
        void unregisterSchema(const std::string& name);

        std::shared_ptr<RocksdbSchema> schema(lib::string_view name) const;

    private:

        RocksdbSchemas();

        std::map<std::string,std::shared_ptr<RocksdbSchema>,std::less<>> m_schemas;
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBSCHEMA_H
