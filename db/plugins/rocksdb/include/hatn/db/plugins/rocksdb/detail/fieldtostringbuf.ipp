/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file db/plugins/rocksdb/detail/fieldtostringbuf.ipp
  *
  *  Defines field visitor for serialization field to string buffer for rocksdb indexes.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBFIELDSTRINGBUF_IPP
#define HATNROCKSDBFIELDSTRINGBUF_IPP

#include <hatn/common/stdwrappers.h>
#include <hatn/common/datetime.h>
#include <hatn/common/daterange.h>

#include <hatn/db/objectid.h>
#include <hatn/db/query.h>

#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

struct FieldToStringBufT
{
    template <typename BufT>
    void operator ()(BufT& buf, const lib::string_view& val) const
    {
        if (val.empty())
        {
            buf.append(EmptyCharStr);
        }
        else
        {
            buf.append(val);
        }
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
        auto ms=val.toEpochMs();
        fmt::format_to(std::back_inserter(buf),"{:010x}",ms);
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
    static void int64_(BufT& buf, const int64_t& val)
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
    void operator ()(BufT& buf, const int8_t& val) const
    {
        int64_(buf,val);
    }

    template <typename BufT>
    void operator ()(BufT& buf, const int16_t& val) const
    {
        int64_(buf,val);
    }

    template <typename BufT>
    void operator ()(BufT& buf, const int32_t& val) const
    {
        int64_(buf,val);
    }

    template <typename BufT>
    void operator ()(BufT& buf, const int64_t& val) const
    {
        int64_(buf,val);
    }

    template <typename BufT>
    void operator ()(BufT& buf, const uint8_t& val) const
    {
        uint64_(buf,val);
    }

    template <typename BufT>
    void operator ()(BufT& buf, const uint16_t& val) const
    {
        uint64_(buf,val);
    }

    template <typename BufT>
    void operator ()(BufT& buf, const uint32_t& val) const
    {
        uint64_(buf,val);
    }

    template <typename BufT>
    static void uint64_(BufT& buf, const uint64_t& val)
    {
        fmt::format_to(std::back_inserter(buf),"1{:016x}",val);
    }

    template <typename BufT>
    void operator ()(BufT& buf, const uint64_t& val) const
    {
        uint64_(buf,val);
    }

    template <typename BufT>
    void bool_(BufT& buf, const bool& val) const
    {
#if 0
        constexpr static lib::string_view True{"1",1};
        constexpr static lib::string_view False{"0",1};

        if (val)
        {
            buf.append(True);
        }
        else
        {
            buf.append(False);
        }
#else
        uint64_(buf,static_cast<uint64_t>(val));
#endif
    }

    template <typename BufT>
    void operator ()(BufT& buf, const bool& val) const
    {
        bool_(buf,val);
    }

    template <typename BufT>
    void operator ()(BufT& buf, const query::BoolValue& val) const
    {
        bool_(buf,val);
    }

    template <typename BufT>
    void operator ()(BufT& buf, const query::NullT&) const
    {
        buf.push_back(NullCharC);
    }

    template <typename BufT>
    void operator ()(BufT&, const query::FirstT&) const
    {
        Assert(false,"Must never call FieldToStringBuf for Firtst");
    }

    template <typename BufT>
    void operator ()(BufT&, const query::LastT&) const
    {
        Assert(false,"Must never call FieldToStringBuf for Last");
    }

    template <typename BufT>
    void operator ()(BufT& buf, const BufT& val) const
    {
        buf.append(val.data(),val.size());
    }

    template <typename BufT>
    void operator ()(BufT& buf, const common::pmr::string& val) const
    {
        constexpr static FieldToStringBufT f{};
        f(buf,lib::string_view{val.data(),val.size()});
    }

    template <typename BufT>
    void operator ()(BufT& buf, const std::string& val) const
    {
        constexpr static FieldToStringBufT f{};
        f(buf,lib::string_view{val});
    }

    template <typename BufT, typename T>
    void operator ()(BufT& buf, const T& val) const
    {
        hana::eval_if(
            std::is_convertible<std::decay_t<T>,lib::string_view>{},
            [&](auto _)
            {
                constexpr static FieldToStringBufT f{};
                f(_(buf),lib::string_view{_(val)});
            },
            [&](auto _)
            {
                constexpr auto isEnum=std::is_enum<std::decay_t<T>>::value;
                static_assert(std::is_enum<std::decay_t<T>>::value,"Unsupported value type");
                if constexpr (isEnum)
                {
                    int64_(_(buf),static_cast<int64_t>(_(val)));
                }
            }
        );
    }
};

constexpr FieldToStringBufT fieldToStringBuf{};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBFIELDSTRINGBUF_IPP
