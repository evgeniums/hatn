/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file callgraph/stacklogrecord.h
  *
  *  Defines StackLogRecord.
  *
  */

/****************************************************************************/

#ifndef HATNSTACKLOGRECORD_H
#define HATNSTACKLOGRECORD_H

#include <hatn/common/stdwrappers.h>
#include <hatn/common/datetime.h>
#include <hatn/common/fixedbytearray.h>

#include <hatn/callgraph/callgraph.h>

HATN_CALLGRAPH_NAMESPACE_BEGIN

constexpr size_t StackLogKeyLength=16;
constexpr size_t StackLogValueLength=64;

template <size_t Length=StackLogValueLength>
using StackLogValueT=common::lib::variant<
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

using StackLogValue=StackLogValueT<>;

namespace detail {

struct LogValueSerializer
{
    LogValueSerializer(common::FmtAllocatedBufferChar &buf):buf(buf)
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
    detail::LogValueSerializer s(buf);
    lib::variantVisit(s,v);
}

template <size_t ValueLength=StackLogValueLength, size_t KeyLength=StackLogKeyLength>
using StackLogRecord=std::pair<common::FixedByteArray<KeyLength>,StackLogValueT<ValueLength>>;

HATN_CALLGRAPH_NAMESPACE_END

#endif // HATNSTACKLOGRECORD_H
