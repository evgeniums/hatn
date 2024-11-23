/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file db/plugins/rocksdb/rocksdbclient.cpp
  *
  *   Implementation of database client for RocksDB backend.
  *
  */

/****************************************************************************/

#include <boost/algorithm/string.hpp>

#include <hatn/logcontext/contextlogger.h>

#include <rocksdb/db.h>
#include <rocksdb/utilities/transaction_db.h>

#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/ipp/syntax.ipp>

#include <hatn/base/configobject.h>

#include <hatn/db/plugins/rocksdb/rocksdberror.h>
#include <hatn/db/plugins/rocksdb/rocksdbclient.h>
#include <hatn/db/plugins/rocksdb/rocksdbhandler.h>
#include <hatn/db/plugins/rocksdb/rocksdbschema.h>
#include <hatn/db/plugins/rocksdb/saveuniquekey.h>
#include <hatn/db/plugins/rocksdb/detail/rocksdbhandler.ipp>

HATN_DB_USING

#ifdef BUILD_DEBUG

    #define ENSURE_MODEL_SCHEMA \
        HATN_CHECK_RETURN(d->handler->ensureModelSchema(model))

#else

    #define ENSURE_MODEL_SCHEMA

#endif

namespace {

/********************** Client config **************************/

#if defined (BUILD_ANDROID) || defined (BUILD_IOS)
    constexpr const bool DefaultWaitCompactShutdown=false;
#else
    constexpr const bool DefaultWaitCompactShutdown=true;
#endif

HDU_UNIT(rocksdb_config,
         HDU_FIELD(dbpath,HDU_TYPE_FIXED_STRING(512),1,true)
         HDU_FIELD(wait_compact_shutdown,TYPE_BOOL,2,false,DefaultWaitCompactShutdown)
         )

HDU_UNIT(rocksdb_options,
         HDU_FIELD(readonly,TYPE_BOOL,1)
         )

} // anonymous namespace

HATN_ROCKSDB_NAMESPACE_BEGIN

/********************** RocksdbClient **************************/

using RocksdbConfig = base::ConfigObject<rocksdb_config::type>;
using RocksdbOptions = base::ConfigObject<rocksdb_options::type>;

class RocksdbClient_p
{
    public:

    RocksdbConfig cfg;
    RocksdbOptions opt;

    std::unique_ptr<RocksdbHandler> handler;
};

//---------------------------------------------------------------

RocksdbClient::RocksdbClient(common::STR_ID_TYPE id)
    :Client(std::move(id)),
     d(std::make_unique<RocksdbClient_p>())
{
}

//---------------------------------------------------------------

RocksdbClient::~RocksdbClient()
{
    if (isOpen())
    {
        Error ec;
        invokeCloseDb(ec);
    }
}

//---------------------------------------------------------------

void RocksdbClient::doOpenDb(const ClientConfig &config, Error &ec, base::config_object::LogRecords& records)
{
    HATN_CTX_SCOPE("rocksdbopen")    
    invokeOpenDb(config,ec,records,false);
}

//---------------------------------------------------------------

Error RocksdbClient::doCreateDb(const ClientConfig &config, base::config_object::LogRecords& records)
{
    HATN_CTX_SCOPE("rocksdbcreatedb")
    Error ec;
    invokeOpenDb(config,ec,records,true);
    HATN_CHECK_EC(ec)
    invokeCloseDb(ec);
    return ec;
}

//---------------------------------------------------------------

Error RocksdbClient::doDestroyDb(const ClientConfig &config, base::config_object::LogRecords& records)
{
    HATN_CTX_SCOPE("rocksdbdestroydb")

    // load config
    auto ec=d->cfg.loadLogConfig(config.main,config.mainPath,records);
    HATN_CHECK_EC(ec)

    // destroy database
    rocksdb::Options options;
    auto status = rocksdb::DestroyDB(d->cfg.config().field(rocksdb_config::dbpath).c_str(),options);
    if (!status.ok())
    {
        copyRocksdbError(ec,DbError::DB_DESTROY_FAILED,status);
    }

    // done
    return OK;
}

