/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** \file dataunit/subunit.h
  *
  *      DataUnit fields for embedded subunits
  *
  */

/****************************************************************************/

#ifndef HATNSUBUNIT_H
#define HATNSUBUNIT_H

#include <hatn/dataunit/fields/fieldtraits.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

struct UnitType{};

//! Field template for embedded DataUnit type
template <typename Type, bool Shared=false>
class FieldTmplUnitEmbedded : public Field, public UnitType
{
    public:

        using Field::isSet;

        using type=typename FTraits<Type,Shared>::type;
        using base=typename FTraits<Type,Shared>::base;

        using selfType=FieldTmplUnitEmbedded<Type,Shared>;
        using baseFieldType=selfType;
        using isEmbeddedUnitType=std::true_type;
        using isUnitType=std::true_type;

        using isBytesType=std::false_type;
        using isStringType=std::false_type;
        using isRepeatedType=std::false_type;

        /**
         * @brief Ctor
         * @param factory Allocator factory used both for the field itself and for the embedded vaule,
         *        thta's why it is propagated to the value constructor
         */
        explicit FieldTmplUnitEmbedded(Unit* parentUnit):
            Field(parentUnit),
            m_value(
                    ::hatn::common::ConstructWithArgsOrDefault<
                                        type,
                                        AllocatorFactory*
                                    >
                                ::f(
                                    std::forward<AllocatorFactory*>(parentUnit->factory())
                                    )
                ),
            m_parseToSharedArrays(false)
        {
        }

        inline static WireType wireTypeDef() noexcept
        {
            return WireType::WithLength;
        }
        //! Get wire type of the field
        virtual WireType wireType() const noexcept override
        {
            return wireTypeDef();
        }

        //! Serialize DataUnit to wire
        inline static bool serialize(const Unit* value, WireData& wired)
        {
            return UnitSer::serialize(value,wired);
        }

        //! Deserialize DataUnit from wire
        inline static bool deserialize(Unit* value, WireData& wired, AllocatorFactory*)
        {
            return UnitSer::deserialize(value,wired);
        }

        //! Format as JSON element
        inline static bool formatJSON(const typename baseFieldType::type& value,json::Writer* writer)
        {
            return formatJSON(&value.value(),writer);
        }

        //! Format as JSON element
        inline static bool formatJSON(const Unit* value,json::Writer* writer)
        {
            return value->toJSON(writer);
        }

        //! Serialize as JSON element
        virtual bool toJSON(json::Writer* writer) const override
        {
            return formatJSON(&this->m_value.value(),writer);
        }

        virtual void pushJsonParseHandler(Unit *topUnit) override
        {
            json::pushHandler<selfType,json::FieldReader<Type,selfType>>(topUnit,this);
        }

        //! Serialize DataUnit to wire
        inline static bool serialize(const typename baseFieldType::type& value,WireData& wired)
        {
            return serialize(&value.value(),wired);
        }

        //! Deserialize DataUnit from wire
        template <typename T> inline static bool deserialize(T& value,WireData& wired, AllocatorFactory* factory)
        {
            return deserialize(value.mutableValue(),wired,factory);
        }

        /**
         * @brief Get field size
         * @return Packed size of the unit
         */
        virtual size_t size() const noexcept override
        {
            return valueSize(this->m_value);
        }

        //! Get size of value
        template <typename T> inline static size_t valueSize(const T& value) noexcept
        {
            if (value.isNull())
            {
                // return unpacked space reserved for length field
                return sizeof(uint32_t)+1;
            }
            // return data size plus unpacked space reserved for length field
            return (value.value().size()+sizeof(uint32_t)+1);
        }

        //! Clear field
        virtual void clear() override
        {
            this->m_value.reset();
            this->m_set=false;
        }

        /**
         * @brief Get pointer to mutable value
         * @return Pointer to value
         *
         * After calling this method the value will be regarded as set.
         */
        virtual typename baseFieldType::base* mutableValue()
        {
            this->m_set=true;
            return this->m_value.mutableValue();
        }

        //! Get const reference to value
        virtual const typename baseFieldType::base& value() const
        {
            Assert(!this->m_value.isNull(),"Shared dataunit field is not set!");
            return this->m_value.value();
        }

        //! Get field
        inline type& get() noexcept
        {
            return m_value;
        }

        //! Get const field
        inline const type& get() const noexcept
        {
            return m_value;
        }

        //! Set field
        inline void set(type val)
        {
            m_set=true;
            m_value=std::move(val);
        }

        /**  Check if unit's field is set. */
        template <typename T>
        auto isSet(T&& fieldName,
                   std::enable_if_t<baseFieldType::base::hasField(std::declval<std::decay_t<T>>()),void*> =nullptr
                ) const noexcept
        {
            return value().isSet(std::forward<T>(fieldName));
        }

        /** Get unit's field value. */
        template <typename T>
        auto operator [] (T&& fieldName) const -> decltype(auto)
        {
            return value()[std::forward<T>(fieldName)];
        }

