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

#include <hatn/db/plugins/rocksdb/rocksdbclient.h>

HATN_DB_USING

HATN_ROCKSDB_NAMESPACE_BEGIN

/********************** RocksdbClient **************************/

class RocksdbClient_p
{
    public:

    std::unique_ptr<rocksdb::DB> db;
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
        invokeClose(ec);
    }
}

//---------------------------------------------------------------

void RocksdbClient::doOpen(const ClientConfig &config, Error &ec)
{
    ec.reset();

    rocksdb::DB* db{nullptr};

    //! @todo use configuration
    //! @todo use column families
    rocksdb::Options options;
    options.create_if_missing = true;
    rocksdb::Status status = rocksdb::DB::Open(options, "/tmp/testdb", &db);

    if (status.ok())
    {
        d->db.reset(db);
    }

    setError(ec,DbError::OPEN_FAILED);
}

//---------------------------------------------------------------

void RocksdbClient::doClose(Error &ec)
{
    invokeClose(ec);
}

//---------------------------------------------------------------

void RocksdbClient::invokeClose(Error &ec)
{
    ec.reset();
    if (d->db)
    {
        //! @todo use closing options, e.g. wait for compact

        auto status=d->db->Close();
        if (!status.ok())
        {
            //! @todo handle error
            setError(ec,DbError::CLOSE_FAILED);
        }
        d->db.reset();
    }
}

//---------------------------------------------------------------

HATN_ROCKSDB_NAMESPACE_END
