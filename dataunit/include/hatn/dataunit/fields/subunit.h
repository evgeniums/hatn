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

template <typename Type>
struct SubunitSetter
{
    template <typename UnitT>
    static void setV(UnitT* self, common::SharedPtr<Unit> val,
                     std::enable_if_t<
                         decltype(has_sharedFromThis<typename UnitT::managed>())::value
                         >* =nullptr
                     )
    {
        using managedType=typename UnitT::managed;
        Assert(strcmp(val->name(),managedType::unitName())==0,"Mismatched unit types");

        static managedType sample;
        managedType* casted=sample.castToManagedUnit(val.get());
        self->set(casted->sharedFromThis());
    }

    template <typename UnitT>
    static void setV(UnitT*, common::SharedPtr<Unit>,
                     std::enable_if_t<
                         !std::is_base_of<common::ManagedObject,typename UnitT::managed>::value
                         >* =nullptr
                     )
    {
        Assert(false,"Cannot set unmanaged unit");
    }
};


template <typename Type>
struct SubunitHolder
{
    using base=typename Type::type;
    using managed=typename Type::managed;
    using shared_managed=common::SharedPtr<managed>;
    using type=lib::variant<lib::monostate,base,shared_managed>;

    static base* visit(base& value)
    {
        return &value;
    }

    static const base* visit(const base& value)
    {
        return &value;
    }

    static base* visit(shared_managed& value)
    {
        return value.get();
    }

    static const base* visit(const shared_managed& value)
    {
        return value.get();
    }

    static base* visit(lib::monostate&)
    {
        return nullptr;
    }

    static const base* visit(const lib::monostate&)
    {
        return nullptr;
    }

    static auto createPlainValue(const AllocatorFactory* factory=nullptr)
    {
        return common::constructWithArgsOrDefault<base>(factory);
    }
};

//! Field template for embedded DataUnit type
template <typename Type>
class SubunitT : public Field, public UnitType
{
    public:

        using Field::isSet;

        using holder=SubunitHolder<Type>;
        using type=typename holder::type;
        using base=typename holder::base;
        using managed=typename holder::managed;
        using shared_managed=typename holder::shared_managed;

        using selfType=SubunitT<Type>;
        using baseFieldType=selfType;
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
        explicit SubunitT(Unit* parentUnit):
            Field(Type::typeId,parentUnit),
            m_parseToSharedArrays(false)
        {
        }

        void setShared(bool enable, bool autoCreate=true, const AllocatorFactory* factory=nullptr)
        {
            m_shared=enable;
            if (autoCreate)
            {
                markSet();
                createValue(factory);
            }
        }

        void resetShared()
        {
            m_shared.reset();
        }