//---------------------------------------------------------------

void RocksdbClient::doCloseDb(Error &ec)
{
    HATN_CTX_SCOPE("rocksdbclosedb")
    invokeCloseDb(ec);
}

//---------------------------------------------------------------

void RocksdbClient::invokeOpenDb(const ClientConfig &config, Error &ec, base::config_object::LogRecords& records, bool createIfMissing)
{
    HATN_CTX_SCOPE("rocksdbinvokeopen")

    // load config
    ec=d->cfg.loadLogConfig(config.main,config.mainPath,records);
    if (ec)
    {
        HATN_CTX_SCOPE_ERROR("load-main-config")
        return;
    }

    // load options
    //! @todo records with prefix?
    ec=d->opt.loadLogConfig(config.opt,config.optPath,records);
    if (ec)
    {
        HATN_CTX_SCOPE_ERROR("load-opt-config")
        return;
    }

    //! @todo fill options from configuration
    ROCKSDB_NAMESPACE::DB* db{nullptr};
    ROCKSDB_NAMESPACE::TransactionDB* transactionDb{nullptr};
    ROCKSDB_NAMESPACE::Options options;
    ROCKSDB_NAMESPACE::TransactionDBOptions txOptions;
    ROCKSDB_NAMESPACE::ColumnFamilyOptions collCfOptions;
    ROCKSDB_NAMESPACE::ColumnFamilyOptions indexCfOptions;
    indexCfOptions.merge_operator=std::make_shared<SaveUniqueKey>();
    ROCKSDB_NAMESPACE::ColumnFamilyOptions ttlCfOptions;
    options.create_if_missing = createIfMissing;

    std::string dbPath{d->cfg.config().field(rocksdb_config::dbpath).c_str()};
    bool createNew=false;
    bool readOnly=d->opt.config().field(rocksdb_options::readonly).value();

    // list names of existing column families
    std::vector<std::string> cfNames;
    auto status = ROCKSDB_NAMESPACE::DB::ListColumnFamilies(options, dbPath, &cfNames);
    // check status
    if (!status.ok())
    {
        createNew=!readOnly && status.subcode()==ROCKSDB_NAMESPACE::Status::SubCode::kPathNotFound && createIfMissing;
        if (!createNew)
        {
            HATN_CTX_SCOPE_ERROR("list-column-families")
            copyRocksdbError(ec,DbError::PARTITION_LIST_FAILED,status);
            return;
        }
    }

    // fill cf descriptors
    std::vector<ROCKSDB_NAMESPACE::ColumnFamilyDescriptor> cfDescriptors;
    for (auto&& cfName : cfNames)
    {
        if (boost::ends_with(cfName,RocksdbPartition::CollectionSuffix))
        {
            cfDescriptors.emplace_back(cfName,collCfOptions);
        }
        else if (boost::ends_with(cfName,RocksdbPartition::IndexSuffix))
        {
            cfDescriptors.emplace_back(cfName,indexCfOptions);
        }
        else if (boost::ends_with(cfName,RocksdbPartition::TtlSuffix))
        {
            cfDescriptors.emplace_back(cfName,ttlCfOptions);
        }
        else if (cfName=="default")
        {
            cfDescriptors.emplace_back(cfName,collCfOptions);
        }
        else
        {
            HATN_CTX_SCOPE_ERROR("unknown-column-family-listed")
            HATN_CTX_SCOPE_PUSH("cf",cfName)
            db::setDbErrorCode(ec,DbError::PARTITION_LIST_FAILED);
            return;
        }
    }

    // open database
    HATN_CTX_SCOPE_PUSH("dbpath",dbPath)
    HATN_CTX_SCOPE_PUSH("create_db",createNew)
    std::vector<ROCKSDB_NAMESPACE::ColumnFamilyHandle*> cfHandles;
    if (createNew)
    {
        status = ROCKSDB_NAMESPACE::TransactionDB::Open(options,txOptions,dbPath,&transactionDb);
        db=transactionDb;
    }
    else
    {
        if (d->opt.config().field(rocksdb_options::readonly).value())
        {
            HATN_CTX_SCOPE_PUSH("readonly",true)
            status = ROCKSDB_NAMESPACE::DB::OpenForReadOnly(options,dbPath,cfDescriptors,&cfHandles,&db);
        }
        else
        {
            status = ROCKSDB_NAMESPACE::TransactionDB::Open(options,txOptions,dbPath,cfDescriptors,&cfHandles,&transactionDb);
            db=transactionDb;
        }
    }

    // check status
    if (!status.ok())
    {
        if (createNew)
        {
            HATN_CTX_SCOPE_ERROR("create-db")
            copyRocksdbError(ec,DbError::DB_CREATE_FAILED,status);
        }
        else
        {
            HATN_CTX_SCOPE_ERROR("open-db")
            copyRocksdbError(ec,DbError::DB_OPEN_FAILED,status);
        }
        return;
    }

    // create handler
    d->handler=std::make_unique<RocksdbHandler>(new RocksdbHandler_p(db,transactionDb));
    d->handler->p()->collColumnFamilyOptions=collCfOptions;
    d->handler->p()->indexColumnFamilyOptions=indexCfOptions;
    d->handler->p()->ttlColumnFamilyOptions=ttlCfOptions;

    // fill partitions
    std::set<size_t> cfIndexes;
    for (size_t i=0;i<cfHandles.size();i++)
    {
        auto cfHandle=cfHandles[i];

        const auto& name=cfHandle->GetName();
        if (name=="default")
        {
            d->handler->p()->defaultCf.reset(cfHandle);
            cfIndexes.insert(i);
            continue;
        }

        std::vector<std::string> parts;
        boost::split(parts,name,boost::is_any_of("_"));
        if (parts.size()!=2)
        {
            HATN_CTX_SCOPE_ERROR("column-family-name")
            HATN_CTX_SCOPE_PUSH("cf",name)
            db::setDbErrorCode(ec,DbError::PARTITION_LIST_FAILED);
            break;
        }

        auto partitionDateRange=common::DateRange::fromString(parts[0]);
        if (partitionDateRange)
        {
            HATN_CTX_SCOPE_ERROR("column-family-date-range")
            HATN_CTX_SCOPE_PUSH("cf",name)
            db::setDbErrorCode(ec,DbError::PARTITION_LIST_FAILED);
            break;
        }

        auto partition=d->handler->partition(partitionDateRange.value());
        if (!partition)
        {
            partition=std::make_shared<RocksdbPartition>();
            d->handler->insertPartition(partitionDateRange.value(),partition);
        }

        if (parts[1]==RocksdbPartition::CollectionSuffix)
        {
            partition->collectionCf.reset(cfHandle);
        }
        else if (parts[1]==RocksdbPartition::IndexSuffix)
        {
            partition->indexCf.reset(cfHandle);
        }
        else if (parts[1]==RocksdbPartition::TtlSuffix)
        {
            partition->ttlCf.reset(cfHandle);
        }
        else
        {
            HATN_CTX_SCOPE_ERROR("unknown-column-family-handle")
            HATN_CTX_SCOPE_PUSH("cf",name)
            db::setDbErrorCode(ec,DbError::PARTITION_LIST_FAILED);
            break;
        }

        cfIndexes.insert(i);
    }

    // create default partition
    if (!ec)
    {
        auto r=d->handler->createPartition();
        if (r)
        {
            HATN_CTX_SCOPE_ERROR("create-default-partition")
            ec=r.takeError();
        }
        else
        {
            d->handler->p()->defaultPartition=r.takeValue();
        }
    }

    // close db in case of error
    if (ec)
    {
        // explicitly delete cf handles not owned by handler
        for (size_t i=0;i<cfHandles.size();i++)
        {
            auto it=cfIndexes.find(i);
            if (it==cfIndexes.end())
            {
                delete cfHandles[i];
            }
        }

        // implicitly delete cf handles owned by handler
        d->handler->resetCf();

        // close db
        auto status=d->handler->p()->db->Close();
        if (!status.ok())
        {
            HATN_CTX_ERROR(makeError(DbError::DB_CLOSE_FAILED,status),"failed to close db after opening failed");
        }

        // done closing db on failure
        d->handler.reset();
    }

    //! @todo Compaction filter using ttl marks.
    //! @todo Ttl indexes background worker.
}