        /** Get subunit's field. */
        template <typename T>
        auto field(T&& fieldName) const -> decltype(auto)
        {
            return value().field(std::forward<T>(fieldName));
        }

        /** Get subunit's field. */
        template <typename T>
        auto field(T&& fieldName) -> decltype(auto)
        {
            return mutableValue()->field(std::forward<T>(fieldName));
        }

        /**
         * @brief Use shared version of byte arrays data when parsing wired data
         * @param enable Enabled on/off
         * @param factory Allocator factory to use for dynamic allocation
         *
         * When enabled then shared byte arrays will be auto allocated in managed shared buffers
         */
        virtual void setParseToSharedArrays(bool enable,AllocatorFactory* =nullptr) override
        {
            m_parseToSharedArrays=enable;
        }

        /**
         * @brief Check if shared byte arrays must be used for parsing
         * @return Boolean flag
         */
        virtual bool isParseToSharedArrays() const noexcept override
        {
            return m_parseToSharedArrays;
        }

        //! Check if field is required
        virtual bool isRequired() const noexcept override {return false;}
        //! Get field ID
        virtual int getID() const noexcept override {return -1;}

        constexpr static const bool CanChainBlocks=true;
        virtual bool canChainBlocks() const noexcept override
        {
            return CanChainBlocks;
        }

        //! Prepare shared form of value storage for parsing from wire
        inline static void prepareSharedStorage(type& /*value*/,AllocatorFactory*)
        {
        }

        virtual const Unit* subunit() const override {return &value();}
        virtual Unit* subunit() override {return mutableValue();}

    protected:

        //! Load field from wire
        virtual bool doLoad(WireData& wired, AllocatorFactory* factory) override
        {
            clear();
            if (factory==nullptr)
            {
                factory=this->unit()->factory();
            }

            auto* value=mutableValue();
            value->setParseToSharedArrays(m_parseToSharedArrays,factory);
            bool ok=this->deserialize(mutableValue(),wired,factory);
            if (ok)
            {
                this->markSet();
            }
            else
            {
                this->m_set=false;
            }
            return ok;
        }

        //! Store field to wire
        virtual bool doStore(WireData& wired) const override
        {
            return serialize(&this->m_value.value(),wired);
        }

        type m_value;

    private:

        bool m_parseToSharedArrays;
};

//! Field template for embedded DataUnit type
template <typename Type> class FieldTmplUnit : public FieldTmplUnitEmbedded<Type,true>
{
    public:

        using baseFieldType=FieldTmplUnitEmbedded<Type,true>;
        using FieldTmplUnitEmbedded<Type,true>::FieldTmplUnitEmbedded;
        using isEmbeddedUnitType=std::false_type;

        /**
         * @brief Get pointer to mutable value
         * @return Pointer to value
         *
         * After calling this method the value will be regarded as set.
         */
        virtual typename baseFieldType::base* mutableValue() override
        {
            this->m_set=true;
            if (this->m_value.isNull())
            {
                this->m_value=this->createValue();
            }
            if (this->m_value.isNull())
            {
                throw std::runtime_error("Failed to create value for dataunit field!");
            }
            return this->m_value.mutableValue();
        }

        //! Create value
        typename baseFieldType::type createValue(AllocatorFactory* factory=nullptr) const
        {
            if (factory==nullptr)
            {
                factory=this->unit()->factory();
            }
            auto val=Type::createManagedObject(factory,this->unit());
            if (val.isNull())
            {
                HATN_ERROR(dataunit,"Cannot create managed object in shared dataunit field!");
                Assert(!val.isNull(),"Shared dataunit field is not set!");
            }
            return val;
        }

        //! Clear field
        virtual void clear() override
        {
            this->m_value.reset();
            this->m_set=false;
        }
};

template <>
class FieldTmpl<TYPE_DATAUNIT>
    : public FieldTmplUnit<TYPE_DATAUNIT>
{
    using FieldTmplUnit<TYPE_DATAUNIT>::FieldTmplUnit;
};

/**  Template class of embedded dataunit field */
template <typename Type>
    struct EmbeddedUnitFieldTmpl
        : public FieldTmplUnitEmbedded<Type>
{
    using FieldTmplUnitEmbedded<Type>::FieldTmplUnitEmbedded;
};

/**  Template class of embedded dataunit field */
template <typename Type>
    struct SharedUnitFieldTmpl
        : public FieldTmplUnit<Type>
{
    using FieldTmplUnit<Type>::FieldTmplUnit;
};

template <typename FieldName,typename Type,int Id>
    struct EmbeddedUnitField : public FieldConf<EmbeddedUnitFieldTmpl<Type>,Id,FieldName,Type,false>
{
    using FieldConf<EmbeddedUnitFieldTmpl<Type>,Id,FieldName,Type,false>::FieldConf;
};

template <typename FieldName,typename Type,int Id>
    struct SharedUnitField : public FieldConf<SharedUnitFieldTmpl<Type>,Id,FieldName,Type,false>
{
    using FieldConf<SharedUnitFieldTmpl<Type>,Id,FieldName,Type,false>::FieldConf;
};

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNSUBUNIT_H
