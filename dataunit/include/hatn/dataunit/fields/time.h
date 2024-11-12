/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file dataunit/time.h
  *
  *      DataUnit field of Time type.
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITTIME_H
#define HATNDATAUNITTIME_H

#include <type_traits>

#include <hatn/common/time.h>

#include <hatn/dataunit/stream.h>
#include <hatn/dataunit/fields/custom.h>
#include <hatn/dataunit/fields/fieldtraits.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

//---------------------------------------------------------------

class TimeTraits
{
    public:

        using TYPE=TYPE_TIME;
        using type=common::Time;

        using maxSize=std::integral_constant<int,sizeof(uint64_t)+1>;

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
            return VariableSer<uint64_t>::serialize(val.toNumber(),wired);
        }

        template <typename BufferT>
        static bool deserialize(type& val, BufferT& wired, HATN_DATAUNIT_NAMESPACE::AllocatorFactory* =nullptr)
        {
            uint64_t num=0;
            auto ok=VariableSer<uint64_t>::deserialize(num,wired);
            if (!ok)
            {
                return false;
            }
            auto ec=val.set(num);
            return !ec;
        }

        static void fieldClear(type& value)
        {
            value.reset();
        }
};

using TimeField=CustomField<TimeTraits>;

//---------------------------------------------------------------

template <>
struct FieldTmpl<TYPE_TIME> : public TimeField
{
    using TimeField::TimeField;

    void setV(const common::Time& val) override
    {
        this->set(val);
    }

    void getV(common::Time& val) const override
    {
        val=m_value;
    }

    bool less(const common::Time& val) const override
    {
        return m_value<val;
    }

    bool equals(const common::Time& val) const override
    {
        return m_value==val;
    }
};

//---------------------------------------------------------------

namespace json {

//! JSON read handler for Time fields
template <typename TYPE,typename FieldType>
struct FieldReader<TYPE,
                   FieldType,
                   std::enable_if_t<
                       !FieldType::isRepeatedType::value
                       &&
                       std::is_same<TYPE,TYPE_TIME>::value
                       >
                   > : public FieldReaderBase<FieldType>
{
    using json::FieldReaderBase<FieldType>::FieldReaderBase;

    bool String(const typename FieldReaderBase<FieldType>::Ch* data, SizeType size, bool)
    {
        auto r=common::Time::parse(common::lib::string_view(data,size));
        HATN_BOOL_RESULT(r)
        this->m_field->set(r.takeValue());
        return true;
    }
};

//! JSON read handler for repeatable Time fields
template <typename TYPE,typename FieldType>
struct FieldReader<TYPE,
                   FieldType,
                   std::enable_if_t<
                       FieldType::isRepeatedType::value
                       &&
                       std::is_same<TYPE,types::TYPE_TIME>::value
                       >
                   > : public FieldReaderBase<FieldType>
{
    using json::FieldReaderBase<FieldType>::FieldReaderBase;

    bool StartArray()
    {
        pushHandler<FieldType,FieldReader<TYPE,FieldType>>(this->m_topUnit,this->m_field,this->m_scopes);
        return true;
    }

    bool String(const typename FieldReaderBase<FieldType>::Ch* data, SizeType size, bool)
    {
        pushHandler<FieldType,FieldReader<TYPE,FieldType>>(this->m_topUnit,this->m_field,this->m_scopes);
        auto r=common::Time::parse(lib::string_view{data,size});
        HATN_BOOL_RESULT(r)
        this->m_field->addValue(r.takeValue());
        return true;
    }

    bool EndArray(SizeType)
    {
        return true;
    }
};


template <typename T,typename Type>
struct Fieldwriter<T,Type,std::enable_if_t<std::is_same<common::Time,std::decay_t<T>>::value>>
{
    static bool write(const T& val,json::Writer* writer)
    {
        auto str=val.toString();
        return writer->String(str.data(),str.size());
    }
};

} // namespace json

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITTIME_H
