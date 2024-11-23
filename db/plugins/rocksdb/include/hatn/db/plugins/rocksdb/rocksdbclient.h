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

        Error doSetSchema(std::shared_ptr<Schema> schema) override;
        Result<std::shared_ptr<Schema>> doGetSchema() const override;
        Error doCheckSchema() override;
        Error doMigrateSchema() override;

        void doOpenDb(const ClientConfig& config, Error& ec, base::config_object::LogRecords& records) override;
        void doCloseDb(Error& ec) override;

        Error doAddDatePartitions(const std::vector<ModelInfo>& models, const std::set<common::DateRange>& dateRanges) override;
        Error doDeleteDatePartitions(const std::vector<ModelInfo>& models, const std::set<common::DateRange>& dateRanges) override;

        Error doCreate(const Topic& topic, const ModelInfo& model, dataunit::Unit* object, Transaction* tx) override;

        Result<common::SharedPtr<dataunit::Unit>> doRead(const Topic& topic,
                                                         const ModelInfo& model,
                                                         const ObjectId& id,
                                                         Transaction* tx,
                                                         bool forUpdate
                                                         ) override;
        Result<common::SharedPtr<dataunit::Unit>> doRead(const Topic& topic,
                                                         const ModelInfo& model,
                                                         const ObjectId& id,
                                                         const common::Date& date,
                                                         Transaction* tx,
                                                         bool forUpdate
                                                         ) override;

        Result<HATN_COMMON_NAMESPACE::pmr::vector<UnitWrapper>> doFind(
            const ModelInfo& model,
            const ModelIndexQuery& query,
            bool single
        ) override;

        Error doDeleteObject(const Topic& topic,
                             const ModelInfo& model,
                             const ObjectId& id,
                             const common::Date& date,
                             Transaction* tx) override;

        Error doDeleteObject(const Topic& topic,
                             const ModelInfo& model,
                             const ObjectId& id,
                             Transaction* tx) override;

        Result<size_t> doDeleteMany(
            const ModelInfo& model,
            const ModelIndexQuery& query,
            Transaction* tx
            ) override;

        Error doTransaction(const TransactionFn& fn) override;

        Error doUpdateObject(const Topic& topic,
                       const ModelInfo& model,
                       const update::Request& request,
                       const ObjectId& id,
                       const common::Date& date,
                       Transaction* tx) override;

        Error doUpdateObject(const Topic& topic,
                       const ModelInfo& model,
                       const update::Request& request,
                       const ObjectId& id,
                       Transaction* tx) override;

        Result<common::SharedPtr<dataunit::Unit>> doReadUpdate(const Topic& topic,
                                                                       const ModelInfo& model,
                                                                       const update::Request& request,
                                                                       const ObjectId& id,
                                                                       const common::Date& date,
                                                                       update::ModifyReturn returnType,
                                                                       Transaction* tx) override;

        Result<common::SharedPtr<dataunit::Unit>> doReadUpdate(const Topic& topic,
                                                                       const ModelInfo& model,
                                                                       const update::Request& request,
                                                                       const ObjectId& id,
                                                                       update::ModifyReturn returnType,
                                                                       Transaction* tx) override;

        Result<size_t> doUpdateMany(
            const ModelInfo& model,
            const ModelIndexQuery& query,
            const update::Request& request,
            Transaction* tx
        ) override;

        Result<common::SharedPtr<dataunit::Unit>> doReadUpdateCreate(
                                                                     const ModelInfo& model,
                                                                     const ModelIndexQuery& query,
                                                                     const update::Request& request,
                                                                     const HATN_COMMON_NAMESPACE::SharedPtr<dataunit::Unit>& object,
                                                                     update::ModifyReturn returnType,
                                                                     Transaction* tx) override;

    private:

        std::unique_ptr<RocksdbClient_p> d;
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBCLIENT_H
