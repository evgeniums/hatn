/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/ipp/rocksdbindexes.ipp
  *
  *   RocksDB indexes processing.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBINDEXES_IPP
#define HATNROCKSDBINDEXES_IPP

#include <rocksdb/db.h>

#include <hatn/common/format.h>

#include <hatn/logcontext/contextlogger.h>

#include <hatn/db/namespace.h>
#include <hatn/db/objectid.h>
#include <hatn/db/dberror.h>

#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

template <typename BufT=common::FmtAllocatedBufferChar>
class IndexesT
{
    public:

        using bufT=BufT;

        IndexesT(ROCKSDB_NAMESPACE::ColumnFamilyHandle* cf):m_cf(cf)
        {}

        template <typename AllocatorT>
        IndexesT(ROCKSDB_NAMESPACE::ColumnFamilyHandle* cf, const AllocatorT& alloc)
            : m_cf(cf),
              m_buf(alloc)
        {}

        void reset()
        {
            m_buf.clear();
        }

        const bufT& buf() const noexcept
        {
            return m_buf;
        }

        template <typename ModelT, typename UnitT>
        Error saveIndexes(const ModelT& model,
                const Namespace& ns,
                const ROCKSDB_NAMESPACE::Slice& objectId,
                const ROCKSDB_NAMESPACE::SliceParts& objectKey,
                UnitT* object
            )
        {
            return OK;
        }

    private:

        BufT m_buf;
        ROCKSDB_NAMESPACE::ColumnFamilyHandle* m_cf;
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBINDEXES_IPP
