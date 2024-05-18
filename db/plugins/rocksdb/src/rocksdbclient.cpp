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

#include <rocksdb/db.h>

#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/detail/syntax.ipp>

#include <hatn/base/configobject.h>

#include <hatn/db/plugins/rocksdb/rocksdbclient.h>

HATN_DB_USING

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

    std::unique_ptr<rocksdb::DB> db;

    RocksdbConfig cfg;
    RocksdbOptions opt;
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
    invokeOpenDb(config,ec,records,false);
}

//---------------------------------------------------------------

Error RocksdbClient::doCreateDb(const ClientConfig &config, base::config_object::LogRecords& records)
{
    Error ec;
    invokeOpenDb(config,ec,records,true);
    HATN_CHECK_EC(ec)
    invokeCloseDb(ec);
    return ec;
}

//---------------------------------------------------------------

Error RocksdbClient::doDestroyDb(const ClientConfig &config, base::config_object::LogRecords& records)
{
    // load config
    auto ec=d->cfg.loadLogConfig(config.main,config.mainPath,records);
    HATN_CHECK_EC(ec)

    // destroy database
    rocksdb::Options options;
    auto status = rocksdb::DestroyDB(d->cfg.config().field(rocksdb_config::dbpath).c_str(),options);
    if (!status.ok())
    {
        //! @todo handle error
        setError(ec,DbError::DB_DESTROY_FAILED);
    }

    // done
    return OK;
}

//---------------------------------------------------------------

void RocksdbClient::doCloseDb(Error &ec)
{
    invokeCloseDb(ec);
}

//---------------------------------------------------------------

void RocksdbClient::invokeOpenDb(const ClientConfig &config, Error &ec, base::config_object::LogRecords& records, bool createIfMissing)
{
    // load config
    ec=d->cfg.loadLogConfig(config.main,config.mainPath,records);
    if (ec)
    {
        return;
    }

    // load options
    //! @todo records with prefix?
    ec=d->opt.loadLogConfig(config.opt,config.optPath,records);
    if (ec)
    {
        return;
    }

    //! @todo use column families
    //! @todo fill options from configuration
    rocksdb::DB* db{nullptr};
    rocksdb::Options options;
    options.create_if_missing = createIfMissing;

    // open database
    rocksdb::Status status;
    if (d->opt.config().field(rocksdb_options::readonly).value())
    {
        status = rocksdb::DB::OpenForReadOnly(options, d->cfg.config().field(rocksdb_config::dbpath).c_str(), &db);
    }
    else
    {
        status = rocksdb::DB::Open(options, d->cfg.config().field(rocksdb_config::dbpath).c_str(), &db);
    }

    // check status
    if (!status.ok())
    {
        //! @todo handle error
        if (createIfMissing)
        {
            setError(ec,DbError::DB_CREATE_FAILED);
        }
        else
        {
            setError(ec,DbError::DB_OPEN_FAILED);
        }
        return;
    }

    // done
    d->db.reset(db);
}

//---------------------------------------------------------------

void RocksdbClient::invokeCloseDb(Error &ec)
{
    ec.reset();
    if (d->db)
    {
        rocksdb::Status status;
        if (d->cfg.config().field(rocksdb_config::wait_compact_shutdown).value())
        {
            auto opt = rocksdb::WaitForCompactOptions();
            opt.close_db = true;
            status = d->db->WaitForCompact(opt);
        }
        else
        {
            status=d->db->Close();
        }

        // check status
        if (!status.ok())
        {
            //! @todo handle error
            setError(ec,DbError::DB_CLOSE_FAILED);
        }

        // done
        d->db.reset();
    }
}

//---------------------------------------------------------------

HATN_ROCKSDB_NAMESPACE_END
