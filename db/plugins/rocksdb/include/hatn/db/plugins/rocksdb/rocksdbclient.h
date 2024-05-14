/*
   Copyright (c) 2019 - current, Evgeny Sidorov (esid1976@gmail.com), All rights reserved
    
    
  */

/****************************************************************************/
/*
    
*/
/** \file db/plugins/rocksdb/rocksdbclient.h
  *
  *   RocksDB database client.
  *
  */

/****************************************************************************/

#ifndef DRACOSHAROCKSDBCLIENT_H
#define DRACOSHAROCKSDBCLIENT_H

#include <dracosha/db/dbclient.h>

namespace dracosha {
namespace db {
namespace rocksdbdriver {

class RocksDbClient : public DbClient
{
    public:

        using DbClient::DbClient;

#if 0
        /**
          @brief Destructor.
          */
        virtual ~RocksDbClient()=default;

        /**
         * @brief Open database.
         * @return Operation status.
         */
        virtual common::Error open() =0;

        /**
         * @brief Close database.
         * @return Operation status.
         */
        virtual common::Error close() =0;

        /**
         * @brief Flush in-memory temporary data to database.
         * @return Operation status.
         */
        virtual common::Error flush() =0;

        /**
         * @brief Begin database transaction.
         * @return Operation status.
         */
        virtual common::Error beginTransaction() =0;

        /**
         * @brief Commit database transaction.
         * @return Operation status.
         */
        virtual common::Error commitTransaction() =0;

        /**
         * @brief Rollback database transaction.
         * @return Operation status.
         */
        virtual common::Error rollbackTransaction() =0;

        // set
        // get
        // mset
        // mget
        // remove

        // setSchema
        // getSchema
        // checkSchema
        // updateSchema
#endif
};

} // namespace rocksdbdriver
} // namespace db
} // namespace dracosha

#endif // DRACOSHAROCKSDBCLIENT_H
