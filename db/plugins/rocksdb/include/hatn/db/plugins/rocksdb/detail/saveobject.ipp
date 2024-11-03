/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/detail/saveobject.ipp
  *
  *   RocksDB save oject.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBSAVEBJECT_IPP
#define HATNROCKSDBSAVEBJECT_IPP

#include <hatn/common/pmr/allocatorfactory.h>

#include <hatn/logcontext/contextlogger.h>

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/wirebufsolid.h>

#include <hatn/db/dberror.h>
#include <hatn/db/namespace.h>
#include <hatn/db/transaction.h>

#include <hatn/db/plugins/rocksdb/rocksdberror.h>
#include <hatn/db/plugins/rocksdb/rocksdbhandler.h>
#include <hatn/db/plugins/rocksdb/ttlmark.h>
#include <hatn/db/plugins/rocksdb/detail/rocksdbhandler.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbkeys.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbindexes.ipp>
#include <hatn/db/plugins/rocksdb/detail/ttlindexes.ipp>
#include <hatn/db/plugins/rocksdb/detail/objectpartition.ipp>
#include <hatn/db/plugins/rocksdb/detail/rocksdbtransaction.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

template <typename ObjectT>
Error serializeObject(const ObjectT* obj, dataunit::WireBufSolid& buf)
{
    if (!obj->wireDataKeeper())
    {
        Error ec;
        dataunit::io::serialize(*obj,buf,ec);
        if(ec)
        {
            HATN_CTX_SCOPE_ERROR("serialize object");
            return ec;
        }
    }
    else
    {
        buf=obj->wireDataKeeper()->toSolidWireBuf();
    }
    return Error{OK};
}

inline Error saveObject(ROCKSDB_NAMESPACE::Transaction* tx, RocksdbPartition* partition, const ROCKSDB_NAMESPACE::SliceParts& key, dataunit::WireBufSolid& buf, const ROCKSDB_NAMESPACE::Slice& ttlMark)
{
    ROCKSDB_NAMESPACE::Status status;
    if (ttlMark.size()==0)
    {        
        ROCKSDB_NAMESPACE::Slice objectValue{buf.mainContainer()->data(),buf.mainContainer()->size()};
        ROCKSDB_NAMESPACE::SliceParts objectValueSlices{&objectValue,1};
        status=tx->Put(partition->collectionCf.get(),key,objectValueSlices);
    }
    else
    {
        std::array<ROCKSDB_NAMESPACE::Slice,2> objectValueParts{
            ROCKSDB_NAMESPACE::Slice{buf.mainContainer()->data(),buf.mainContainer()->size()},
            ttlMark
        };
        ROCKSDB_NAMESPACE::SliceParts objectValueSlices{&objectValueParts[0],static_cast<int>(objectValueParts.size())};
        status=tx->Put(partition->collectionCf.get(),key,objectValueSlices);
    }

    if (!status.ok())
    {
        HATN_CTX_SCOPE_ERROR("save-object");
        return makeError(DbError::WRITE_OBJECT_FAILED,status);
    }

    return Error{OK};
}

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBSAVEBJECT_IPP
