/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/subunit.h
  *
  *      DataUnit fields for embedded subunits
  *
  */

/****************************************************************************/

#ifndef HATNSUBUNIT_H
#define HATNSUBUNIT_H

#include <boost/hana.hpp>

#include <hatn/common/utils.h>

#include <hatn/dataunit/fields/fieldtraits.h>
#include <hatn/dataunit/visitors.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

struct UnitType{
    constexpr static const ValueType typeId=ValueType::Dataunit;
};

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
        using isEmbeddedUnitType=boost::hana::bool_<!Shared>;
        using isUnitType=std::true_type;

        using isBytesType=std::false_type;
        using isStringType=std::false_type;
        using isRepeatedType=std::false_type;
        using isPackedProtoBufCompatible=typename Type::isPackedProtoBufCompatible;

        /**
         * @brief Ctor
         * @param factory Allocator factory used both for the field itself and for the embedded vaule,
         *        thta's why it is propagated to the value constructor
         */
        explicit FieldTmplUnitEmbedded(Unit* parentUnit):
            Field(Type::typeId,parentUnit),
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

        constexpr static WireType fieldWireType() noexcept
        {
            return WireType::WithLength;
        }
        //! Get wire type of the field
        virtual WireType wireType() const noexcept override
        {
            return fieldWireType();
        }

        //! Serialize DataUnit to wire
        template <typename UnitT, typename BufferT>
        static bool serialize(const UnitT* value, BufferT& wired)
        {
            return UnitSer::serialize(value,wired);
        }

        //! Serialize DataUnit to wire
        template <typename UnitT, typename BufferT>
        static bool serialize(const common::SharedPtr<UnitT>& value, BufferT& wired)
        {
            return UnitSer::serialize(value.get(),wired);
        }

        //! Serialize DataUnit to wire
        template <typename UnitT, typename BufferT>
        static bool serialize(const UnitT& value, BufferT& wired)
        {
            return UnitSer::serialize(&value,wired);
        }

        template <typename BufferT>
        bool serialize(BufferT& wired) const
        {
            return UnitSer::serialize(&this->m_value.value(),wired);
        }

        //! Deserialize DataUnit from wire
        template <typename UnitT, typename BufferT>
        static bool deserialize(UnitT* value, BufferT& wired, AllocatorFactory*, bool /*repeated*/=false)
        {
            return UnitSer::deserialize(value,wired);
        }

        template <typename BufferT>
        bool deserialize(BufferT& wired, AllocatorFactory* factory)
        {
            this->fieldReset(true);
            if (factory==nullptr)
            {
                factory=this->unit()->factory();
            }

            auto* value=mutableValue();
            io::setParseToSharedArrays(*value,m_parseToSharedArrays,factory);
            this->markSet(this->deserialize(mutableValue(),wired,factory));
            return this->isSet();
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
        template <typename UnitT, typename BufferT>
        static bool deserialize(UnitT& value,BufferT& wired, AllocatorFactory* factory)
        {
            return deserialize(value.mutableValue(),wired,factory);
        }

        /**
         * @brief Get field size
         * @return Packed size of the unit
         */
        virtual size_t size() const noexcept override
        {
            return fieldSize();
        }

        //! Get size of value
        template <typename T>
        static size_t valueSize(const T& value) noexcept
        {
            if (value.isNull())
            {
                // return unpacked space reserved for length field
                return sizeof(uint32_t)+1;
            }
            // return data size plus unpacked space reserved for length field
            return (value.value().size()+sizeof(uint32_t)+1);
        }

        size_t fieldSize() const noexcept
        {
            return valueSize(this->m_value);
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

        /**
         * @brief Get pointer to mutable value
         * @return Pointer to value
         *
         * After calling this method the value will be regarded as set.
         */
        typename baseFieldType::base* mutableValue()
        {
            auto self=this;
            hana::eval_if(
                isEmbeddedUnitType{},
                [&](auto)
                {},
                [&](auto _)
                {
                    auto s=_(self);
                    if (s->m_value.isNull())
                    {
                        s->m_value=s->createValue();
                    }
                    if (s->m_value.isNull())
                    {
                        throw std::runtime_error("Failed to create value for dataunit field!");
                    }
                }
            );
            this->markSet(true);
            return this->m_value.mutableValue();
        }

        auto createValue(AllocatorFactory* factory=nullptr) const
        {
            auto self=this;
            return hana::eval_if(
                isEmbeddedUnitType{},
                [&](auto)
                {
                    return 0;
                },
                [&](auto _)
                {
                    auto f=_(factory);
                    auto s=_(self);
                    if (f==nullptr)
                    {
                        f=s->unit()->factory();
                    }
                    auto val=Type::createManagedObject(f,s->unit());
                    if (val.isNull())
                    {
                        HATN_ERROR(dataunit,"Cannot create managed object in shared dataunit field!");
                        Assert(!val.isNull(),"Shared dataunit field is not set!");
                    }
                    return val;
                }
            );
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
            this->markSet(true);
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
         * When enabled then shared byte arrays will be auto allocated in managed shared buffers.
         *
         */
        virtual void setParseToSharedArrays(bool enable,AllocatorFactory* factory=nullptr) override
        {
            fieldSetParseToSharedArrays(enable,factory);
        }

        /**
         * @brief Check if shared byte arrays must be used for parsing
         * @return Boolean flag
         */
        virtual bool isParseToSharedArrays() const noexcept override
        {
            return fieldIsParseToSharedArrays();
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

        void fieldSetParseToSharedArrays(bool enable,AllocatorFactory* =nullptr)
        {
            m_parseToSharedArrays=enable;
        }

        bool fieldIsParseToSharedArrays() const noexcept
        {
            return m_parseToSharedArrays;
        }

        //! Can chain blocks
        constexpr static bool fieldCanChainBlocks() noexcept
        {
            return CanChainBlocks;
        }

        //! Prepare shared form of value storage for parsing from wire
        inline static void prepareSharedStorage(type& /*value*/,AllocatorFactory*)
        {
        }

        virtual const Unit* subunit() const override {return &value();}
        virtual Unit* subunit() override {return mutableValue();}

        void fieldClear()
        {
            io::clear(*(this->mutableValue()));
        }

        //! Reset field
        void fieldReset(bool onlyNonClean=false)
        {
            auto self=this;
            boost::hana::eval_if(
                boost::hana::is_a<common::shared_pointer_tag,decltype(this->m_value)>,
                [&](auto _)
                {
                    _(self)->m_value.reset();
                    _(self)->markSet(false);
                },
                [&](auto _)
                {
                    io::reset(_(self)->m_value,_(onlyNonClean));
                }
            );
            this->markSet(false);
        }

    protected:

        //! Load field from wire
        virtual bool doLoad(WireData& wired, AllocatorFactory* factory) override
        {
            return this->deserialize(mutableValue(),wired,factory);
        }

        //! Store field to wire
        virtual bool doStore(WireData& wired) const override
        {
            return serialize(wired);
        }

        type m_value;

    private:

        bool m_parseToSharedArrays;
};

template <>
struct FieldTmpl<TYPE_DATAUNIT> : public FieldTmplUnitEmbedded<TYPE_DATAUNIT,true>
{
    using FieldTmplUnitEmbedded<TYPE_DATAUNIT,true>::FieldTmplUnitEmbedded;
};

/**  Template class of embedded dataunit field */
template <typename Type>
struct EmbeddedUnitFieldTmpl : public FieldTmplUnitEmbedded<Type,false>
{
    using FieldTmplUnitEmbedded<Type,false>::FieldTmplUnitEmbedded;
};

/**  Template class of embedded dataunit field */
template <typename Type>
struct SharedUnitFieldTmpl : public FieldTmplUnitEmbedded<Type,true>
{
    using FieldTmplUnitEmbedded<Type,true>::FieldTmplUnitEmbedded;
};

template <typename FieldName,typename Type,int Id, bool Required=false>
struct EmbeddedUnitField : public FieldConf<EmbeddedUnitFieldTmpl<Type>,Id,FieldName,Type,Required>
{
    using FieldConf<EmbeddedUnitFieldTmpl<Type>,Id,FieldName,Type,Required>::FieldConf;
};

template <typename FieldName,typename Type,int Id, bool Required=false>
struct SharedUnitField : public FieldConf<SharedUnitFieldTmpl<Type>,Id,FieldName,Type,Required>
{
    using FieldConf<SharedUnitFieldTmpl<Type>,Id,FieldName,Type,Required>::FieldConf;
};

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNSUBUNIT_H
