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

HDU_UNIT_WITH(ttl_index,(HDU_BASE(object)),
    HDU_FIELD(ref_id,TYPE_OBJECT_ID,1)
    HDU_FIELD(ref_collection,HDU_TYPE_FIXED_STRING(8),2)
    HDU_FIELD(date_range,TYPE_DATE_RANGE,3)
)

class HATN_ROCKSDB_SCHEMA_EXPORT TtlMark
{
    public:

        constexpr static size_t Size=5;

        template <typename ModelT, typename UnitT>
        TtlMark(
                const ModelT&,
                const UnitT* object
            ) : m_size(1)
        {
            using modelT=std::decay_t<ModelT>;
            constexpr static auto ttlIndexes=modelT::ttlIndexes();

            auto self=this;
            hana::eval_if(
                hana::is_empty(ttlIndexes),
                [&](auto _)
                {
                    _(self)->m_expireAt[0]=0;
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
                            auto tp=field.value().toEpoch();
                            auto exp=tp+idxT::ttl();
                            return (exp<prev)?exp:prev;
                        }
                    );
                    _(self)->fillExpireAt(expireAt);
                }
            );
        }

        void fillExpireAt(uint32_t expireAt);

        ROCKSDB_NAMESPACE::Slice slice() const noexcept
        {
            return ROCKSDB_NAMESPACE::Slice{m_expireAt.data(),m_size};
        }

        static uint32_t currentTimepoint() noexcept;

        static void refreshCurrentTimepoint();

        static bool isExpired(uint32_t tp) noexcept;

        static bool isExpired(const char *data, size_t size) noexcept;

        static bool isExpired(const ROCKSDB_NAMESPACE::Slice* slice) noexcept
        {
            return isExpired(slice->data(),slice->size());
        }

    private:

        size_t m_size;
        std::array<char,5> m_expireAt; // 4 bytes for uint timepoint + 1 byte for flag (1 - expire, 0 - ttl disabled)
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBTTLMARK_H
