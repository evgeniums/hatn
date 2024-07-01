/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file db/plugins/rocksdb/ipp/fieldtostringbuf.ipp
  *
  *  Defines field visitor for serialization field to string buffer for rocksdb indexes.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBFIELDSTRINGBUF_H
#define HATNROCKSDBFIELDSTRINGBUF_H

#include <hatn/common/stdwrappers.h>
#include <hatn/common/datetime.h>
#include <hatn/common/daterange.h>

#include <hatn/db/objectid.h>

#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

struct FieldToStringBufT
{
    template <typename BufT>
    void operator ()(BufT& buf, const lib::string_view& val) const
    {
        buf.append(val);
    }

    template <typename BufT>
    void operator ()(BufT& buf, const ObjectId& val) const
    {
        auto offset=buf.size();
        buf.resize(offset+ObjectId::Length);
        val.serialize(buf,offset);
    }

    template <typename BufT>
    void operator ()(BufT& buf, const common::DateTime& val) const
    {
        val.serialize(buf);
    }

    template <typename BufT>
    void operator ()(BufT& buf, const common::Time& val) const
    {
        val.serialize(buf,common::Time::FormatPrecision::Number);
    }

    template <typename BufT>
    void operator ()(BufT& buf, const common::Date& val) const
    {
        val.serialize(buf);
    }

    template <typename BufT>
    void operator ()(BufT& buf, const common::DateRange& val) const
    {
        val.serialize(buf);
    }

    template <typename BufT>
    void operator ()(BufT& buf, const int8_t& val) const
    {
        if (val<0)
        {
            fmt::format_to(std::back_inserter(buf),"0{:02x}",uint8_t(val));
        }
        else
        {
            fmt::format_to(std::back_inserter(buf),"1{:02x}",val);
        }
    }

    template <typename BufT>
    void operator ()(BufT& buf, const int16_t& val) const
    {
        if (val<0)
        {
            fmt::format_to(std::back_inserter(buf),"0{:04x}",uint16_t(val));
        }
        else
        {
            fmt::format_to(std::back_inserter(buf),"1{:04x}",val);
        }
    }

    template <typename BufT>
    void operator ()(BufT& buf, const int32_t& val) const
    {
        if (val<0)
        {
            fmt::format_to(std::back_inserter(buf),"0{:08x}",uint32_t(val));
        }
        else
        {
            fmt::format_to(std::back_inserter(buf),"1{:08x}",val);
        }
    }

    template <typename BufT>
    void operator ()(BufT& buf, const int64_t& val) const
    {
        if (val<0)
        {
            fmt::format_to(std::back_inserter(buf),"0{:016x}",uint64_t(val));
        }
        else
        {
            fmt::format_to(std::back_inserter(buf),"1{:016x}",val);
        }
    }

    template <typename BufT>
    void operator ()(BufT& buf, const uint8_t& val) const
    {
        fmt::format_to(std::back_inserter(buf),"{:02x}",val);
    }

    template <typename BufT>
    void operator ()(BufT& buf, const uint16_t& val) const
    {
        fmt::format_to(std::back_inserter(buf),"{:04x}",val);
    }

    template <typename BufT>
    void operator ()(BufT& buf, const uint32_t& val) const
    {
        fmt::format_to(std::back_inserter(buf),"{:08x}",val);
    }

    template <typename BufT>
    void operator ()(BufT& buf, const uint64_t& val) const
    {
        fmt::format_to(std::back_inserter(buf),"{:016x}",val);
    }

    template <typename BufT>
    void operator ()(BufT& buf, const bool& val) const
    {
        constexpr static lib::string_view True{"1",1};
        constexpr static lib::string_view False{" ",1};

        if (val)
        {
            buf.append(True);
        }
        else
        {
            buf.append(False);
        }
    }

    template <typename BufT, typename T>
    void operator ()(BufT&, const T&) const
    {
        // for any other type, is_same here is just to use templtae typenames for static assert
        static_assert(std::is_same<T,BufT>::value,"Unsupported value type");
    }
};

constexpr FieldToStringBufT fieldToStringBuf{};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBFIELDSTRINGBUF_H
