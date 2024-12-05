/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/custom.h
  *
  *      Base class for custom datauint fields.
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITCUSTOMFIELD_H
#define HATNDATAUNITCUSTOMFIELD_H

#include <type_traits>

#include <hatn/common/datetime.h>

#include <hatn/dataunit/stream.h>
#include <hatn/dataunit/fields/fieldtraits.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

//---------------------------------------------------------------

template <typename TraitsT>
class CustomField : public Field
{
    public:

        using Type=typename TraitsT::TYPE;
        using type=typename TraitsT::type;
        using selfType=CustomField<TraitsT>;

        using isRepeatedType=std::false_type;
        using isEnum=typename Type::isEnum;
        using Enum=typename Type::Enum;
        constexpr static auto typeId=Type::typeId;
        constexpr static auto isArray=isRepeatedType{};

        using maxSize=typename TraitsT::maxSize;

        static size_t valueSize(const type& v) noexcept
        {
            return TraitsT::valueSize(v);
        }

        constexpr static size_t fieldSize() noexcept
        {
            return TraitsT::fieldSize();
        }

        template <typename BufferT>
        static bool serialize(const type& val, BufferT& wired)
        {
            return TraitsT::serialize(val,wired);
        }

        template <typename BufferT>
        static bool deserialize(type& val, BufferT& wired, AllocatorFactory* f=nullptr)
        {
            return TraitsT::deserialize(val,wired,f);
        }

        void fieldClear()
        {
            TraitsT::fieldClear(m_value);
        }

        //! Ctor with default value
        CustomField(Unit* unit,type defaultValue)
            : Field(Type::typeId,unit),
            m_value(std::move(defaultValue))
        {}

        //! Ctor with parent unit
        explicit CustomField(Unit* unit)
            : Field(Type::typeId,unit)
        {}

        //! Get const value
        const type& value() const noexcept
        {
            return m_value;
        }

        //! Get mutable value marking as set.
        type* mutableValue() noexcept
        {
            this->markSet();
            return &m_value;
        }

        //! Set field
        void set(type val)
        {
            this->markSet(true);
            m_value=std::move(val);
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

        //! Reset field
        void fieldReset()
        {
            fieldClear();
            this->markSet(false);
        }

        //! Prepare shared form of value storage for parsing from wire
        static void prepareSharedStorage(type& /*value*/,AllocatorFactory*)
        {
        }

        //! Create value
        virtual type createValue(AllocatorFactory* =AllocatorFactory::getDefault()) const
        {
            return type{};
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

        void copyValue(type &val) const noexcept
        {
            val=m_value;
        }

        virtual size_t size() const noexcept override
        {
            return fieldSize();
        }

        template <typename BufferT>
        bool serialize(BufferT& wired) const
        {
            return serialize(this->m_value,wired);
        }

        template <typename BufferT>
        bool deserialize(BufferT& wired, AllocatorFactory*)
        {
            this->markSet(deserialize(this->m_value,wired));
            return this->isSet();
        }

    protected:

        //! Load field from wire
        virtual bool doLoad(WireData& wired, AllocatorFactory* factory) override
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

//---------------------------------------------------------------

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITCUSTOMFIELD_H
