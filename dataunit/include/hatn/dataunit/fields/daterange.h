/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file dataunit/daterange.h
  *
  *      DataUnit field of DateRange type.
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITDATERANGE_H
#define HATNDATAUNITDATERANGE_H

#include <type_traits>

#include <hatn/common/daterange.h>

#include <hatn/dataunit/stream.h>
#include <hatn/dataunit/fields/custom.h>
#include <hatn/dataunit/fields/fieldtraits.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

//---------------------------------------------------------------

class DateRangeTraits
{
    public:

        using TYPE=TYPE_DATE_RANGE;
        using type=common::DateRange;

        using maxSize=std::integral_constant<int,sizeof(uint32_t)+1>;

        static size_t valueSize(const type&) noexcept
        {
            return maxSize::value;
        }

        constexpr static size_t fieldSize() noexcept
        {
            return maxSize::value;
        }

        template <typename BufferT>
        static bool serialize(const type& val, BufferT& wired)
        {
            return VariableSer<uint32_t>::serialize(val.value(),wired);
        }

        template <typename BufferT>
        static bool deserialize(type& val, BufferT& wired, HATN_DATAUNIT_NAMESPACE::AllocatorFactory* =nullptr)
        {
            uint32_t num=0;
            auto ok=VariableSer<uint32_t>::deserialize(num,wired);
            if (!ok)
            {
                return false;
            }
            val.set(num);
            return true;
        }

        static void fieldClear(type& value)
        {
            value.reset();
        }
};

using DateRangeField=CustomField<DateRangeTraits>;

//---------------------------------------------------------------

template <>
struct FieldTmpl<TYPE_DATE_RANGE> : public DateRangeField
{
    using DateRangeField::DateRangeField;

    void setV(const common::DateRange& val) override
    {
        this->set(val);
    }

    void getV(common::DateRange& val) const override
    {
        val=m_value;
    }

    bool less(const common::DateRange& val) const override
    {
        return m_value<val;
    }

    bool equals(const common::DateRange& val) const override
    {
        return m_value==val;
    }
};

//---------------------------------------------------------------

namespace json {

//! @todo Add repeated DateRange field

//! JSON read handler for DateRangeTime fields
template <typename TYPE,typename FieldType>
struct FieldReader<TYPE,
                   FieldType,
                   std::enable_if_t<
                       !FieldType::isRepeatedType::value
                       &&
                       std::is_same<TYPE,TYPE_DATE_RANGE>::value
                       >
                   > : public FieldReaderBase<FieldType>
{
    using json::FieldReaderBase<FieldType>::FieldReaderBase;

    bool String(const typename FieldReaderBase<FieldType>::Ch* data, SizeType size, bool)
    {
        auto r=common::DateRange::fromString(common::lib::string_view(data,size));
        if (!r)
        {
            this->m_field->set(r.takeValue());
            return true;
        }
        return false;
    }
};

template <typename T>
struct Fieldwriter<T,std::enable_if_t<std::is_same<common::DateRange,std::decay_t<T>>::value>>
{
    static bool write(const T& val,json::Writer* writer)
    {
        auto str=val.toString();
        return writer->String(str.data(),str.size());
    }
};

} // namespace json

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITDATERANGE_H