//---------------------------------------------------------------

void RocksdbClient::invokeCloseDb(Error &ec)
{
    HATN_CTX_SCOPE("rocksdbinvokeclose")

    ec.reset();
    if (d->handler)
    {
        d->handler->resetCf();

        rocksdb::Status status;
        if (d->cfg.config().field(rocksdb_config::wait_compact_shutdown).value())
        {
            auto opt = rocksdb::WaitForCompactOptions{};
            opt.close_db = true;
            status = d->handler->p()->db->WaitForCompact(opt);
        }
        else
        {
            status=d->handler->p()->db->Close();
        }

        // check status
        if (!status.ok())
        {
            copyRocksdbError(ec,DbError::DB_CLOSE_FAILED,status);
        }

        // done
        d->handler.reset();
    }
}

//---------------------------------------------------------------

Error RocksdbClient::doSetSchema(std::shared_ptr<Schema> schema)
{
    HATN_CTX_SCOPE("rocksdbaddschema")

    auto rs=RocksdbSchemas::instance().schema(schema->name());
    if (!rs)
    {
        HATN_CTX_SCOPE_LOCK()
        return dbError(DbError::SCHEMA_NOT_REGISTERED);
    }

    d->handler->setSchema(std::move(rs));
    return OK;
}

//---------------------------------------------------------------

