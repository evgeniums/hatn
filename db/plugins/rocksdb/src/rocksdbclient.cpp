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

#include <hatn/common/utils.h>
#include <hatn/common/filesystem.h>

#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/ipp/syntax.ipp>

#include <hatn/base/configobject.h>

#include <hatn/db/plugins/rocksdb/rocksdberror.h>
#include <hatn/db/plugins/rocksdb/rocksdbclient.h>
#include <hatn/db/plugins/rocksdb/rocksdbhandler.h>
#include <hatn/db/plugins/rocksdb/rocksdbschema.h>
#include <hatn/db/plugins/rocksdb/saveuniquekey.h>
#include <hatn/db/plugins/rocksdb/detail/rocksdbhandler.ipp>
#include <hatn/db/plugins/rocksdb/ttlcompactionfilter.h>
#include <hatn/db/plugins/rocksdb/modeltopics.h>
#include <hatn/db/plugins/rocksdb/rocksdbencryption.h>
#include <hatn/db/plugins/rocksdb/rocksdbmodels.h>

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

/********************** Client environment **************************/

class RocksdbEnvironment : public ClientEnvironment
{
    public:

        std::shared_ptr<RocksdbEncryptionManager> encryptionManager;
        std::unique_ptr<rocksdb::Env> env;
};

/********************** RocksdbClient **************************/

using RocksdbConfig = base::ConfigObject<rocksdb_config::type>;
using RocksdbOptions = base::ConfigObject<rocksdb_options::type>;

class RocksdbClient_p
{
    public:

        RocksdbConfig cfg;
        RocksdbOptions opt;

        std::unique_ptr<RocksdbHandler> handler;
        std::unique_ptr<TtlCompactionFilter> ttlCompactionFilter;

        std::shared_ptr<RocksdbEnvironment> env;

        RocksdbClient_p() : ttlCompactionFilter(std::make_unique<TtlCompactionFilter>())
        {}
};

//---------------------------------------------------------------

RocksdbClient::RocksdbClient(const lib::string_view &id)
    :Client(id),
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

void RocksdbClient::doOpenDb(const ClientConfig &config, Error &ec, base::config_object::LogRecords& records, bool creatIfNotExists)
{
    HATN_CTX_SCOPE("rdbopen")
    invokeOpenDb(config,ec,records,creatIfNotExists);
}

//---------------------------------------------------------------

Error RocksdbClient::doCreateDb(const ClientConfig &config, base::config_object::LogRecords& records)
{
    HATN_CTX_SCOPE("rdbcreatedb")
    Error ec;
    invokeOpenDb(config,ec,records,true);
    HATN_CHECK_EC(ec)
    invokeCloseDb(ec);
    HATN_CHECK_EC(ec)
    HATN_CTX_INFO("db created")
    return ec;
}

//---------------------------------------------------------------

Error RocksdbClient::doDestroyDb(const ClientConfig &config, base::config_object::LogRecords& records)
{
    HATN_CTX_SCOPE("rdbdestroydb")

    // load config
    auto ec=d->cfg.loadLogConfig(*config.main,config.mainPath,records);
    HATN_CHECK_EC(ec)

    // destroy database
    rocksdb::Options options;
    std::string path{d->cfg.config().field(rocksdb_config::dbpath).value()};
    auto status = rocksdb::DestroyDB(path,options);
    if (!status.ok())
    {
        copyRocksdbError(ec,DbError::DB_DESTROY_FAILED,status);
    }
    lib::fs_error_code fsec;
    lib::filesystem::remove_all(path,fsec);
    if (fsec)
    {
        auto ec1=common::lib::makeFilesystemError(fsec);
        HATN_CTX_ERROR(ec1,"failed to delete database folder");
    }

    if (!ec && !fsec)
    {
        HATN_CTX_INFO("db destroyed")
    }

    // done
    return ec;
}

//---------------------------------------------------------------

void RocksdbClient::doCloseDb(Error &ec)
{
    HATN_CTX_SCOPE("rdbclosedb")
    invokeCloseDb(ec);
}

//---------------------------------------------------------------

