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

#include <limits>

#include <rocksdb/db.h>

#include <hatn/common/datetime.h>

#include <hatn/dataunit/syntax.h>

#include <hatn/db/objectid.h>
#include <hatn/db/object.h>
#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

class HATN_ROCKSDB_SCHEMA_EXPORT TtlMark
{
    public:

        constexpr static size_t Size=5;
        using ExpireAt=std::array<char,Size>;
        constexpr static size_t MinSize=1;

        TtlMark(): m_size(0)
        {}

        bool isNull() const noexcept
        {
            return m_size==0;
        }

        void load(const char* data, size_t size)
        {
            Assert(size<=Size,"Invalid data size to load TTL");
            memcpy(m_expireAt.data(),data,size);
            m_size=size;
        }

        size_t copy(char* data, size_t maxSize) const
        {
            Assert(maxSize>=m_size,"Invalid data size to copy from TTL");
            memcpy(data,m_expireAt.data(),m_size);
            return m_size;
        }

        size_t size() const noexcept
        {
            return m_size;
        }

        template <typename ModelT, typename UnitT>
        void fill(
                const ModelT&,
                const UnitT* object
            )
        {
            TtlMark::refreshCurrentTimepoint();
            m_size=1;
            using modelT=std::decay_t<ModelT>;
            constexpr static auto ttlIndexes=modelT::ttlIndexes();

            auto self=this;
            hana::eval_if(
                hana::is_empty(ttlIndexes),
                [&](auto _)
                {
                    _(self)->fillExpireAt(0);
                },
                [&](auto _)
                {
                    auto expireAt=hana::fold(
                        ttlIndexes,
                        std::numeric_limits<uint32_t>::max(),
                        [&](auto prev, auto&& idx)
                        {
                            using idxC=std::decay_t<decltype(idx)>;
                            using idxT=typename idxC::type;
                            const auto& field=getPlainIndexField(*_(object),idx);
                            if (!field.isSet())
                            {
                                return prev;
                            }
                            auto tp=field.value().toEpoch();
                            if (tp==0)
                            {
                                return prev;
                            }
                            auto exp=tp+idxT::ttl();
                            return (exp<prev)?exp:prev;
                        }
                        );
                    if (expireAt==std::numeric_limits<uint32_t>::max())
                    {
                        expireAt=0;
                    }
                    _(self)->fillExpireAt(expireAt);
                }
            );
        }

        template <typename ModelT, typename UnitT>
        TtlMark(
                const ModelT& model,
                const UnitT* object
            ) : m_size(1)
        {
            fill(model,object);
        }

        void fillExpireAt(uint32_t expireAt);

        ROCKSDB_NAMESPACE::Slice slice() const noexcept
        {
            return ROCKSDB_NAMESPACE::Slice{m_expireAt.data(),m_size};
        }

        uint32_t timepoint() const noexcept
        {
            return ttlMarkTimepoint(m_expireAt.data(),m_size);
        }

        static uint32_t currentTimepoint() noexcept;

        static void refreshCurrentTimepoint();

        static bool isExpired(uint32_t tp, uint32_t currentTp=0) noexcept;

        static bool isExpired(const char *data, size_t size, uint32_t currentTp=0) noexcept;

        static bool isExpired(const ROCKSDB_NAMESPACE::Slice& slice, uint32_t currentTp=0) noexcept
        {
            return isExpired(slice.data(),slice.size(),currentTp);
        }

        static bool isExpired(const ROCKSDB_NAMESPACE::PinnableSlice& slice, uint32_t currentTp=0) noexcept
        {
            return isExpired(slice.data(),slice.size(),currentTp);
        }

        static uint32_t ttlMarkTimepoint(const char *data, size_t size) noexcept;

        static size_t ttlMarkOffset(const char *data, size_t size) noexcept
        {
            if (size==0)
            {
                return 0;
            }
            if (data[size-1]==0)
            {
                return 1;
            }
            if (size<Size)
            {
                return 0;
            }

            return Size;
        }

        template <typename T>
        static ROCKSDB_NAMESPACE::Slice stripTtlMark(const T& slice) noexcept
        {
            auto offset=ttlMarkOffset(slice.data(),slice.size());
            return ROCKSDB_NAMESPACE::Slice{slice.data(),slice.size()-offset};
        }

        template <typename T>
        static ROCKSDB_NAMESPACE::Slice ttlMark(const T& slice) noexcept
        {
            auto offset=ttlMarkOffset(slice.data(),slice.size());
            return ROCKSDB_NAMESPACE::Slice{slice.data()+slice.size()-offset,offset};
        }                

    private:

        size_t m_size;
        ExpireAt m_expireAt; // 4 bytes for uint timepoint + 1 byte for flag (1 - expire, 0 - ttl disabled)
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBTTLMARK_H
