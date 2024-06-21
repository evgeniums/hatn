/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file logcontext/stacklogrecord.h
  *
  *  Defines types and helpers of logcontext records.
  *
  */

/****************************************************************************/

#ifndef HATNLOGRECORD_H
#define HATNLOGRECORD_H

#include <hatn/common/stdwrappers.h>
#include <hatn/common/datetime.h>
#include <hatn/common/fixedbytearray.h>

#include <hatn/logcontext/logcontext.h>

HATN_LOGCONTEXT_NAMESPACE_BEGIN

constexpr size_t MaxKeyLength=16;
constexpr size_t MaxValueLength=64;

template <size_t Length=MaxValueLength>
using ValueT=common::lib::variant<
        int8_t,
        uint8_t,
        int16_t,
        uint16_t,
        int32_t,
        uint32_t,
        int64_t,
        uint64_t,
        float,
        double,
        common::DateTime,
        common::Date,
        common::Time,
        common::DateRange,
        common::FixedByteArray<Length>
    >;

using Value=ValueT<>;

namespace detail {

struct ValueSerializer
{
    ValueSerializer(common::FmtAllocatedBufferChar &buf):buf(buf)
    {}

    common::FmtAllocatedBufferChar &buf;

    void operator()(int8_t v)
    {
        fmt::format_to(std::back_inserter(buf),"{:d}",v);
    }

    void operator()(uint8_t v)
    {
        fmt::format_to(std::back_inserter(buf),"{:d}",v);
    }

    void operator()(int16_t v)
    {
        fmt::format_to(std::back_inserter(buf),"{:d}",v);
    }

    void operator()(uint16_t v)
    {
        fmt::format_to(std::back_inserter(buf),"{:d}",v);
    }

    void operator()(int32_t v)
    {
        fmt::format_to(std::back_inserter(buf),"{:d}",v);
    }

    void operator()(uint32_t v)
    {
        fmt::format_to(std::back_inserter(buf),"{:d}",v);
    }

    void operator()(int64_t v)
    {
        if ((v>int64_t(0xFFFFFFFF)) | (v<(-int64_t(0xFFFFFFFF))))
        {
            fmt::format_to(std::back_inserter(buf),"{:#x}",v);
        }
        else
        {
            fmt::format_to(std::back_inserter(buf),"{:d}",v);
        }
    }

    void operator()(uint64_t v)
    {
        if (v>uint64_t(0xFFFFFFFF))
        {
            fmt::format_to(std::back_inserter(buf),"{:#x}",v);
        }
        else
        {
            fmt::format_to(std::back_inserter(buf),"{:d}",v);
        }
    }

    void operator()(float v)
    {
        fmt::format_to(std::back_inserter(buf),"{:g}",v);
    }

    void operator()(double v)
    {
        fmt::format_to(std::back_inserter(buf),"{:g}",v);
    }

    void operator()(const common::DateTime& v)
    {
        v.serialize(buf);
    }

    void operator()(const common::Date& v)
    {
        v.serialize(buf);
    }

    void operator()(const common::Time& v)
    {
        v.serialize(buf);
    }

    void operator()(const common::DateRange& v)
    {
        v.serialize(buf);
    }

    template <size_t Length>
    void operator()(const common::FixedByteArray<Length>& v)
    {
        buf.append(v);
    }
};

}

template <typename T>
void serializeValue(common::FmtAllocatedBufferChar &buf, const T& v)
{
    detail::ValueSerializer s(buf);
    lib::variantVisit(s,v);
}

template <size_t KeyLength=MaxKeyLength>
using KeyT=common::FixedByteArray<KeyLength>;

using Key=KeyT<>;

template <typename ValueT=Value, typename KeyT=Key>
using RecordT=std::pair<KeyT,ValueT>;

using Record=RecordT<>;

HATN_LOGCONTEXT_NAMESPACE_END

#endif // HATNLOGRECORD_H
