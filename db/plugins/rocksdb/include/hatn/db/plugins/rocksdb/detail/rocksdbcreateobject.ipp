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

    // handle _id
    if (!object.field(db::object::_id).isSet())
    {
        auto& obj=const_cast<UnitT&>(object);
        auto& id=obj.field(db::object::_id);
        ec=db::GenerateId({id.dataPtr(),id.DataSize()});
        HATN_CHECK_EMPTY_RETURN(ec)
        obj.resetWireDataKeeper();
    }

    // handle creation time
    if (!object.field(db::object::created_at).isSet())
    {
        auto& obj=const_cast<UnitT&>(object);
        auto& f=obj.field(db::object::created_at);
        //! @todo generate created_at
        obj.resetWireDataKeeper();
    }

    // handle update time
    if (!object.field(db::object::updated_at).isSet())
    {
        auto& obj=const_cast<UnitT&>(object);
        auto& c=obj.field(db::object::created_at);
        auto& u=obj.field(db::object::updated_at);
        u.set(c.value());
        obj.resetWireDataKeeper();
    }

    // handle partition
    auto partition=handler.p()->defaultPartition;
    if (model.definition.field(db::model::partition).isSet())
    {
        //! @todo figure out partition key by creation time
        uint32_t partitionKey{0};
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
