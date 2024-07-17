/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/rocksdbclient.h
  *
  *   RocksDB database client.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBCLIENT_H
#define HATNROCKSDBCLIENT_H

#include <memory>

#include <hatn/db/client.h>

#include <hatn/db/plugins/rocksdb/rocksdbdriver.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

class RocksdbClient_p;

class HATN_ROCKSDB_EXPORT RocksdbClient : public Client
{
    public:

        RocksdbClient(
            common::STR_ID_TYPE id=common::STR_ID_TYPE()
        );

        ~RocksdbClient();

        void invokeCloseDb(Error& ec);
        void invokeOpenDb(const ClientConfig& config, Error& ec, base::config_object::LogRecords& records, bool createIfMissing=false);

    protected:

        Error doCreateDb(const ClientConfig& config, base::config_object::LogRecords& records) override;
        Error doDestroyDb(const ClientConfig& config, base::config_object::LogRecords& records) override;

        Error doAddSchema(std::shared_ptr<DbSchema> schema) override;
        Result<std::shared_ptr<DbSchema>> doFindSchema(const common::lib::string_view& schemaName) const override;
        Result<std::vector<std::shared_ptr<DbSchema>>> doListSchemas() const override;
        Error doCheckSchemas() override;
        Error doMigrateSchemas() override;

        void doOpenDb(const ClientConfig& config, Error& ec, base::config_object::LogRecords& records) override;
        void doCloseDb(Error& ec) override;

        Error doAddDatePartitions(const std::vector<ModelInfo>& models, const std::set<common::DateRange>& dateRanges) override;
        Error doDeleteDatePartitions(const std::vector<ModelInfo>& models, const std::set<common::DateRange>& dateRanges) override;

        Error doCreate(const db::Namespace& ns, const ModelInfo& model, dataunit::Unit* object) override;

        Result<common::SharedPtr<dataunit::Unit>> doRead(const Namespace& ns, const ModelInfo& model, const ObjectId& id) override;
        Result<common::SharedPtr<dataunit::Unit>> doRead(const Namespace& ns, const ModelInfo& model, const ObjectId& id, const common::Date& date) override;

        Result<HATN_COMMON_NAMESPACE::pmr::vector<UnitWrapper>> doFind(
            const Namespace& ns,
            const ModelInfo& model,
            IndexQuery& query
        ) override;

        Error doDeleteObject(const Namespace& ns,
                             const ModelInfo& model,
                             const ObjectId& id,
                             const common::Date& date) override;

        Error doDeleteObject(const Namespace& ns,
                             const ModelInfo& model,
                             const ObjectId& id) override;

        Error doDeleteMany(
            const Namespace& ns,
            const ModelInfo& model,
            IndexQuery& query
            ) override;

        Error doTransaction(const TransactionFn& fn) override;

    private:

        std::unique_ptr<RocksdbClient_p> d;
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBCLIENT_H
