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

        virtual Error doCreateDb(const ClientConfig& config, base::config_object::LogRecords& records) override;
        virtual Error doDestroyDb(const ClientConfig& config, base::config_object::LogRecords& records) override;

        void doOpenDb(const ClientConfig& config, Error& ec, base::config_object::LogRecords& records) override;
        void doCloseDb(Error& ec) override;

    private:

        std::unique_ptr<RocksdbClient_p> d;
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBCLIENT_H
