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

template <typename UnitT, typename ...Indexes>
void CreateObjectT::operator ()(RocksdbHandler& handler, const db::Namespace& ns, const db::Model<UnitT,Indexes...>& model, const UnitT& object, Error& ec) const
{
    if (handler.p()->readOnly)
    {
        setError(ec,DbError::DB_READ_ONLY);
        return;
    }

    // handle partition
    auto partition=handler.p()->defaultPartition;
    if (model.definition.field(db::model::date_partition_mode).isSet())
    {
        auto mode=model.definition.field(db::model::date_partition_mode).value();

        // figure out partition key
        constexpr auto partitionField=datePartitionField(model.indexes);
        auto partitionKey=hana::eval_if(
            hana::is_nothing(partitionField),
            [&](auto _)
            {
                auto dt=_(object).field(db::object::created_at).value();
                return common::DateRange::dateToRangeNumber(dt,_(mode));
            },
            [&](auto _)
            {
                const auto& field=hana::value(_(partitionField)).datePartitionField;
                auto dt=_(object).field(field).value();
                return common::DateRange::dateToRangeNumber(dt,_(mode));
            }
        );

        auto r=handler.p()->partition(partitionKey);
        HATN_CHECK_EMPTY_RETURN(r)
        partition=r.takeValue();
    }

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

    // init transaction
    auto rdb=handler.p()->transactionDb;
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
                ec=dbError(DbError::TX_COMMIT_FAILED);
            }
        }
        delete tx;
    };
    auto scopeGuard=HATN_COMMON_NAMESPACE::makeScopeGuard(std::move(onExit));
    std::ignore=scopeGuard;

    // check unique indexes

    // write serialized object to rocksdb
    auto collectionCF=partition->collectionCF(ns.collection());
    //! @todo handle keys in one place
    auto objectKey=fmt::format("{}:{}:{}",ns.tenancyName(),ns.collection(),object.field(db::object::_id).c_str());
    ROCKSDB_NAMESPACE::Slice objectValue{buf.mainContainer()->data(),buf.mainContainer()->size()};
    auto status=rdb->Put(handler.p()->writeOptions,collectionCF,objectKey,objectValue);
    if (!status.ok())
    {
        //! @todo construct error
        ec=dbError(DbError::WRITE_OBJECT_FAILED);
        return;
    }

    // construct and write indexes to rocksdb

    // construct and write ttl indexes to rocksdb
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBCREATEOBJECT_IPP
