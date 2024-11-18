/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/ipp/fieldvaluetobuf.ipp
  *
  *   Query field to buffer formatter for RocksDB.
  *
  */

/****************************************************************************/

#include <hatn/common/format.h>
#include <hatn/common/stdwrappers.h>
#include <hatn/common/datetime.h>

#include <hatn/db/objectid.h>
#include <hatn/db/query.h>

#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>
#include <hatn/db/plugins/rocksdb/detail/fieldtostringbuf.ipp>

HATN_ROCKSDB_NAMESPACE_BEGIN

template <typename BufT,typename EndpointT=hana::true_>
struct valueVisitor
{
    constexpr static const EndpointT endpoint{};

    template <typename T>
    void operator()(const query::Interval<T>& value) const
    {
        auto self=this;
        hana::eval_if(
            endpoint,
            [&](auto _)
            {
                switch (_(value).from.type)
                {
                case(query::IntervalType::Last):
                {
                    (*_(self))(query::Last);
                }
                break;

                case(query::IntervalType::First):
                {
                    (*_(self))(query::First);
                }
                break;

                default:
                {
                    fieldToStringBuf(_(self)->buf,_(value).from.value);
                }
                break;
                }
            },
            [&](auto _)
            {
                switch (_(value).to.type)
                {
                case(query::IntervalType::Last):
                {
                    (*_(self))(query::Last);
                }
                break;

                case(query::IntervalType::First):
                {
                    (*_(self))(query::First);
                }
                break;

                default:
                {
                    fieldToStringBuf(_(self)->buf,_(value).to.value);
                }
                break;
                }
            }
            );
        if (sep!=nullptr)
        {
            buf.append(*sep);
        }
    }

    template <typename T>
    void operator()(const query::VectorT<T>&) const
    {}

    void operator()(const query::VectorString&) const
    {}

    template <typename T>
    void operator()(const T& value) const
    {
        fieldToStringBuf(buf,value);
        if (sep!=nullptr)
        {
            buf.append(*sep);
        }
    }

    void operator()(const query::LastT&) const
    {
        // replace previous separator with SeparatorCharPlusStr
        buf[buf.size()-1]=SeparatorCharPlusStr[0];
    }

    void operator()(const query::FirstT&) const
    {}

    valueVisitor(BufT& buf, const lib::string_view& sep):buf(buf),sep(&sep)
    {}

    valueVisitor(BufT& buf):buf(buf),sep(nullptr)
    {}

    BufT& buf;
    const lib::string_view* sep;
};

template <typename EndpointT=hana::true_>
struct fieldValueToBufT
{
    template <typename BufT>
    void operator()(BufT& buf, const query::Field& field, const lib::string_view& sep) const
    {
        lib::variantVisit(valueVisitor<BufT,EndpointT>{buf,sep},field.value());
    }

    template <typename BufT>
    void operator()(BufT& buf, const query::Field& field) const
    {
        lib::variantVisit(valueVisitor<BufT,EndpointT>{buf},field.value());
    }
};
constexpr fieldValueToBufT<hana::true_> fieldValueToBuf{};

HATN_ROCKSDB_NAMESPACE_END