Result<std::shared_ptr<Schema>> RocksdbClient::doGetSchema() const
{
    HATN_CTX_SCOPE("rocksdbfindschema")

    auto s=d->handler->schema();
    if (!s)
    {
        HATN_CTX_SCOPE_LOCK()
        return dbError(DbError::SCHEMA_NOT_FOUND);
    }
    return s->dbSchema();
}

//---------------------------------------------------------------

Error RocksdbClient::doCheckSchema()
{
    //! @todo Load ids of keys and collections from schema
    return common::CommonError::NOT_IMPLEMENTED;
}

//---------------------------------------------------------------

Error RocksdbClient::doMigrateSchema()
{
    //! @todo Save ids of keys and collections from schema
    return common::CommonError::NOT_IMPLEMENTED;
}

//---------------------------------------------------------------

Error RocksdbClient::doAddDatePartitions(const std::vector<ModelInfo>&, const std::set<common::DateRange>& dateRanges)
{
    HATN_CTX_SCOPE("rocksdbaddpartitions")

    for (auto&& range: dateRanges)
    {
        auto r=d->handler->createPartition(range);
        HATN_CHECK_RESULT(r)
    }

    // done
    return OK;
}

//---------------------------------------------------------------

Error RocksdbClient::doDeleteDatePartitions(const std::vector<ModelInfo>&, const std::set<common::DateRange>& dateRanges)
{
    HATN_CTX_SCOPE("rocksdbdeletepartitions")

    //! @todo test delete partition
    for (auto&& range: dateRanges)
    {
        auto ec=d->handler->deletePartition(range);
        HATN_CHECK_EC(ec)
    }

    // done
    return OK;
}

//---------------------------------------------------------------

Error RocksdbClient::doCreate(const Topic& topic, const ModelInfo& model, dataunit::Unit* object, Transaction* tx)
{
    HATN_CTX_SCOPE("rocksdbcreate")

    ENSURE_MODEL_SCHEMA

    auto rdbModel=model.nativeModel<RocksdbModel>();
    Assert(rdbModel,"Model not registered");

    return rdbModel->createObject(*d->handler,topic,object,tx);
}

//---------------------------------------------------------------

Result<common::SharedPtr<dataunit::Unit>> RocksdbClient::doRead(const Topic& topic,
                                                                const ModelInfo &model,
                                                                const ObjectId &id,
                                                                Transaction* tx,
                                                                bool forUpdate
                                                                )
{
    HATN_CTX_SCOPE("rocksdbread")

    ENSURE_MODEL_SCHEMA

    auto rdbModel=model.nativeModel<RocksdbModel>();
    Assert(rdbModel,"Model not registered");

    return rdbModel->readObject(*d->handler,topic,id,tx,forUpdate);
}

