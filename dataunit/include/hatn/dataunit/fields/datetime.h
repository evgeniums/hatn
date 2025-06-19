/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/datetime.h
  *
  *      DataUnit field of DateTime type.
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITDATETIME_H
#define HATNDATAUNITDATETIME_H

#include <type_traits>

#include <hatn/common/datetime.h>

#include <hatn/dataunit/stream.h>
#include <hatn/dataunit/fields/fieldtraits.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

//---------------------------------------------------------------

namespace json {

//! JSON read handler for DateTime fields
template <typename TYPE,typename FieldType>
struct FieldReader<TYPE,
                   FieldType,
                   std::enable_if_t<
                       !FieldType::isRepeatedType::value
                       &&
                       std::is_same<TYPE,types::TYPE_DATETIME>::value
                       >
                   > : public FieldReaderBase<FieldType>
{
    using json::FieldReaderBase<FieldType>::FieldReaderBase;

    bool String(const typename FieldReaderBase<FieldType>::Ch* data, SizeType size, bool)
    {
        auto r=common::DateTime::parseIsoString(common::lib::string_view{data,size});
        HATN_BOOL_RESULT(r)
        this->m_field->set(r.takeValue());
        return true;
    }
};

//! JSON read handler for repeatable DateTime fields
template <typename TYPE,typename FieldType>
struct FieldReader<TYPE,
                   FieldType,
                   std::enable_if_t<
                       FieldType::isRepeatedType::value
                       &&
                       std::is_same<TYPE,types::TYPE_DATETIME>::value
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
        auto r=common::DateTime::parseIsoString(lib::string_view{data,size});
        HATN_BOOL_RESULT(r)
        this->m_field->appendValue(r.takeValue());
        return true;
    }

    bool EndArray(SizeType)
    {
        return true;
    }
};

template <typename T,typename Type>
struct Fieldwriter<T,Type,std::enable_if_t<std::is_same<common::DateTime,std::decay_t<T>>::value>>
{
    static bool write(const T& val,json::Writer* writer)
    {
        auto str=val.toIsoString();
        return writer->String(str.data(),str.size());
    }
};

}
//---------------------------------------------------------------

//! DateTime field.
class DateTime : public Field
{
    public:

        using Type=TYPE_DATETIME;

        using type=common::DateTime;

        using isRepeatedType=std::false_type;
        using isEnum=typename Type::isEnum;
        using Enum=typename Type::Enum;
        using selfType=DateTime;
        constexpr static auto typeId=Type::typeId;
//! @todo Fix isArray
#if 0
        constexpr static auto isArray=isRepeatedType{};
#endif

        using maxSize=std::integral_constant<int,sizeof(uint64_t)>;

        //! Ctor with default value
        DateTime(Unit* unit,type defaultValue)
            : Field(Type::typeId,unit),
            m_value(std::move(defaultValue))
        {}

        //! Ctor with parent unit
        explicit DateTime(Unit* unit)
            : Field(Type::typeId,unit)
        {}

        //! Get const value
        const type& value() const noexcept
        {
            return m_value;
        }

        //! Get mutable value
        type* mutableValue() noexcept
        {
            this->markSet();
            return &m_value;
        }

        //! Set field
        void set(const type& val)
        {
            this->markSet(true);
            m_value=val;
        }

        //! Copy field
        void copy(type &val) const
        {
            val=m_value;
        }

        //! Get default value
        type defaultValue() const
        {
            return fieldDefaultValue();
        }

        //! Clear field
        virtual void clear() override
        {
            fieldClear();
        }

        //! Reset field
        virtual void reset() override
        {
            fieldReset();
        }

        //! Get default value
        type fieldDefaultValue() const
        {
            return type{};
        }

        //! Clear field
        void fieldClear()
        {
            m_value.reset();
        }

        //! Reset field
        void fieldReset()
        {
            fieldClear();
            this->markSet(false);
        }

        //! Get field size
        virtual size_t maxPackedSize() const noexcept override
        {
            return fieldSize();
        }

        //! Get size of value
        static size_t valueSize(const type&) noexcept
        {
            return sizeof(uint64_t);
        }

        constexpr static size_t fieldSize() noexcept
        {
            return sizeof(uint64_t);
        }

        //! Prepare shared form of value storage for parsing from wire
        inline static void prepareSharedStorage(type& /*value*/,const AllocatorFactory*)
        {
        }

        //! Create value
        virtual type createValue(const AllocatorFactory* =AllocatorFactory::getDefault()) const
        {
            return type();
        }

        //! Format as JSON element
        template <typename Y>
        static bool formatJSON(
                    const Y& val,
                    json::Writer* writer
                )
        {
            return json::Fieldwriter<Y,Type>::write(val,writer);
        }

        //! Serialize as JSON element
        virtual bool toJSON(json::Writer* writer) const override
        {
            return formatJSON(m_value,writer);
        }

        virtual void pushJsonParseHandler(Unit* topUnit) override
        {
            JsonR<Type,std::false_type>::push(topUnit,this);
        }

        template <typename BufferT>
        static bool serialize(const type& val, BufferT& wired)
        {
            return VariableSer<uint64_t>::serialize(val.toNumber(),wired);
        }

        template <typename BufferT>
        static bool deserialize(type& val, BufferT& wired, const AllocatorFactory* =nullptr)
        {
            uint64_t num=0;
            if (!VariableSer<uint64_t>::deserialize(num,wired))
            {
                return false;
            }
            auto r=common::DateTime::fromNumber(num);
            if (r)
            {
                return false;
            }
            val=r.takeValue();
            return true;
        }

        template <typename BufferT>
        bool serialize(BufferT& wired) const
        {
            return serialize(this->m_value,wired);
        }

        template <typename BufferT>
        bool deserialize(BufferT& wired, const AllocatorFactory*)
        {
            this->markSet(deserialize(this->m_value,wired));
            return this->isSet();
        }

        void setV(const common::DateTime& val) override
        {
            set(val);
        }

        void getV(common::DateTime& val) const override
        {
            val=m_value;
        }

        bool less(const common::DateTime& val) const override
        {
            return m_value<val;
        }

        bool equals(const common::DateTime& val) const override
        {
            return m_value==val;
        }

    protected:

        //! Load field from wire
        virtual bool doLoad(WireData& wired, const AllocatorFactory* factory) override
        {
            return deserialize(wired,factory);
        }

        //! Store field to wire
        virtual bool doStore(WireData& wired) const override
        {
            return serialize(wired);
        }

    protected:

        type m_value;
};

template <>
struct FieldTmpl<TYPE_DATETIME> : public DateTime
{
    using DateTime::DateTime;
};

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITDATETIME_H
