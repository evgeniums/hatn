/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/ipp/rocksdbkeys.ipp
  *
  *   RocksDB keys processing.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBKEYS_IPP
#define HATNROCKSDBKEYS_IPP

#include <rocksdb/db.h>

#include <hatn/common/format.h>

#include <hatn/db/objectid.h>
#include <hatn/db/namespace.h>

#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

template <typename BufT=common::FmtAllocatedBufferChar>
class KeysT
{
    public:

        using bufT=BufT;

        KeysT()
        {}

        template <typename AllocatorT>
        KeysT(const AllocatorT& alloc):m_buf(alloc)
        {}

        void reset()
        {
            m_buf.clear();
        }

        const bufT& buf() const noexcept
        {
            return m_buf;
        }

        template <typename ModelT>
        auto makeObjectKey(const ModelT& model,
                           const Namespace& ns,
                           const ROCKSDB_NAMESPACE::Slice& objectId)
        {
            std::array<ROCKSDB_NAMESPACE::Slice,3> parts;

            parts[0]=ROCKSDB_NAMESPACE::Slice{ns.topic().data(),ns.topic().size()};
            parts[1]=ROCKSDB_NAMESPACE::Slice{model.modelIdStr().data(),model.modelIdStr().size()};
            parts[2]=objectId;

            return parts;
        }

    private:

        BufT m_buf;
};

using Keys=KeysT<>;

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBKEYS_H