//---------------------------------------------------------------

Result<common::SharedPtr<dataunit::Unit>> RocksdbClient::doRead(const Topic& topic,
                                                                const ModelInfo &model,
                                                                const ObjectId &id,
                                                                const common::Date& date,
                                                                Transaction* tx,
                                                                bool forUpdate
                                                                )
{
    HATN_CTX_SCOPE("rocksdbreaddate")

    ENSURE_MODEL_SCHEMA

    auto rdbModel=model.nativeModel<RocksdbModel>();
    Assert(rdbModel,"Model not registered");

    return rdbModel->readObjectWithDate(*d->handler,topic,id,date,tx,forUpdate);
}

//---------------------------------------------------------------

Result<HATN_COMMON_NAMESPACE::pmr::vector<UnitWrapper>> RocksdbClient::doFind(
                                                                              const ModelInfo &model,
                                                                              const ModelIndexQuery &query,
                                                                              bool single
                                                                              )
{
    HATN_CTX_SCOPE("rocksdbfind")

    ENSURE_MODEL_SCHEMA

    auto rdbModel=model.nativeModel<RocksdbModel>();
    Assert(rdbModel,"Model not registered");

    return rdbModel->find(*d->handler,query,single);
}

//---------------------------------------------------------------

Result<size_t> RocksdbClient::doCount(
        const ModelInfo &model,
        const ModelIndexQuery &query
    )
{
    HATN_CTX_SCOPE("rocksdbcount")

    ENSURE_MODEL_SCHEMA

    auto rdbModel=model.nativeModel<RocksdbModel>();
    Assert(rdbModel,"Model not registered");

    return rdbModel->count(*d->handler,query);
}

//---------------------------------------------------------------

Error RocksdbClient::doDeleteObject(
        const Topic& topic,
        const ModelInfo &model,
        const ObjectId &id,
        const common::Date& date,
        Transaction* tx
    )
{
    HATN_CTX_SCOPE("rocksdbdelete")

    ENSURE_MODEL_SCHEMA

    auto rdbModel=model.nativeModel<RocksdbModel>();
    Assert(rdbModel,"Model not registered");

    return rdbModel->deleteObjectWithDate(*d->handler,topic,id,date,tx);
}

//---------------------------------------------------------------

Error RocksdbClient::doDeleteObject(
        const Topic& topic,
        const ModelInfo &model,
        const ObjectId &id,
        Transaction* tx
    )
{
    HATN_CTX_SCOPE("rocksdbdelete")

    ENSURE_MODEL_SCHEMA

    auto rdbModel=model.nativeModel<RocksdbModel>();
    Assert(rdbModel,"Model not registered");

    return rdbModel->deleteObject(*d->handler,topic,id,tx);
}

//---------------------------------------------------------------

Error RocksdbClient::doTransaction(const TransactionFn &fn)
{
    return d->handler->transaction(fn);
}

//---------------------------------------------------------------

Result<size_t> RocksdbClient::doDeleteMany(
                                  const ModelInfo &model,
                                  const ModelIndexQuery &query,
                                  Transaction* tx)
{
    HATN_CTX_SCOPE("rocksdbdeletemany")

    ENSURE_MODEL_SCHEMA

    auto rdbModel=model.nativeModel<RocksdbModel>();
    Assert(rdbModel,"Model not registered");

    return rdbModel->deleteMany(*d->handler,query,tx);
}

//---------------------------------------------------------------

Error RocksdbClient::doUpdateObject(const Topic &topic,
                                    const ModelInfo &model,
                                    const update::Request &request,
                                    const ObjectId &id,
                                    const common::Date &date,
                                    Transaction* tx)
{
    HATN_CTX_SCOPE("rocksdbupdate")

    ENSURE_MODEL_SCHEMA

    auto rdbModel=model.nativeModel<RocksdbModel>();
    Assert(rdbModel,"Model not registered");

    auto r=rdbModel->updateObjectWithDate(*d->handler,topic,id,request,date,db::update::ModifyReturn::None,tx);
    if (r)
    {
        return r.takeError();
    }
    return OK;
}

