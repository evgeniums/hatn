/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/ttlvalue.h
  *
  *   Helper for ttl marks.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBTTLMARK_H
#define HATNROCKSDBTTLMARK_H

#include <rocksdb/db.h>

#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

class HATN_ROCKSDB_SCHEMA_EXPORT TtlMark
{
    public:

        constexpr static size_t Size=5;

        template <typename ModelT, typename UnitT>
        TtlMark(
                    const ModelT& model,
                    const UnitT* object
            ) : m_size(1)
        {
            //! @todo extract ttl from object

            m_tp[0]=0;
        }

        ROCKSDB_NAMESPACE::Slice slice() const noexcept
        {
            return ROCKSDB_NAMESPACE::Slice{m_tp.data(),m_size};
        }

        static uint32_t currentTimepoint() noexcept;

        static ROCKSDB_NAMESPACE::Slice currentTimepointSlice() noexcept;

        static void refreshCurrentTimepoint();

        static bool isExpired(uint32_t tp) noexcept;

        static bool isExpired(const char* data) noexcept;

        static bool isExpired(const ROCKSDB_NAMESPACE::Slice* slice) noexcept;

    private:

        size_t m_size;
        std::array<char,5> m_tp;
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBTTLMARK_H