        bool isShared() const
        {
            return m_shared.value_or(unit()->isSharedSubunits());
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

        common::ByteArrayShared skippedNotParsedContent() const
        {
            return m_skippedNotParsedContent;
        }

        void resetSkippedNotParsedContent(common::ByteArrayShared buf={}) noexcept
        {
            m_skippedNotParsedContent=std::move(buf);
        }

        template <typename BufferT>
        bool serialize(BufferT& wired) const
        {
            return UnitSer::serialize(&variantValue()->value(),wired);
        }

        //! Serialize DataUnit to wire
        template <typename BufferT>
        static bool serialize(const base* value, BufferT& wired)
        {
            return UnitSer::serialize(value,wired);
        }

        //! Serialize DataUnit to wire
        template <typename SubunitT, typename BufferT>
        static bool serialize(const SubunitT& subunit, BufferT& wired)
        {
            return UnitSer::serialize(&subunit.value(),wired);
        }

        //! Deserialize DataUnit from wire
        template <typename SubunitT, typename BufferT>
        static bool deserialize(SubunitT& subunit,BufferT& wired, const AllocatorFactory* factory)
        {
            return deserialize(subunit.mutableValue(),wired,factory);
        }

        //! Deserialize DataUnit from wire
        template <typename BufferT>
        static bool deserialize(base* value, BufferT& wired, const AllocatorFactory*, bool /*repeated*/=false)
        {
            return UnitSer::deserialize(value,wired);
        }

        template <typename BufferT>
        bool deserialize(BufferT& wired, const AllocatorFactory* factory)
        {
            if (factory==nullptr)
            {
                factory=this->unit()->factory();
            }
            auto deserializeToBuf=[this,&wired,factory]()
            {
                this->markSet(BytesSer<
                              typename common::ByteArrayManaged,
                              typename common::ByteArrayShared
                              >
                              ::
                              deserialize(
                                  wired,
                                  m_skippedNotParsedContent.get(),
                                  m_skippedNotParsedContent,
                                  factory,
                                  -1,
                                  true
                                  ));
                return this->isSet();
            };

            // deserialized to buf if it is set
            if (m_skippedNotParsedContent)
            {
                return deserializeToBuf();
            }

            // get/create mutable value
            auto* value=mutableValue();            
            if (value==nullptr)
            {
                // if failed to create value then create skip buffer and deserialize to it
                m_skippedNotParsedContent=factory->template createObject<common::ByteArrayManaged>(factory);
                return deserializeToBuf();
            }

            // normal parsing
            this->fieldClear();
            io::setParseToSharedArrays(*value,m_parseToSharedArrays,factory);
            this->markSet(deserialize(*this,wired,factory));
            return this->isSet();
        }

        inline static bool formatJSON(const selfType& subunit,json::Writer* writer)
        {
            return formatJSON(&subunit.value(),writer);
        }

        //! Format as JSON element
        inline static bool formatJSON(const base* value,json::Writer* writer)
        {
            return value->toJSON(writer);
        }

        //! Serialize as JSON element
        virtual bool toJSON(json::Writer* writer) const override
        {
            return formatJSON(variantValue(),writer);
        }

        virtual void pushJsonParseHandler(Unit *topUnit) override
        {
            json::pushHandler<selfType,json::FieldReader<Type,selfType>>(topUnit,this);
        }

        /**
         * @brief Get field size
         * @return Packed size of the unit
         */
        virtual size_t maxPackedSize() const noexcept override
        {
            return fieldSize();
        }
        //! Get size of value
        static size_t valueSize(const selfType& subunit) noexcept
        {
            return subunit.fieldSize();
        }

        bool isNull() const noexcept
        {
            return lib::variantIndex(m_value)==0;
        }

        bool isPlainValue() const noexcept
        {
            return lib::variantIndex(m_value)==1;
        }

        bool isSharedValue() const noexcept
        {
            return lib::variantIndex(m_value)==2;
        }

        size_t fieldSize() const noexcept
        {
            if (isNull())
            {
                // return unpacked space reserved for length field
                return sizeof(uint32_t)+1;
            }
            // return data size plus unpacked space reserved for length field
            return (variantValue()->maxPackedSize()+sizeof(uint32_t)+1);
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
        base* mutableValue()
        {
            if (lib::variantIndex(m_value)==0)
            {
                createValue();
            }
            this->markSet(true);
            return variantValue();
        }

        auto createSharedValue(const AllocatorFactory* factory=nullptr, bool repeatedSubunit=false) const
        {
            auto val=Type::createManagedObject(factory,unit(),repeatedSubunit);
            return val;
        }

        auto createPlainValue(const AllocatorFactory* factory=nullptr) const
        {
            return holder::createPlainValue(factory);
        }

        base* createValue(const AllocatorFactory* factory=nullptr, bool repeatedSubunit=false)
        {
            if (factory==nullptr)
            {
                factory=unit()->factory();
            }
            if (isShared())
            {
                auto sharedValue=createSharedValue(factory,repeatedSubunit);
                if (sharedValue)
                {
                    m_value=std::move(sharedValue);
                }
            }
            else
            {
                m_value=createPlainValue(factory);
            }
            if (isNull())
            {
                return nullptr;
            }
            auto v=variantValue();
            v->setSharedSubunits(isShared());
            return v;
        }

        //! Get const reference to value
        const base& value() const
        {
            Assert(lib::variantIndex(m_value)!=0,"Dataunit field is not set!");
            return *variantValue();
        }

        //! Get field
        inline base& get()
        {
            Assert(lib::variantIndex(m_value)!=0,"Dataunit field is not set!");
            return *variantValue();
        }

        //! Get const field
        inline const base& get() const
        {
            Assert(lib::variantIndex(m_value)!=0,"Dataunit field is not set!");
            return *variantValue();
        }

        //! Set field
        inline void set(base&& val)
        {
            m_shared=false;
            this->markSet(true);
            m_value=std::move(val);
        }

        //! Set field
        inline void set(shared_managed val)
        {
            m_shared=true;
            this->markSet(true);
            m_value=std::move(val);
        }

        virtual void setV(common::SharedPtr<Unit> val) override
        {
            SubunitSetter<Type>::setV(this,std::move(val));
        }

        virtual void getV(common::SharedPtr<Unit>& val) const override
        {
            auto self=this;
            hana::eval_if(
                std::is_same<type,Unit>{},
                [&](auto _)
                {
                    _(val)=_(self)->sharedValue();
                },
                []()
                {
                    throw std::runtime_error("Can not get custom subunit field");
                }
            );
        }

        /**  Check if unit's field is set. */
        template <typename T>
        auto isSet(T&& fieldName) const
        {
            return value().isSet(std::forward<T>(fieldName));
        }

        /** Get unit's field value. */
        template <typename T>
        auto operator [] (T&& fieldName) const -> decltype(auto)
        {
            return value()[std::forward<T>(fieldName)];
        }

        /** Get unit's field value. */
        template <typename T>
        auto operator [] (T&& fieldName) -> decltype(auto)
        {
            return (*mutableValue())[std::forward<T>(fieldName)];
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

        template <typename T, typename ...Args>
        void setFieldValue(T&& fieldName, Args&&... val)
        {
            field(std::forward<T>(fieldName)).set(std::forward<Args>(val)...);
        }

        /** Get field's value. */
        template <typename T>
        auto fieldValue(T&& fieldName) const noexcept -> decltype(auto)
        {
            return field(std::forward<T>(fieldName)).value();
        }

        /**
         * @brief Use shared version of byte arrays data when parsing wired data
         * @param enable Enabled on/off
         * @param factory Allocator factory to use for dynamic allocation
         *
         * When enabled then shared byte arrays will be auto allocated in managed shared buffers.
         *
         */
        virtual void setParseToSharedArrays(bool enable,const AllocatorFactory* factory=nullptr) override
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

        void fieldSetParseToSharedArrays(bool enable,const AllocatorFactory* =nullptr)
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
        template <typename T>
        inline static void prepareSharedStorage(T&& /*value*/,const AllocatorFactory*)
        {
        }

        virtual const Unit* subunit() const override {return &value();}
        virtual Unit* subunit() override {return mutableValue();}

        void fieldClear()
        {
            m_skippedNotParsedContent.reset();
            if (!this->mutableValue())
            {
                return;
            }
            io::clear(*(this->mutableValue()));
        }

        //! Reset field
        void fieldReset(bool /*onlyNonClean*/=false)
        {
            m_skippedNotParsedContent.reset();
            m_value=type{};
            m_shared.reset();
            markSet(false);
        }

        const type& nativeValue() const
        {
            return *variantValue();
        }

        type& nativeValue()
        {
            return *variantValue();
        }

        shared_managed sharedValue() const
        {
            if (!isSharedValue())
            {
                return shared_managed{};
            }
            return lib::variantGet<shared_managed>(m_value);
        }

        shared_managed sharedValue(bool autoCreate=false)
        {
            if (!isSharedValue())
            {
                if (autoCreate)
                {
                    setShared(true);
                    return lib::variantGet<shared_managed>(m_value);
                }
                return shared_managed{};
            }
            return lib::variantGet<shared_managed>(m_value);
        }

    protected:

        //! Load field from wire
        virtual bool doLoad(WireData& wired, const AllocatorFactory* factory) override
        {
            return this->deserialize(mutableValue(),wired,factory);
        }

        //! Store field to wire
        virtual bool doStore(WireData& wired) const override
        {
            return serialize(wired);
        }

        auto* variantValue() noexcept
        {
            return lib::variantVisit([](auto& val){return holder::visit(val);},m_value);
        }

        const auto* variantValue() const noexcept
        {
            return lib::variantVisit([](const auto& val){return holder::visit(val);},m_value);
        }

        type m_value;

    private:

        bool m_parseToSharedArrays;
        lib::optional<bool> m_shared;

        common::ByteArrayShared m_skippedNotParsedContent;
};

template <>
struct FieldTmpl<TYPE_DATAUNIT> : public SubunitT<TYPE_DATAUNIT>
{
    using SubunitT<TYPE_DATAUNIT>::SubunitT;
};

/**  Template class of embedded dataunit field */
template <typename Type>
struct SubunitFieldTmpl : public SubunitT<Type>
{
    using SubunitT<Type>::SubunitT;
};

template <typename FieldName,typename Type,int Id, bool Required=false>
struct SubunitField : public FieldConf<SubunitFieldTmpl<Type>,Id,FieldName,Type,Required>
{
    using FieldConf<SubunitFieldTmpl<Type>,Id,FieldName,Type,Required>::FieldConf;
};

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNSUBUNIT_H
