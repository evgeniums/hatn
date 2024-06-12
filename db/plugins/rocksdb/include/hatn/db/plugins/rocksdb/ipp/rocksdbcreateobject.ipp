/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/ipp/rocksdbcreateobject.ipp
  *
  *   RocksDB database template for object creating.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBCREATEOBJECT_IPP
#define HATNROCKSDBCREATEOBJECT_IPP

#include <hatn/common/runonscopeexit.h>

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/wirebufsolid.h>

#include <hatn/db/dberror.h>
#include <hatn/db/plugins/rocksdb/detail/rocksdbhandler_p.h>
#include <hatn/db/plugins/rocksdb/rocksdbcreateobject.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

template <typename ConfigT, typename UnitT, typename ...Indexes>
void CreateObjectT::operator ()(RocksdbHandler& handler, const db::Namespace& ns, const db::Model<ConfigT,UnitT,Indexes...>& model, const UnitT& object, Error& ec) const
{
    using modelType=std::decay_t<db::Model<ConfigT,UnitT,Indexes...>>;

    if (handler.p()->readOnly)
    {
        setError(ec,DbError::DB_READ_ONLY);
        return;
    }

    // handle partition
    auto r=hana::eval_if(
        hana::bool_<modelType::isDatePartitioned()>{},
        [&](auto _)
        {
            auto partitionRange=datePartition(_(object),_(model));
            return handler.p()->partition(partitionRange.value());
        },
        [&](auto _)
        {
            return Result<RocksdbPartition*>{_(handler).p()->defaultPartition};
        }
    );
    if (r)
    {
        ec=r.takeError();
        return;
    }
    auto&& partition=r.takeValue();

    // serialize object
    dataunit::WireBufSolid buf;
    if (!object.wireDataKeeper())
    {
        dataunit::io::serialize(object,buf,ec);
        HATN_CHECK_EMPTY_RETURN(ec)
    }
    else
    {
        buf=object.wireDataKeeper()->toSolidWireBuf();
    }

    // transaction fn
    auto transactionFn=[&]()
    {
        auto rdb=handler.p()->transactionDb;
        auto objectId=object.field(db::object::_id).value().toString();

        // check unique indexes

        // write serialized object to rocksdb
        auto collectionCF=partition->collectionCF(ns.collection());
        //! @todo handle keys in one place
        auto objectKey=fmt::format("{}:{}:{}",ns.collection(),objectId);
        ROCKSDB_NAMESPACE::Slice objectValue{buf.mainContainer()->data(),buf.mainContainer()->size()};
        auto status=rdb->Put(handler.p()->writeOptions,collectionCF,objectKey,objectValue);
        if (!status.ok())
        {
            //! @todo construct error
            return dbError(DbError::WRITE_OBJECT_FAILED);
        }

        // construct and write indexes to rocksdb

        // construct and write ttl indexes to rocksdb
    };

    // invoke transaction
    ec=handler->transaction(transactionFn,false);
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBCREATEOBJECT_IPP
