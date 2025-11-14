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

        /**
         * @todo Implement in rocksdb:
         *
         * 1. Replication.
         * 2. Migration.
         * 3. UTF-8 support.
         *
         */

        RocksdbClient(
            const lib::string_view& id=lib::string_view{}
        );

        ~RocksdbClient();

        void invokeCloseDb(Error& ec);
        void invokeOpenDb(const ClientConfig& config, Error& ec, base::config_object::LogRecords& records, bool createIfMissing=false);

    protected:

        std::shared_ptr<ClientEnvironment> doCloneEnvironment() override;

        Error doCreateDb(const ClientConfig& config, base::config_object::LogRecords& records) override;
        Error doDestroyDb(const ClientConfig& config, base::config_object::LogRecords& records) override;

        Error doSetSchema(std::shared_ptr<Schema> schema) override;
        Result<std::shared_ptr<Schema>> doGetSchema() const override;
        Error doCheckSchema() override;
        Error doMigrateSchema() override;

        void doOpenDb(const ClientConfig& config, Error& ec, base::config_object::LogRecords& records, bool creatIfNotExists) override;
        void doCloseDb(Error& ec) override;

        Error doAddDatePartitions(const std::vector<ModelInfo>& models, const std::set<common::DateRange>& dateRanges) override;
        Error doDeleteDatePartitions(const std::vector<ModelInfo>& models, const std::set<common::DateRange>& dateRanges) override;

        Result<std::set<common::DateRange>> doListDatePartitions() override;

        Error doDeleteTopic(Topic topic) override;

        Error doCreate(Topic topic, const ModelInfo& model, const dataunit::Unit* object, Transaction* tx) override;

        Result<DbObject> doRead(Topic topic,
                                                         const ModelInfo& model,
                                                         const ObjectId& id,
                                                         Transaction* tx,
                                                         bool forUpdate
                                                         ) override;
        Result<DbObject> doRead(Topic topic,
                                                         const ModelInfo& model,
                                                         const ObjectId& id,
                                                         const common::Date& date,
                                                         Transaction* tx,
                                                         bool forUpdate
                                                         ) override;

        Result<HATN_COMMON_NAMESPACE::pmr::vector<DbObject>> doFind(
            const ModelInfo& model,
            const ModelIndexQuery& query
        ) override;

        Error doFindCb(
            const ModelInfo& model,
            const ModelIndexQuery& query,
            const FindCb& cb,
            Transaction* tx,
            bool forUpdate
        ) override;

        virtual Result<size_t> doCount(
            const ModelInfo& model,
            const ModelIndexQuery& query
        ) override;

        virtual Result<size_t> doCount(
            const ModelInfo& model,
            Topic topic
            ) override;

        virtual Result<size_t> doCount(
            const ModelInfo& model,
            const common::Date& date,
            Topic topic
            ) override;

        Error doDeleteObject(Topic topic,
                             const ModelInfo& model,
                             const ObjectId& id,
                             const common::Date& date,
                             Transaction* tx) override;

        Error doDeleteObject(Topic topic,
                             const ModelInfo& model,
                             const ObjectId& id,
                             Transaction* tx) override;

        Result<size_t> doDeleteMany(
            const ModelInfo& model,
            const ModelIndexQuery& query,
            Transaction* tx
            ) override;

        Result<size_t> doDeleteManyBulk(
            const ModelInfo& model,
            const ModelIndexQuery& query,
            Transaction* tx
        ) override;

        Error doTransaction(const TransactionFn& fn) override;

        Error doUpdateObject(Topic topic,
                       const ModelInfo& model,
                       const ObjectId& id,
                       const update::Request& request,                       
                       const common::Date& date,
                       Transaction* tx) override;

        Error doUpdateObject(Topic topic,
                       const ModelInfo& model,
                       const ObjectId& id,
                       const update::Request& request,
                       Transaction* tx) override;

        Result<DbObject> doReadUpdate(Topic topic,
                                                                       const ModelInfo& model,
                                                                       const ObjectId &id,
                                                                       const update::Request& request,
                                                                       const common::Date& date,
                                                                       update::ModifyReturn returnMode,
                                                                       Transaction* tx) override;

        Result<DbObject> doReadUpdate(Topic topic,
                                                                       const ModelInfo& model,
                                                                       const ObjectId& id,
                                                                       const update::Request& request,                                                                       
                                                                       update::ModifyReturn returnMode,
                                                                       Transaction* tx) override;

        Result<size_t> doUpdateMany(
            const ModelInfo& model,
            const ModelIndexQuery& query,
            const update::Request& request,
            Transaction* tx
        ) override;

        Result<DbObject> doFindUpdateCreate(
                                             const ModelInfo& model,
                                             const ModelIndexQuery& query,
                                             const update::Request& request,
                                             const HATN_COMMON_NAMESPACE::SharedPtr<dataunit::Unit>& object,
                                             update::ModifyReturn returnMode,
                                             Transaction* tx) override;

        Result<DbObject> doFindOne(
            const ModelInfo& model,
            const ModelIndexQuery& query
        ) override;

        Result<std::pmr::set<Topic>>
        doListModelTopics(
            const ModelInfo& model,
            const common::DateRange& partitionDateRange,
            bool onlyDefaultPartition
            ) override;

    private:

        std::unique_ptr<RocksdbClient_p> d;
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBCLIENT_H