void RocksdbClient::invokeOpenDb(const ClientConfig &config, Error &ec, base::config_object::LogRecords& records, bool createIfMissing)
{
    // load config
    ec=d->cfg.loadLogConfig(*config.main,config.mainPath,records);
    if (ec)
    {
        HATN_CTX_SCOPE_ERROR("load-main-config")
        return;
    }

    // load options
    ec=d->opt.loadLogConfig(*config.opt,config.optPath,records);
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

    // prepare env
    if (config.environment)
    {
        d->env=std::dynamic_pointer_cast<RocksdbEnvironment>(config.environment);
        if (d->env)
        {
            options.env=d->env->env.get();
        }
    }
    if (!d->env && config.encryptionManager)
    {
        d->env=std::make_shared<RocksdbEnvironment>();
        d->env->encryptionManager=std::make_shared<RocksdbEncryptionManager>(config.encryptionManager);
        d->env->env=makeEncryptedEnv(d->env->encryptionManager);
        options.env=d->env->env.get();
    }

    options.create_if_missing = createIfMissing;

    //! @todo Make compression configurable
    auto compression=ROCKSDB_NAMESPACE::CompressionType::kLZ4HCCompression;
    options.compression=compression;

    //! @todo Enable compression for mobile, disable for server
    //! @todo build ZSTD
    options.wal_compression=ROCKSDB_NAMESPACE::CompressionType::kZSTD;
#if 0
    //! @todo Limit WAL size for clients
    options.max_total_wal_size=1024*1024*16;

    //! @todo Tune compaction period
    // options.periodic_compaction_seconds=15;
#endif

#ifdef BUILD_DEBUG
    txOptions.transaction_lock_timeout=10000;
#endif
    ROCKSDB_NAMESPACE::ColumnFamilyOptions collCfOptions;
    collCfOptions.compaction_filter=d->ttlCompactionFilter.get();
    collCfOptions.merge_operator=std::make_shared<MergeModelTopic>();
    collCfOptions.compression=compression;

    ROCKSDB_NAMESPACE::ColumnFamilyOptions indexCfOptions;
    indexCfOptions.merge_operator=std::make_shared<SaveUniqueKey>();
    indexCfOptions.compaction_filter=d->ttlCompactionFilter.get();
    indexCfOptions.compression=compression;

    ROCKSDB_NAMESPACE::ColumnFamilyOptions ttlCfOptions;
    ttlCfOptions.compression=compression;

    // construct db path
    std::string dbPath{d->cfg.config().field(rocksdb_config::dbpath).c_str()};
    if (!config.dbPathPrefix.empty())
    {
        lib::filesystem::path path{dbPath};
        if (!path.is_absolute())
        {
            lib::filesystem::path dir{config.dbPathPrefix};
            dir.append(dbPath);
            dbPath=dir.string();
        }
    }

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
        lib::filesystem::path dbDir{dbPath};
        lib::fs_error_code fec;
        lib::filesystem::create_directories(dbDir.parent_path(),fec);
        if (fec)
        {
            ec=Error{fec.value(),&fec.category()};
            return;
        }

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
    HATN_CTX_INFO("db opened")

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

        std::shared_ptr<RocksdbPartition> partition;
        if (partitionDateRange.value().isNull())
        {
            partition=d->handler->p()->defaultPartition;
            if (!partition)
            {
                partition=std::make_shared<RocksdbPartition>();
                d->handler->p()->defaultPartition=partition;
                HATN_CTX_INFO("found default partition")
            }
        }
        else
        {
            partition=d->handler->partition(partitionDateRange.value());
            if (!partition)
            {
                HATN_CTX_DEBUG_RECORDS(0,"partition not registered",
                                       {"range",partitionDateRange->toString()},
                                       {"cf",parts[1]}
                                       )
                HATN_CTX_INFO_RECORDS("found date partition",{"range",partitionDateRange->toString()})
                partition=std::make_shared<RocksdbPartition>(partitionDateRange.value());
                d->handler->insertPartition(partition);
            }
            else
            {
                HATN_CTX_DEBUG_RECORDS(0,"date partition already registered, just add cf",
                                       {"range",partitionDateRange->toString()},
                                       {"cf",parts[1]}
                                       )
            }
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
    if (!ec && !d->handler->p()->defaultPartition)
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

    //! @todo Ttl indexes background worker.
}

//---------------------------------------------------------------

void RocksdbClient::invokeCloseDb(Error &ec)
{
    HATN_CTX_SCOPE("rdbinvokeclose")

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
    d->env.reset();
}

//---------------------------------------------------------------

Error RocksdbClient::doSetSchema(std::shared_ptr<Schema> schema)
{
    HATN_CTX_SCOPE("rdbaddschema")

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
    HATN_CTX_SCOPE("rdbfindschema")

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
    for (auto&& range: dateRanges)
    {
        auto r=d->handler->createPartition(range);
        HATN_CHECK_RESULT(r)
    }

    // done
    return OK;
}

//---------------------------------------------------------------

Result<std::set<common::DateRange>> RocksdbClient::doListDatePartitions()
{
    ROCKSDB_NAMESPACE::Options options;
    if (d->env)
    {
        options.env=d->env->env.get();
    }
    std::string dbPath{d->cfg.config().field(rocksdb_config::dbpath).c_str()};

    // list names of existing column families
    std::vector<std::string> cfNames;
    auto status = ROCKSDB_NAMESPACE::DB::ListColumnFamilies(options, dbPath, &cfNames);

    // check status
    if (!status.ok())
    {
        return makeError(DbError::PARTITION_LIST_FAILED,status);
    }

    std::set<common::DateRange> r;
    for (auto&& it: cfNames)
    {
        std::vector<std::string> parts;
        common::Utils::split(parts,it,'_');
        if (parts.size()==2)
        {
            auto range=common::DateRange::fromString(parts[0]);
            if (!range)
            {
                if (range->isValid())
                {
//! @maybe Log debug
#if 0
                    std::cout << "Found partition range " << range->toString() << " for column family " << parts[1] << std::endl;
#endif
                    r.insert(range.value());
                }
            }
            else
            {
                HATN_CTX_ERROR_RECORDS(range.error(),"invalid partition range",{"cf",it})
            }
        }
    }

    // done
    return r;
}

//---------------------------------------------------------------

Error RocksdbClient::doDeleteDatePartitions(const std::vector<ModelInfo>&, const std::set<common::DateRange>& dateRanges)
{
    HATN_CTX_SCOPE("rdbdeletepartitions")

    for (auto&& range: dateRanges)
    {
        auto ec=d->handler->deletePartition(range);
        HATN_CHECK_EC(ec)
    }

    // done
    return OK;
}

//---------------------------------------------------------------

Error RocksdbClient::doCreate(Topic topic, const ModelInfo& model, const dataunit::Unit* object, Transaction* tx)
{
    HATN_CTX_SCOPE("rdbcreate")

    ENSURE_MODEL_SCHEMA

    auto rdbModel=model.nativeModel<RocksdbModel>();
    Assert(rdbModel,"Model not registered");

    return rdbModel->createObject(*d->handler,topic,object,tx);
}

//---------------------------------------------------------------

Result<DbObject> RocksdbClient::doRead(Topic topic,
                                                                const ModelInfo &model,
                                                                const ObjectId &id,
                                                                Transaction* tx,
                                                                bool forUpdate
                                                                )
{
    HATN_CTX_SCOPE("rdbread")

    ENSURE_MODEL_SCHEMA

    auto rdbModel=model.nativeModel<RocksdbModel>();
    Assert(rdbModel,"Model not registered");

    return rdbModel->readObject(*d->handler,topic,id,tx,forUpdate);
}

//---------------------------------------------------------------

Result<DbObject> RocksdbClient::doRead(Topic topic,
                                                                const ModelInfo &model,
                                                                const ObjectId &id,
                                                                const common::Date& date,
                                                                Transaction* tx,
                                                                bool forUpdate
                                                                )
{
    HATN_CTX_SCOPE("rdbreaddate")

    ENSURE_MODEL_SCHEMA

    auto rdbModel=model.nativeModel<RocksdbModel>();
    Assert(rdbModel,"Model not registered");

    return rdbModel->readObjectWithDate(*d->handler,topic,id,date,tx,forUpdate);
}

//---------------------------------------------------------------

Result<HATN_COMMON_NAMESPACE::pmr::vector<DbObject>> RocksdbClient::doFind(
                                                                              const ModelInfo &model,
                                                                              const ModelIndexQuery &query
                                                                              )
{
    HATN_CTX_SCOPE("rdbfind")

    ENSURE_MODEL_SCHEMA

    auto rdbModel=model.nativeModel<RocksdbModel>();
    Assert(rdbModel,"Model not registered");

    return rdbModel->find(*d->handler,query,false);
}

//---------------------------------------------------------------

Result<size_t> RocksdbClient::doCount(
        const ModelInfo &model,
        const ModelIndexQuery &query
    )
{
    HATN_CTX_SCOPE("rdbcount")

    ENSURE_MODEL_SCHEMA

    auto rdbModel=model.nativeModel<RocksdbModel>();
    Assert(rdbModel,"Model not registered");

    return rdbModel->count(*d->handler,query);
}

//---------------------------------------------------------------

Result<size_t> RocksdbClient::doCount(
    const ModelInfo &model,
    Topic topic
    )
{
    HATN_CTX_SCOPE("rdbcountmodel")
    return ModelTopics::count(model,topic,common::Date{},*d->handler);
}

//---------------------------------------------------------------

Result<size_t> RocksdbClient::doCount(
    const ModelInfo &model,
    const common::Date& date,
    Topic topic
    )
{
    HATN_CTX_SCOPE("rdbcountmodel")
    return ModelTopics::count(model,topic,date,*d->handler);
}
//---------------------------------------------------------------

Error RocksdbClient::doDeleteObject(
        Topic topic,
        const ModelInfo &model,
        const ObjectId &id,
        const common::Date& date,
        Transaction* tx
    )
{
    HATN_CTX_SCOPE("rdbdelete")

    ENSURE_MODEL_SCHEMA

    auto rdbModel=model.nativeModel<RocksdbModel>();
    Assert(rdbModel,"Model not registered");

    return rdbModel->deleteObjectWithDate(*d->handler,topic,id,date,tx);
}

//---------------------------------------------------------------

Error RocksdbClient::doDeleteObject(
        Topic topic,
        const ModelInfo &model,
        const ObjectId &id,
        Transaction* tx
    )
{
    HATN_CTX_SCOPE("rdbdelete")

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
    HATN_CTX_SCOPE("rdbdeletemany")

    ENSURE_MODEL_SCHEMA

    auto rdbModel=model.nativeModel<RocksdbModel>();
    Assert(rdbModel,"Model not registered");

    return rdbModel->deleteMany(*d->handler,query,false,tx);
}

//---------------------------------------------------------------

Result<size_t> RocksdbClient::doDeleteManyBulk(
    const ModelInfo &model,
    const ModelIndexQuery &query,
    Transaction* tx)
{
    HATN_CTX_SCOPE("rdbdeletemanybulk")

    ENSURE_MODEL_SCHEMA

    auto rdbModel=model.nativeModel<RocksdbModel>();
    Assert(rdbModel,"Model not registered");

    return rdbModel->deleteMany(*d->handler,query,true,tx);
}

//---------------------------------------------------------------

Error RocksdbClient::doUpdateObject(Topic topic,
                                    const ModelInfo &model,
                                    const ObjectId& id,
                                    const update::Request &request,
                                    const common::Date &date,
                                    Transaction* tx)
{
    HATN_CTX_SCOPE("rdbupdate")

    ENSURE_MODEL_SCHEMA

    auto rdbModel=model.nativeModel<RocksdbModel>();
    Assert(rdbModel,"Model not registered");

    auto r=rdbModel->updateObjectWithDate(*d->handler,topic,id,request,date,db::update::ModifyReturn::None,tx);
    HATN_CHECK_RESULT(r)
    if (r.value().isNull())
    {
        return dbError(DbError::NOT_FOUND);
    }
    return OK;
}

//---------------------------------------------------------------

Error RocksdbClient::doUpdateObject(Topic topic,
                                    const ModelInfo &model,
                                    const ObjectId& id,
                                    const update::Request &request,
                                    Transaction* tx)
{
    HATN_CTX_SCOPE("rdbbupdate")

    ENSURE_MODEL_SCHEMA

    auto rdbModel=model.nativeModel<RocksdbModel>();
    Assert(rdbModel,"Model not registered");

    auto r=rdbModel->updateObject(*d->handler,topic,id,request,db::update::ModifyReturn::None,tx);
    HATN_CHECK_RESULT(r)
    if (r.value().isNull())
    {
        return dbError(DbError::NOT_FOUND);
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
    HATN_CTX_SCOPE("rdbupdatemany")

    ENSURE_MODEL_SCHEMA

    auto rdbModel=model.nativeModel<RocksdbModel>();
    Assert(rdbModel,"Model not registered");

    return rdbModel->updateMany(*d->handler,query,request,db::update::ModifyReturn::None,tx);
}

//---------------------------------------------------------------

Result<DbObject> RocksdbClient::doReadUpdate(Topic topic,
                                    const ModelInfo &model,
                                    const ObjectId &id,
                                    const update::Request &request,                                    
                                    const common::Date &date,
                                    update::ModifyReturn returnMode,
                                    Transaction* tx)
{
    HATN_CTX_SCOPE("rdbreadupdate")

    ENSURE_MODEL_SCHEMA

    auto rdbModel=model.nativeModel<RocksdbModel>();
    Assert(rdbModel,"Model not registered");

    auto r=rdbModel->updateObjectWithDate(*d->handler,topic,id,request,date,returnMode,tx);
    HATN_CHECK_RESULT(r)
    if (r.value().isNull())
    {
        return dbError(DbError::NOT_FOUND);
    }
    return r;
}

//---------------------------------------------------------------

Result<DbObject> RocksdbClient::doReadUpdate(Topic topic,
                                                                      const ModelInfo &model,
                                                                      const ObjectId &id,
                                                                      const update::Request &request,                                                                      
                                                                      update::ModifyReturn returnMode,
                                                                      Transaction* tx)
{
    HATN_CTX_SCOPE("rdbreadupdate")

    ENSURE_MODEL_SCHEMA

    auto rdbModel=model.nativeModel<RocksdbModel>();
    Assert(rdbModel,"Model not registered");

    auto r=rdbModel->updateObject(*d->handler,topic,id,request,returnMode,tx);
    HATN_CHECK_RESULT(r)
    if (r.value().isNull())
    {
        return dbError(DbError::NOT_FOUND);
    }
    return r;
}

//---------------------------------------------------------------

Result<DbObject> RocksdbClient::doFindUpdateCreate(
                                            const ModelInfo& model,
                                            const ModelIndexQuery& query,
                                            const update::Request& request,
                                            const HATN_COMMON_NAMESPACE::SharedPtr<dataunit::Unit>& object,
                                            update::ModifyReturn returnMode,
                                            Transaction* tx)
{
    HATN_CTX_SCOPE("rdbfindupdatecreate")

    ENSURE_MODEL_SCHEMA

    auto rdbModel=model.nativeModel<RocksdbModel>();
    Assert(rdbModel,"Model not registered");

    return rdbModel->findUpdateCreate(*d->handler,query,request,object,returnMode,tx);
}

//---------------------------------------------------------------

Result<DbObject>
RocksdbClient::doFindOne(
        const ModelInfo& model,
        const ModelIndexQuery& query
    )
{
    HATN_CTX_SCOPE("rdbfindone")

    ENSURE_MODEL_SCHEMA

    auto rdbModel=model.nativeModel<RocksdbModel>();
    Assert(rdbModel,"Model not registered");

    auto r=rdbModel->find(*d->handler,query,true);
    HATN_CHECK_RESULT(r)
    if (r->empty())
    {
        return Result<DbObject>{DbObject{}};
    }
    return r->at(0);
}

//---------------------------------------------------------------

Error RocksdbClient::doFindCb(
        const ModelInfo& model,
        const ModelIndexQuery& query,
        const FindCb& cb,
        Transaction* tx,
        bool forUpdate
    )
{
    HATN_CTX_SCOPE("rdbfindcb")

    ENSURE_MODEL_SCHEMA

    auto rdbModel=model.nativeModel<RocksdbModel>();
    Assert(rdbModel,"Model not registered");

    return rdbModel->findCb(*d->handler,query,cb,tx,forUpdate);
}

//---------------------------------------------------------------

Error RocksdbClient::doDeleteTopic(Topic topic)
{
    return d->handler->deleteTopic(topic);
}

//---------------------------------------------------------------

std::shared_ptr<ClientEnvironment> RocksdbClient::doCloneEnvironment()
{
    return d->env;
}

//---------------------------------------------------------------

Result<std::pmr::set<TopicHolder>> RocksdbClient::doListModelTopics(
        const ModelInfo& model,
        const common::DateRange& partitionDateRange,
        bool onlyDefaultPartition
    )
{
    HATN_CTX_SCOPE("rdbcountmodel")

    auto rdbModel=model.nativeModel<RocksdbModel>();
    Assert(rdbModel,"Model not registered");

    std::shared_ptr<RocksdbPartition> partition;
    if (partitionDateRange.isNull())
    {
        if (onlyDefaultPartition)
        {
            partition=d->handler->defaultPartition();
        }
    }
    else
    {
        partition=d->handler->partition(partitionDateRange);
        if (!partition)
        {
            return std::pmr::set<TopicHolder>{rdbModel->factory()->objectAllocator<TopicHolder>()};
        }
    }

    return ModelTopics::modelTopics(model.modelIdStr(),*d->handler,partition.get(),onlyDefaultPartition,rdbModel->factory());
}

//---------------------------------------------------------------

HATN_ROCKSDB_NAMESPACE_END
