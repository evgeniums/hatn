/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/detail/rocksdbcreateobject.ipp
  *
  *   RocksDB database template for object creating.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBCREATEOBJECT_IPP
#define HATNROCKSDBCREATEOBJECT_IPP

#include <hatn/common/runonscopeexit.h>

#include <hatn/db/dberror.h>
#include <hatn/db/plugins/rocksdb/detail/rocksdbhandler_p.h>
#include <hatn/db/plugins/rocksdb/rocksdbcreateobject.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

template <typename UnitT, typename ...Indexes>
void CreateObjectT::operator ()(RocksdbHandler& handler, const db::Namespace& ns, const db::Model<UnitT,Indexes...>, const UnitT& object, Error& ec) const
{
    if (handler.p()->readOnly)
    {
        setError(ec,DbError::DB_READ_ONLY);
        return;
    }

    auto rdb=handler.p()->transactionDb;

    // init transaction
    auto tx=rdb->BeginTransaction(handler.p()->writeOptions,handler.p()->transactionOptions);
    if (tx==nullptr)
    {
        setError(ec,DbError::TX_BEGIN_FAILED);
        return;
    }
    auto onExit=[&ec,&tx]()
    {
        rocksdb::Status status;
        if (ec)
        {
            status=tx->Rollback();
            if (!status.ok())
            {
                //! @todo report error
            }
        }
        else
        {
            status=tx->Commit();
            if (!status.ok())
            {
                //! @todo construct error
                ec=dbError(DbError::TX_CREATE_OBJECT_FAILED);
            }
        }
        delete tx;
    };
    auto scopeGuard=HATN_COMMON_NAMESPACE::makeScopeGuard(std::move(onExit));
    std::ignore=scopeGuard;

    // handle partition

    // check unique indexes

    // serialize object

    // write binary data to rocksdb

    // construct and write indexes to rocksdb
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBCREATEOBJECT_IPP