//---------------------------------------------------------------

Error RocksdbClient::doUpdateObject(const Topic &topic,
                                    const ModelInfo &model,
                                    const update::Request &request,
                                    const ObjectId &id,
                                    Transaction* tx)
{
    HATN_CTX_SCOPE("rocksdbupdate")

    ENSURE_MODEL_SCHEMA

    auto rdbModel=model.nativeModel<RocksdbModel>();
    Assert(rdbModel,"Model not registered");

    auto r=rdbModel->updateObject(*d->handler,topic,id,request,db::update::ModifyReturn::None,tx);
    if (r)
    {
        return r.takeError();
    }
    return OK;
}

//---------------------------------------------------------------

Result<size_t> RocksdbClient::doUpdateMany(
                                  const ModelInfo &model,
                                  const ModelIndexQuery &query,
                                  const update::Request& request,
                                  Transaction* tx)
{
    HATN_CTX_SCOPE("rocksdbupdatemany")

    ENSURE_MODEL_SCHEMA

    auto rdbModel=model.nativeModel<RocksdbModel>();
    Assert(rdbModel,"Model not registered");

    return rdbModel->updateMany(*d->handler,query,request,db::update::ModifyReturn::None,tx);
}

//---------------------------------------------------------------

Result<common::SharedPtr<dataunit::Unit>> RocksdbClient::doReadUpdate(const Topic &topic,
                                    const ModelInfo &model,
                                    const ObjectId &id,
                                    const update::Request &request,                                    
                                    const common::Date &date,
                                    update::ModifyReturn returnType,
                                    Transaction* tx)
{
    HATN_CTX_SCOPE("rocksdbreadupdate")

    ENSURE_MODEL_SCHEMA

    auto rdbModel=model.nativeModel<RocksdbModel>();
    Assert(rdbModel,"Model not registered");

    auto r=rdbModel->updateObjectWithDate(*d->handler,topic,id,request,date,returnType,tx);
    HATN_CHECK_RESULT(r)
    if (r.value().isNull())
    {
        return dbError(DbError::NOT_FOUND);
    }
    return r;
}

//---------------------------------------------------------------

Result<common::SharedPtr<dataunit::Unit>> RocksdbClient::doReadUpdate(const Topic &topic,
                                                                      const ModelInfo &model,
                                                                      const ObjectId &id,
                                                                      const update::Request &request,                                                                      
                                                                      update::ModifyReturn returnType,
                                                                      Transaction* tx)
{
    HATN_CTX_SCOPE("rocksdbreadupdate")

    ENSURE_MODEL_SCHEMA

    auto rdbModel=model.nativeModel<RocksdbModel>();
    Assert(rdbModel,"Model not registered");

    auto r=rdbModel->updateObject(*d->handler,topic,id,request,returnType,tx);
    HATN_CHECK_RESULT(r)
    if (r.value().isNull())
    {
        return dbError(DbError::NOT_FOUND);
    }
    return r;
}

//---------------------------------------------------------------

Result<common::SharedPtr<dataunit::Unit>> RocksdbClient::doFindUpdateCreate(
                                                                            const ModelInfo& model,
                                                                            const ModelIndexQuery& query,
                                                                            const update::Request& request,
                                                                            const HATN_COMMON_NAMESPACE::SharedPtr<dataunit::Unit>& object,
                                                                            update::ModifyReturn returnType,
                                                                            Transaction* tx)
{
    HATN_CTX_SCOPE("rocksdbfindupdatecreate")

    ENSURE_MODEL_SCHEMA

    auto rdbModel=model.nativeModel<RocksdbModel>();
    Assert(rdbModel,"Model not registered");

    return rdbModel->findUpdateCreate(*d->handler,query,request,object,returnType,tx);
}

//---------------------------------------------------------------

HATN_ROCKSDB_NAMESPACE_END
