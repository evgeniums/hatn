/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file dataunit/repeated.h
  *
  *      DataUnit fields of repeated types
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITREPEATEDFIELDS_H
#define HATNDATAUNITREPEATEDFIELDS_H

#include <type_traits>
#include <array>

#include <boost/hana.hpp>

#include <hatn/validator/utils/safe_compare.hpp>

#include <hatn/common/containerutils.h>

#include <hatn/dataunit/fields/fieldtraits.h>
#include <hatn/dataunit/fields/subunit.h>
#include <hatn/dataunit/fields/scalar.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

template <typename Type>
struct RepeatedTraits
{
    using fieldType=FieldTmpl<Type>;
    using valueType=typename Type::type;
    using type=valueType;
    constexpr static const bool isSizeIterateNeeded=Type::isSizeIterateNeeded;
    constexpr static const ValueType typeId=Type::typeId;
    using isUnitType=typename Type::isUnitType;
    using isPackedProtoBufCompatible=typename Type::isPackedProtoBufCompatible;

    template <typename T,typename=void> struct Ctor
    {};
    template <typename T> struct Ctor<T,
                std::enable_if_t<T::HasDefV::value>
                >
    {
        inline static valueType f(Unit* =nullptr)
        {
            return T::value();
        }
    };
    template <typename T> struct Ctor<T,
                std::enable_if_t<!T::HasDefV::value>
                >
    {
        inline static valueType f(Unit* parentUnit)
        {
            return common::ConstructWithArgsOrDefault<valueType,Unit*>::f(
                std::forward<Unit*>(parentUnit)
                );
        }
    };

    template <typename DefaultV> inline static valueType createValue(Unit* parentUnit)
    {
        return Ctor<DefaultV>::f(parentUnit);
    }
};

template <typename Type>
struct RepeatedTraits<SubunitFieldTmpl<Type>>
{
    using fieldType=SubunitFieldTmpl<Type>;
    using valueType=fieldType;
    using type=valueType;
    constexpr static const bool isSizeIterateNeeded=true;
    constexpr static const ValueType typeId=Type::typeId;

    template <typename>
    static auto createValue(
            Unit* parentUnit
        )
    {
        fieldType subunit{parentUnit};
        subunit.createValue(parentUnit->factory(),true);
        return subunit;
    }

    static auto createSubunit(
        Unit* parentUnit,
        bool shared
        )
    {
        fieldType subunit{parentUnit};
        subunit.setShared(shared);
        subunit.createValue(parentUnit->factory(),true);
        return subunit;
    }
};

struct RepeatedGetterSetterNoBytes
{
    template <typename ArrayT>
    constexpr static void bufResize(ArrayT&,size_t,size_t)
    {
        Assert(false,"Invalid operation for field of this type");
    }
    template <typename ArrayT>
    constexpr static void bufReserve(ArrayT&,size_t,size_t)
    {
        Assert(false,"Invalid operation for field of this type");
    }
    template <typename ArrayT>
    constexpr static const char* bufCStr(const ArrayT&,size_t)
    {
        Assert(false,"Invalid operation for field of this type");
        return nullptr;
    }
    template <typename ArrayT>
    constexpr static const char* bufData(const ArrayT&,size_t)
    {
        Assert(false,"Invalid operation for field of this type");
        return nullptr;
    }
    template <typename ArrayT>
    constexpr static char* bufData(ArrayT&,size_t)
    {
        Assert(false,"Invalid operation for field of this type");
        return nullptr;
    }
    template <typename ArrayT>
    constexpr static size_t bufSize(const ArrayT&,size_t)
    {
        Assert(false,"Invalid operation for field of this type");
        return 0;
    }
    template <typename ArrayT>
    constexpr static size_t bufCapacity(const ArrayT&,size_t)
    {
        Assert(false,"Invalid operation for field of this type");
        return 0;
    }
    template <typename ArrayT>
    constexpr static bool bufEmpty(const ArrayT&,size_t)
    {
        Assert(false,"Invalid operation for field of this type");
        return true;
    }
    template <typename ArrayT>
    constexpr static void bufSetValue(ArrayT&,size_t,const char*, size_t)
    {
        Assert(false,"Invalid operation for field of this type");
    }
    template <typename ArrayT>
    constexpr static void bufCreateShared(ArrayT&,size_t,const AllocatorFactory*)
    {
        Assert(false,"Invalid operation for field of this type");
    }
    template <typename RepeatedT>
    constexpr static void bufAppendValue(RepeatedT*,const char*, size_t)
    {
        Assert(false,"Invalid operation for field of this type");
    }

    template <typename ArrayT>
    constexpr static void bufGet(const ArrayT&,size_t, common::DataBuf&)
    {
        Assert(false,"Invalid operation for field of this type");
    }

    template <typename ArrayT>
    static bool equals(const ArrayT&,size_t,const common::ConstDataBuf&)
    {
        Assert(false,"Invalid operation for field of this type");
        return false;
    }
};
struct RepeatedGetterSetterNoUnit
{
    template <typename ArrayT>
    constexpr static Unit* unit(ArrayT&,size_t)
    {Assert(false,"Invalid operation for field of this type");return nullptr;}

    template <typename ArrayT>
    constexpr static const Unit* unit(const ArrayT&,size_t)
    {Assert(false,"Invalid operation for field of this type");return nullptr;}
};

struct RepeatedGetterSetterNoScalar
{
    template <typename T1, typename T2>
    constexpr static void setVal(T1&,const T2&)
    {
        Assert(false,"Invalid operation for field of this type");
    }
    template <typename T1, typename T2>
    constexpr static void incVal(T1&,const T2&)
    {
        Assert(false,"Invalid operation for field of this type");
    }
    template <typename T1, typename T2>
    constexpr static void getVal(const T1&,T2&)
    {
        Assert(false,"Invalid operation for field of this type");
    }
    template <typename ArrayT,typename T>
    constexpr static void appendVal(ArrayT&,const T&)
    {
        Assert(false,"Invalid operation for field of this type");
    }
    template <typename ValueT,typename T>
    static bool equals(const ValueT&,const T&)
    {
        Assert(false,"Invalid operation for field of this type");
        return false;
    }
};

template <typename Type, typename=void>
struct RepeatedGetterSetter
{
};
template <typename Type>
struct RepeatedGetterSetter<Type,
                            std::enable_if_t<!Type::isBytesType::value && !Type::isUnitType::value>
                            > : public RepeatedGetterSetterNoBytes,
                                public RepeatedGetterSetterNoUnit
{
    using valueType=typename Type::type;

    template <typename T>
    static void setVal(valueType& arrayVal,const T& val)
    {
        hana::eval_if(
            std::is_convertible<T,valueType>{},
            [&](auto _)
            {
                _(arrayVal)=static_cast<valueType>(_(val));
            },
            [](auto)
            {
                Assert(false,"Invalid operation for field of this type");
            }
        );
    }

    template <typename T>
    static void incVal(valueType& arrayVal,const T& val)
    {
        hana::eval_if(
            std::is_convertible<T,valueType>{},
            [&](auto _)
            {
                _(arrayVal)=static_cast<valueType>(_(arrayVal)+_(val));
            },
            [](auto)
            {
                Assert(false,"Invalid operation for field of this type");
            }
            );
    }

    template <typename T>
    static void getVal(const valueType& arrayVal,T &val) noexcept
    {
        hana::eval_if(
            std::is_convertible<valueType,T>{},
            [&](auto _)
            {
                _(val)=static_cast<T>(_(arrayVal));
            },
            [](auto)
            {
                Assert(false,"Invalid operation for field of this type");
            }
        );
    }

    template <typename ArrayT,typename T>
    static void appendVal(ArrayT& array,const T& val) noexcept
    {
        hana::eval_if(
            std::is_convertible<T,valueType>{},
            [&](auto _)
            {
                _(array).emplace_back(static_cast<valueType>(_(val)));
            },
            [](auto)
            {
                Assert(false,"Invalid operation for field of this type");
            }
        );
    }

    using RepeatedGetterSetterNoBytes::equals;

    template <typename T>
    static bool equals(const valueType& arrayVal,const T& val) noexcept
    {
        return ScalarCompare<valueType,T>::equal(arrayVal,val);
    }
};

template <typename Type>
struct RepeatedGetterSetter<Type,
                            std::enable_if_t<Type::isBytesType::value>
                            > : public RepeatedGetterSetterNoScalar,
                                public RepeatedGetterSetterNoUnit
{
    using fieldType=FieldTmpl<Type>;

    template <typename ArrayT>
    constexpr static void bufResize(ArrayT& array,size_t idx,size_t size)
    {
        array[idx].buf()->resize(size);
    }
    template <typename ArrayT>
    constexpr static void bufReserve(ArrayT& array,size_t idx,size_t size)
    {
        array[idx].buf()->reserve(size);
    }
    template <typename ArrayT>
    constexpr static const char* bufCStr(const ArrayT& array,size_t idx)
    {
        return array[idx].buf()->c_str();
    }
    template <typename ArrayT>
    constexpr static const char* bufData(const ArrayT& array,size_t idx)
    {
        return array[idx].buf()->data();
    }
    template <typename ArrayT>
    constexpr static char* bufData(ArrayT& array,size_t idx)
    {
        return array[idx].buf()->data();
    }
    template <typename ArrayT>
    constexpr static size_t bufSize(const ArrayT& array,size_t idx)
    {
        return array[idx].buf()->size();
    }
    template <typename ArrayT>
    constexpr static void bufGet(const ArrayT& array,size_t idx, common::DataBuf& result)
    {
        const auto* buf=array[idx].buf();
        result.set(*buf);
    }
    template <typename ArrayT>
    constexpr static size_t bufCapacity(const ArrayT& array,size_t idx)
    {
        return array[idx].buf()->capacity();
    }
    template <typename ArrayT>
    constexpr static bool bufEmpty(const ArrayT& array,size_t idx)
    {
        return array[idx].buf()->isEmpty();
    }
    template <typename ArrayT>
    constexpr static void bufSetValue(ArrayT& array,size_t idx,const char* data, size_t length)
    {
        array[idx].buf()->load(data,length);
    }
    template <typename ArrayT>
    constexpr static void bufCreateShared(ArrayT& array,size_t idx,const AllocatorFactory* factory)
    {
        fieldType::prepareSharedStorage(array[idx],factory);
    }
    template <typename RepeatedT>
    constexpr static void bufAppendValue(RepeatedT* field,const char* data, size_t length)
    {
        field->appendValue(data,length);
    }

    using RepeatedGetterSetterNoScalar::equals;

    template <typename ArrayT>
    static bool equals(const ArrayT& array,size_t idx,const common::ConstDataBuf& val) noexcept
    {
        return array[idx].buf()->isEqual(val.data(),val.size());
    }
};

template <typename Type> struct RepeatedGetterSetter<Type,
                            std::enable_if_t<Type::isUnitType::value>
                            > : public RepeatedGetterSetterNoScalar,
                                public RepeatedGetterSetterNoBytes
{
    using valueType=typename Type::type;

    template <typename ArrayT>
    constexpr static Unit* unit(ArrayT& array,size_t idx)
    {
        return array[idx].mutableValue();
    }

    template <typename ArrayT>
    constexpr static const Unit* unit(const ArrayT& array,size_t idx)
    {
        return &array[idx].value();
    }

    template <typename ValueT,typename T>
    static bool equals(const ValueT&,const T&)
    {
        Assert(false,"Invalid operation for field of this type");
        return false;
    }

    template <typename ArrayT>
    static bool equals(const ArrayT&,size_t,const common::ConstDataBuf&)
    {
        Assert(false,"Invalid operation for field of this type");
        return false;
    }

    using RepeatedGetterSetterNoScalar::setVal;
    using RepeatedGetterSetterNoScalar::getVal;
    using RepeatedGetterSetterNoScalar::appendVal;

    template <typename SubunitT>
    static void setVal(SubunitT& subunit,common::SharedPtr<Unit> val)
    {
        SubunitSetter<Type>::setV(&subunit,std::move(val));
    }

    template <typename SubunitT>
    static void getVal(const SubunitT& subunit,common::SharedPtr<Unit>& val)
    {
        val=subunit.sharedValue();
    }

    template <typename RepeatedFieldT>
    static void appendVal(RepeatedFieldT& field,common::SharedPtr<Unit> val) noexcept
    {
        auto& subunit=field.appendSharedSubunit();
        SubunitSetter<Type>::setV(&subunit,std::move(val));
    }
};

struct RepeatedType{};

/**  Template class for repeated dataunit field */
template <typename Type, int Id, typename DefaultTraits>
struct RepeatedFieldTmpl : public Field, public RepeatedType
{
    using type=typename RepeatedTraits<Type>::valueType;
    using fieldType=typename RepeatedTraits<Type>::fieldType;
    //! @todo optimization: Maybe use vector on stack
    using vectorType=HATN_COMMON_NAMESPACE::pmr::vector<type>;
    using isRepeatedType=std::true_type;
    using selfType=RepeatedFieldTmpl<Type,Id,DefaultTraits>;

    constexpr static const ValueType typeId=Type::typeId;

    constexpr static const bool isSizeIterateNeeded=RepeatedTraits<Type>::isSizeIterateNeeded;
    constexpr static int fieldId()
    {
        return Id;
    }

    /**  Ctor */
    explicit RepeatedFieldTmpl(
        Unit* parentUnit
        ): Field(RepeatedTraits<Type>::typeId,parentUnit,true),
        vector(parentUnit->factory()->dataAllocator<type>()),
        m_parseToSharedArrays(false)
    {}

    constexpr static WireType fieldWireType() noexcept
    {
        return WireType::WithLength;
    }
    //! Get wire type of the field
    virtual WireType wireType() const noexcept override
    {
        return fieldWireType();
    }

    /**  Get value by index */
    inline type& value(size_t index)
    {
        return vector[index];
    }

    /**  Get const value by index */
    inline const type& value(size_t index) const
    {
        return vector[index];
    }

    /**  Get value by index */
    inline type& at(size_t index)
    {
        return vector[index];
    }

    /**  Get const value by index */
    inline const type& at(size_t index) const
    {
        return vector[index];
    }

    //! Overload operator []
    inline const type& operator[] (std::size_t index) const
    {
        return vector[index];
    }

    //! Overload operator []
    inline type& operator[] (std::size_t index)
    {
        return vector[index];
    }

    /**  Get value by index */
    inline type& field(size_t index)
    {
        return vector[index];
    }

    /**  Get const value by index */
    inline const type& field(size_t index) const
    {
        return vector[index];
    }

    /**  Get vector */
    inline const vectorType& value() const noexcept
    {
        return vector;
    }

    /**  Get vector */
    inline vectorType* mutableValue() noexcept
    {
        this->markSet();
        return &vector;
    }

    /**  Get value by index */
    inline type* mutableValue(size_t index)
    {
        this->markSet();
        return &vector[index];
    }

    /**  Set value by index */
    template <typename T>
    inline void setValue(size_t index, T&& val)
    {
        vector[index]=std::forward<T>(val);
    }

    /**  Set value by index */
    template <typename T>
    inline void set(size_t index, T&& val)
    {
        vector[index]=std::forward<T>(val);
    }

    template <typename T,typename=void> struct AddStringHelper
    {
    };
    template <typename T> struct AddStringHelper<T,std::enable_if_t<!T::isBytesType::value>>
    {
        inline static void f(selfType&,const char*, size_t) noexcept
        {}
    };
    template <typename T> struct AddStringHelper<T,std::enable_if_t<T::isBytesType::value>>
    {
        template <typename FieldT>
        static void f(FieldT& field,const char* data, size_t size)
        {
            auto& val=field.createAndAppendValue();
            val.buf()->load(data,size);
        }
    };

    inline size_t appendValue(const char* data, size_t size)
    {
        AddStringHelper<Type>::f(*this,data,size);
        return vector.size();
    }

    inline size_t appendValue(const char* data)
    {
        return appendValue(data,strlen(data));
    }

    inline size_t appendValue(const std::string& str)
    {
        return appendValue(str.data(),str.size());
    }

    inline size_t appendValue(const lib::string_view& str)
    {
        return appendValue(str.data(),str.size());
    }

    inline size_t appendValue(type value)
    {
        this->markSet();
        vector.emplace_back(std::move(value));
        return vector.size();
    }

    template <typename T>
    inline size_t appendValue(T value)
    {
        this->markSet();

        hana::eval_if(
            typename Type::isUnitType{},
            [&](auto _)
            {
                auto& subunit=_(vector).emplace_back(_(unit()));
                subunit.set(std::move(_(value)));
            },
            [&](auto _)
            {
                _(vector).emplace_back(std::move(_(value)));
            }
        );

        return vector.size();
    }

    template <typename T>
    inline size_t append(T&& value)
    {
        return appendValue(std::forward<T>(value));
    }

    template <typename ... Vals>
    inline size_t append(Vals&&... vals)
    {
        this->markSet();
        common::ContainerUtils::append(vector,std::forward<Vals>(vals)...);
        return vector.size();
    }

    /**  Create and add value */
    type& createAndAppendValue()
    {
        this->markSet();
        auto val=RepeatedTraits<Type>::template createValue<DefaultTraits>(this->unit());
        if (fieldIsParseToSharedArrays())
        {
            fieldType::prepareSharedStorage(val,unit()->factory());
        }
        vector.emplace_back(std::move(val));
        return vector.back();
    }

    /**  Create and add plain value */
    type& createAndAppendSubunit(bool shared)
    {
        this->markSet();
        auto val=RepeatedTraits<Type>::createSubunit(this->unit(),shared);
        if (fieldIsParseToSharedArrays())
        {
            fieldType::prepareSharedStorage(val,unit()->factory());
        }
        vector.emplace_back(std::move(val));
        return vector.back();
    }

    type& appendPlainSubunit()
    {
        return createAndAppendSubunit(false);
    }

    type& appendSharedSubunit()
    {
        return createAndAppendSubunit(true);
    }

    /**  Emplace value */
    template <typename ...Args>
    type& emplaceValue(Args&&... args)
    {
        this->markSet();
        vector.emplace_back(std::forward<Args>(args)...);
        return vector.back();
    }

    /**  Add number of values */
    void appendValues(size_t n, bool onlySizeIterate=false)
    {
        auto self=this;
        hana::eval_if(
            hana::bool_c<isSizeIterateNeeded>,
            [&](auto _)
            {
                _(self)->vector.reserve(_(self)->vector.size()+n);
                for (size_t i=0;i<n;i++)
                {
                    _(self)->createAndAppendValue();
                }
            },
            [&](auto _)
            {
                if (DefaultTraits::HasDefV::value && !onlySizeIterate)
                {
                    _(self)->vector.reserve(_(self)->vector.size()+n);
                    for (size_t i=0;i<n;i++)
                    {
                        _(self)->createAndAppendValue();
                    }
                }
                else
                {
                    _(self)->vector.resize(_(self)->vector.size()+n);
                    _(self)->markSet();
                }
            }
        );
    }

    /**  Get number of repeated fields */
    inline size_t count() const noexcept
    {
        return vector.size();
    }

    /**  Get number of repeated fields */
    inline size_t size() const noexcept
    {
        return vector.size();
    }

    /**  Check if array is empty */
    inline bool empty() const noexcept
    {
        return vector.empty();
    }

    /**  Get number of repeated fields */
    inline size_t dataSize() const noexcept
    {
        return vector.size();
    }

    /**  Get vector capacity */
    inline size_t capacity() const noexcept
    {
        return vector.capacity();
    }

    /**  Reserve space in memory for repeated fields */
    inline void reserve(size_t size)
    {
        vector.reserve(size);
    }

    /**  Resize array */
    inline void resize(size_t size)
    {
        if (size==vector.size())
        {
            return;
        }

        if (size==0)
        {
            vector.clear();
        }
        else if (size<vector.size())
        {
            auto n=vector.size()-size;
            vector.erase(vector.end() - n, vector.end());
        }
        else
        {
            auto n=size-vector.size();
            appendValues(n);
        }

        this->markSet();
    }

    /**  Clear array */
    inline void clearArray() noexcept
    {
        vector.clear();
    }

    /** Get expected field size */
    virtual size_t maxPackedSize() const noexcept override
    {
        return fieldSize();
    }

    size_t fieldSize() const noexcept
    {
        if (isSizeIterateNeeded)
        {
            size_t result=0;
            for (auto&& it:vector)
            {
                result+=fieldType::valueSize(it);
            }
            return result;
        }
        return count()*sizeof(type);
    }

    /**  Clear field */
    void fieldClear()
    {
        clearArray();
    }

    //! Reset field
    void fieldReset()
    {
        fieldClear();
        this->markSet(false);
    }

    /**  Clear array */
    virtual void clear() override
    {
        fieldClear();
    }

    /**  Reset field */
    virtual void reset() override
    {
        fieldReset();
    }

    template <typename BufferT>
    bool deserialize(BufferT& wired, const AllocatorFactory* factory)
    {
        fieldClear();
        if (factory==nullptr)
        {
            factory=this->unit()->factory();
        }

        /* get count of elements in array */
        uint32_t count=0;
        auto* buf=wired.mainContainer();
        size_t availableBufSize=buf->size()-wired.currentOffset();
        auto consumed=Stream<uint32_t>::unpackVarInt(buf->data()+wired.currentOffset(),availableBufSize,count);
        if (consumed<0)
        {
            return false;
        }
        wired.incCurrentOffset(consumed);
        if (wired.currentOffset()>buf->size())
        {
            rawError(RawErrorCode::END_OF_STREAM,"current offset overflow after repeated count at offset {} for size {}",wired.currentOffset(),buf->size());
            return false;
        }

        // check max number of elements - it can not exceed the size of unprocessed data in the buffer, in general it depends on the element type
        auto unprocessed=buf->size()-wired.currentOffset();
        if (count>unprocessed)
        {
            rawError(RawErrorCode::END_OF_STREAM,"repeated count {} exceeds available buffer size {}",count,unprocessed);
            return false;
        }

        /* add required number of elements */
        appendValues(count,true);

        /* fill each field */
        for (uint32_t i=0;i<count;i++)
        {
            auto& field=value(i);
            if (!fieldType::deserialize(field,wired,factory))
            {
                return false;
            }
        }

        /* ok */
        this->markSet();
        return this->isSet();
    }

    /**  Load fields from wire */
    virtual bool doLoad(WireData& wired, const AllocatorFactory* factory) override
    {
        return deserialize(wired,factory);
    }

    constexpr static const bool CanChainBlocks=fieldType::CanChainBlocks;
    virtual bool canChainBlocks() const noexcept override
    {
        return CanChainBlocks;
    }
    constexpr static bool fieldCanChainBlocks() noexcept
    {
        return CanChainBlocks;
    }

    /**  Serialize field to wire */
    template <typename BufferT>
    bool serialize(BufferT& wired) const
    {
        /* append array count */
        if (CanChainBlocks && !wired.isSingleBuffer())
        {
            wired.appendUint32(static_cast<uint32_t>(count()));
        }
        else
        {
            wired.incSize(Stream<uint32_t>::packVarInt(wired.mainContainer(),static_cast<int>(count())));
        }

        /* append each field */
        for (size_t i=0;i<count();i++)
        {
            const auto& field=value(i);
            if (!fieldType::serialize(field,wired))
            {
                return false;
            }
        }

        /* ok */
        return true;
    }

    /**  Store field to wire */
    virtual bool doStore(WireData& wired) const override
    {
        return serialize(wired);
    }

    /**
    * @brief Use shared version of byte arrays data when parsing wired data
    * @param enable Enabled on/off
    *
    * When enabled then shared byte arrays will be auto allocated in managed shared buffers
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

    void fieldSetParseToSharedArrays(bool enable,const AllocatorFactory* =nullptr)
    {
        m_parseToSharedArrays=enable;
    }

    bool fieldIsParseToSharedArrays() const noexcept
    {
        return m_parseToSharedArrays;
    }

    /** Format as JSON element
    */
    inline static bool formatJSON(const RepeatedFieldTmpl& value,json::Writer* writer)
    {
        if (writer->StartArray())
        {
            for (size_t i=0;i<value.count();i++)
            {
                const auto& field=value.value(i);
                if (!fieldType::formatJSON(field,writer))
                {
                    return false;
                }
            }
            return writer->EndArray();
        }
        return false;
    }

    /** Serialize as JSON element */
    virtual bool toJSON(json::Writer* writer) const override
    {
        return formatJSON(*this,writer);
    }

    virtual void pushJsonParseHandler(Unit *topUnit) override
    {
        json::pushHandler<selfType,json::FieldReader<Type,selfType>>(topUnit,this);
    }

    vectorType vector;

    virtual size_t arraySize() const override {return count();}
    virtual size_t arrayCapacity() const override {return vector.capacity();}
    virtual bool arrayEmpty() const override {return vector.empty();}
    virtual void arrayClear() override {vector.clear();}

    virtual void arrayResize(size_t size) override
    {
        if (size>vector.size())
        {
            appendValues(size-vector.size());
        }
        else
        {
            resize(size);
        }
    }
    virtual void arrayReserve(size_t size) override {reserve(size);}

    virtual void arrayErase(size_t idx) override {vector.erase(vector.begin() + idx);}

    virtual void arraySet(size_t idx,bool val) override {RepeatedGetterSetter<Type>::setVal(vector[idx],val);}
    virtual void arraySet(size_t idx,uint8_t val) override {RepeatedGetterSetter<Type>::setVal(vector[idx],val);}
    virtual void arraySet(size_t idx,uint16_t val) override {RepeatedGetterSetter<Type>::setVal(vector[idx],val);}
    virtual void arraySet(size_t idx,uint32_t val) override {RepeatedGetterSetter<Type>::setVal(vector[idx],val);}
    virtual void arraySet(size_t idx,uint64_t val) override {RepeatedGetterSetter<Type>::setVal(vector[idx],val);}
    virtual void arraySet(size_t idx,int8_t val) override {RepeatedGetterSetter<Type>::setVal(vector[idx],val);}
    virtual void arraySet(size_t idx,int16_t val) override {RepeatedGetterSetter<Type>::setVal(vector[idx],val);}
    virtual void arraySet(size_t idx,int32_t val) override {RepeatedGetterSetter<Type>::setVal(vector[idx],val);}
    virtual void arraySet(size_t idx,int64_t val) override {RepeatedGetterSetter<Type>::setVal(vector[idx],val);}
    virtual void arraySet(size_t idx,float val) override {RepeatedGetterSetter<Type>::setVal(vector[idx],val);}
    virtual void arraySet(size_t idx,double val) override {RepeatedGetterSetter<Type>::setVal(vector[idx],val);}
    virtual void arraySet(size_t idx,const common::DateTime& val) override {RepeatedGetterSetter<Type>::setVal(vector[idx],val);}
    virtual void arraySet(size_t idx,const common::Date& val) override {RepeatedGetterSetter<Type>::setVal(vector[idx],val);}
    virtual void arraySet(size_t idx,const common::Time& val) override {RepeatedGetterSetter<Type>::setVal(vector[idx],val);}
    virtual void arraySet(size_t idx,const common::DateRange& val) override {RepeatedGetterSetter<Type>::setVal(vector[idx],val);}
    virtual void arraySet(size_t idx,const common::ConstDataBuf& val) override {RepeatedGetterSetter<Type>::bufSetValue(vector,idx,val.data(),val.size());}
    virtual void arraySet(size_t idx,const ObjectId& val) override {RepeatedGetterSetter<Type>::setVal(vector[idx],val);}

    virtual void arraySet(size_t idx,common::SharedPtr<Unit> val) override {
        RepeatedGetterSetter<Type>::setVal(vector[idx],val);
    }

    virtual void arrayGet(size_t idx,bool& val) const override {RepeatedGetterSetter<Type>::getVal(vector[idx],val);}
    virtual void arrayGet(size_t idx,uint8_t& val) const override {RepeatedGetterSetter<Type>::getVal(vector[idx],val);}
    virtual void arrayGet(size_t idx,uint16_t& val) const override {RepeatedGetterSetter<Type>::getVal(vector[idx],val);}
    virtual void arrayGet(size_t idx,uint32_t& val) const override {RepeatedGetterSetter<Type>::getVal(vector[idx],val);}
    virtual void arrayGet(size_t idx,uint64_t& val) const override {RepeatedGetterSetter<Type>::getVal(vector[idx],val);}
    virtual void arrayGet(size_t idx,int8_t& val) const override {RepeatedGetterSetter<Type>::getVal(vector[idx],val);}
    virtual void arrayGet(size_t idx,int16_t& val) const override {RepeatedGetterSetter<Type>::getVal(vector[idx],val);}
    virtual void arrayGet(size_t idx,int32_t& val) const override {RepeatedGetterSetter<Type>::getVal(vector[idx],val);}
    virtual void arrayGet(size_t idx,int64_t& val) const override {RepeatedGetterSetter<Type>::getVal(vector[idx],val);}
    virtual void arrayGet(size_t idx,float& val) const override {RepeatedGetterSetter<Type>::getVal(vector[idx],val);}
    virtual void arrayGet(size_t idx,double& val) const override {RepeatedGetterSetter<Type>::getVal(vector[idx],val);}    
    virtual void arrayGet(size_t idx,common::DateTime& val) const override {RepeatedGetterSetter<Type>::getVal(vector[idx],val);}
    virtual void arrayGet(size_t idx,common::Date& val) const override {RepeatedGetterSetter<Type>::getVal(vector[idx],val);}
    virtual void arrayGet(size_t idx,common::Time& val) const override {RepeatedGetterSetter<Type>::getVal(vector[idx],val);}
    virtual void arrayGet(size_t idx,common::DateRange& val) const override {RepeatedGetterSetter<Type>::getVal(vector[idx],val);}
    virtual void arrayGet(size_t idx,common::DataBuf& val) const override {RepeatedGetterSetter<Type>::bufGet(vector,idx,val);}
    virtual void arrayGet(size_t idx,ObjectId& val) const override {RepeatedGetterSetter<Type>::getVal(vector[idx],val);}
    virtual void arrayGet(size_t idx,common::SharedPtr<Unit>& val) const override {RepeatedGetterSetter<Type>::getVal(vector[idx],val);}

    virtual void arrayAppend(bool val) override {this->markSet();RepeatedGetterSetter<Type>::appendVal(vector,val);}
    virtual void arrayAppend(uint8_t val) override {this->markSet();RepeatedGetterSetter<Type>::appendVal(vector,val);}
    virtual void arrayAppend(uint16_t val) override {this->markSet();RepeatedGetterSetter<Type>::appendVal(vector,val);}
    virtual void arrayAppend(uint32_t val) override {this->markSet();RepeatedGetterSetter<Type>::appendVal(vector,val);}
    virtual void arrayAppend(uint64_t val) override {this->markSet();RepeatedGetterSetter<Type>::appendVal(vector,val);}
    virtual void arrayAppend(int8_t val) override {this->markSet();RepeatedGetterSetter<Type>::appendVal(vector,val);}
    virtual void arrayAppend(int16_t val) override {this->markSet();RepeatedGetterSetter<Type>::appendVal(vector,val);}
    virtual void arrayAppend(int32_t val) override {this->markSet();RepeatedGetterSetter<Type>::appendVal(vector,val);}
    virtual void arrayAppend(int64_t val) override {this->markSet();RepeatedGetterSetter<Type>::appendVal(vector,val);}
    virtual void arrayAppend(float val) override {this->markSet();RepeatedGetterSetter<Type>::appendVal(vector,val);}
    virtual void arrayAppend(double val) override {this->markSet();RepeatedGetterSetter<Type>::appendVal(vector,val);}
    virtual void arrayAppend(const common::DateTime& val) override {this->markSet();RepeatedGetterSetter<Type>::appendVal(vector,val);}
    virtual void arrayAppend(const common::Date& val) override {this->markSet();RepeatedGetterSetter<Type>::appendVal(vector,val);}
    virtual void arrayAppend(const common::Time& val) override {this->markSet();RepeatedGetterSetter<Type>::appendVal(vector,val);}
    virtual void arrayAppend(const common::DateRange& val) override {this->markSet();RepeatedGetterSetter<Type>::appendVal(vector,val);}
    virtual void arrayAppend(const common::ConstDataBuf& val) override {this->markSet();RepeatedGetterSetter<Type>::bufAppendValue(this,val.data(),val.size());}
    virtual void arrayAppend(const ObjectId& val) override {this->markSet();RepeatedGetterSetter<Type>::appendVal(vector,val);}

    virtual void arrayAppend(common::SharedPtr<Unit> val) override
    {
        // if constexpr (Type::isUnitType::value)
        // {
        //     markSet();
        //     RepeatedGetterSetter<type>::appendVal(*this,std::move(val));
        // }
#if 1
        auto self=this;
        auto t=hana::type_c<Type>;
        hana::eval_if(
          typename Type::isUnitType{},
          [&](auto _)
          {
            using type=typename std::decay_t<decltype(_(t))>::type;
            _(self)->markSet();
            RepeatedGetterSetter<type>::appendVal(*_(self),std::move(_(val)));
          },
          [&](auto)
          {
            Assert(false,"Invalid operation for field of this type");
          }
        );
#endif
    }

    virtual void arrayInc(size_t idx,uint8_t val) override {RepeatedGetterSetter<Type>::incVal(vector[idx],val);}
    virtual void arrayInc(size_t idx,uint16_t val) override {RepeatedGetterSetter<Type>::incVal(vector[idx],val);}
    virtual void arrayInc(size_t idx,uint32_t val) override {RepeatedGetterSetter<Type>::incVal(vector[idx],val);}
    virtual void arrayInc(size_t idx,uint64_t val) override {RepeatedGetterSetter<Type>::incVal(vector[idx],val);}
    virtual void arrayInc(size_t idx,int8_t val) override {RepeatedGetterSetter<Type>::incVal(vector[idx],val);}
    virtual void arrayInc(size_t idx,int16_t val) override {RepeatedGetterSetter<Type>::incVal(vector[idx],val);}
    virtual void arrayInc(size_t idx,int32_t val) override {RepeatedGetterSetter<Type>::incVal(vector[idx],val);}
    virtual void arrayInc(size_t idx,int64_t val) override {RepeatedGetterSetter<Type>::incVal(vector[idx],val);}
    virtual void arrayInc(size_t idx,float val) override {RepeatedGetterSetter<Type>::incVal(vector[idx],val);}
    virtual void arrayInc(size_t idx,double val) override {RepeatedGetterSetter<Type>::incVal(vector[idx],val);}

    virtual bool arrayEquals(size_t idx,bool val) const override {return RepeatedGetterSetter<Type>::equals(vector[idx],val);}
    virtual bool arrayEquals(size_t idx,uint8_t val) const override {return RepeatedGetterSetter<Type>::equals(vector[idx],val);}
    virtual bool arrayEquals(size_t idx,uint16_t val) const override {return RepeatedGetterSetter<Type>::equals(vector[idx],val);}
    virtual bool arrayEquals(size_t idx,uint32_t val) const override {return RepeatedGetterSetter<Type>::equals(vector[idx],val);}
    virtual bool arrayEquals(size_t idx,uint64_t val) const override {return RepeatedGetterSetter<Type>::equals(vector[idx],val);}
    virtual bool arrayEquals(size_t idx,int8_t val) const override {return RepeatedGetterSetter<Type>::equals(vector[idx],val);}
    virtual bool arrayEquals(size_t idx,int16_t val) const override {return RepeatedGetterSetter<Type>::equals(vector[idx],val);}
    virtual bool arrayEquals(size_t idx,int32_t val) const override {return RepeatedGetterSetter<Type>::equals(vector[idx],val);}
    virtual bool arrayEquals(size_t idx,int64_t val) const override {return RepeatedGetterSetter<Type>::equals(vector[idx],val);}
    virtual bool arrayEquals(size_t idx,float val) const override {return RepeatedGetterSetter<Type>::equals(vector[idx],val);}
    virtual bool arrayEquals(size_t idx,double val) const override {return RepeatedGetterSetter<Type>::equals(vector[idx],val);}
    virtual bool arrayEquals(size_t idx,const common::DateTime& val) const override {return RepeatedGetterSetter<Type>::equals(vector[idx],val);}
    virtual bool arrayEquals(size_t idx,const common::Date& val) const override {return RepeatedGetterSetter<Type>::equals(vector[idx],val);}
    virtual bool arrayEquals(size_t idx,const common::Time& val) const override {return RepeatedGetterSetter<Type>::equals(vector[idx],val);}
    virtual bool arrayEquals(size_t idx,const common::DateRange& val) const override {return RepeatedGetterSetter<Type>::equals(vector[idx],val);}
    virtual bool arrayEquals(size_t idx,const common::ConstDataBuf& val) const override {return RepeatedGetterSetter<Type>::equals(vector,idx,val);}
    virtual bool arrayEquals(size_t idx,const ObjectId& val) const override {return RepeatedGetterSetter<Type>::equals(vector[idx],val);}

    virtual void arrayBufResize(size_t idx,size_t size) override {RepeatedGetterSetter<Type>::bufResize(vector,idx,size);}
    virtual void arrayBufReserve(size_t idx,size_t size) override {RepeatedGetterSetter<Type>::bufReserve(vector,idx,size);}
    virtual const char* arrayBufCStr(size_t idx) const override {return RepeatedGetterSetter<Type>::bufCStr(vector,idx);}
    virtual const char* arrayBufData(size_t idx) const override {return RepeatedGetterSetter<Type>::bufData(vector,idx);}
    virtual char* arrayBufData(size_t idx) override {return RepeatedGetterSetter<Type>::bufData(vector,idx);}
    virtual size_t arrayBufSize(size_t idx) const override {return RepeatedGetterSetter<Type>::bufSize(vector,idx);}
    virtual size_t arrayBufCapacity(size_t idx) const override {return RepeatedGetterSetter<Type>::bufCapacity(vector,idx);}
    virtual bool arrayBufEmpty(size_t idx) const override {return RepeatedGetterSetter<Type>::bufEmpty(vector,idx);}
    virtual void arrayBufCreateShared(size_t idx) override {RepeatedGetterSetter<Type>::bufCreateShared(vector,idx,this->unit()->factory());}
    virtual void arrayBufSetValue(size_t idx,const char* data,size_t length) override {RepeatedGetterSetter<Type>::bufSetValue(vector,idx,data,length);}
    virtual void arrayBufAppendValue(const char* data,size_t length) override {RepeatedGetterSetter<Type>::bufAppendValue(this,data,length);}

    virtual Unit* arraySubunit(size_t idx) override {return RepeatedGetterSetter<Type>::unit(vector,idx);}
    virtual const Unit* arraySubunit(size_t idx) const override {return RepeatedGetterSetter<Type>::unit(vector,idx);}

protected:

    bool m_parseToSharedArrays;
};

/**  Template class for repeated dataunit field compatible with packed repeated fields of Google Protocol Buffers*/
template <typename Type, int Id, typename DefaultTraits>
struct RepeatedFieldProtoBufPackedTmpl : public RepeatedFieldTmpl<Type,Id,DefaultTraits>
{
    using base=RepeatedFieldTmpl<Type,Id,DefaultTraits>;
    using type=typename base::type;
    using fieldType=typename base::fieldType;
    static_assert(Type::isPackedProtoBufCompatible::value,"You can't use this type in fields compatible with repeated packed type of Google Protol Buffers");

    using RepeatedFieldTmpl<Type,Id,DefaultTraits>::RepeatedFieldTmpl;

    template <typename BufferT>
    bool deserialize(BufferT& wired, const AllocatorFactory* factory)
    {
        this->fieldClear();

        if (factory==nullptr)
        {
            factory=this->unit()->factory();
        }

        /* get size of array */
        uint32_t length=0;
        auto* buf=wired.mainContainer();
        size_t availableBufSize=buf->size()-wired.currentOffset();
        auto consumed=Stream<uint32_t>::unpackVarInt(buf->data()+wired.currentOffset(),availableBufSize,length);
        if (consumed<0)
        {
            return false;
        }
        wired.incCurrentOffset(consumed);

        /* check if there is enough size in array left */
        availableBufSize=buf->size()-wired.currentOffset();
        if (availableBufSize<static_cast<size_t>(length))
        {
            return false;
        }

        /* fill each field */
        size_t lastOffset=wired.currentOffset()+length;
        while(wired.currentOffset()<lastOffset)
        {
            auto& field=this->createAndAppendValue();
            if (!fieldType::deserialize(field,wired,factory))
            {
                return false;
            }
        }

        /* ok */
        this->markSet();
        return this->isSet();
    }

    /**  Load fields from wire */
    virtual bool doLoad(WireData& wired, const AllocatorFactory* factory) override
    {
        return deserialize(wired,factory);
    }

    template <typename BufferT>
    bool serialize(BufferT& wired) const
    {
        auto* buf=wired.mainContainer();

        /* prepare buffer for length of the packed field */
        size_t headerCursor=buf->size();
        size_t prevSize=wired.size();
        /* reserve 5 bytes for length */
        wired.incSize(5);
        buf->resize(buf->size()+5);

        /* append each field */
        for (size_t i=0;i<this->count();i++)
        {
            const auto& field=this->vector[i];
            if (!fieldType::serialize(field,wired))
            {
                return false;
            }
        }
        size_t length=wired.size()-prevSize-5;

        /* pack length */
        std::array<char,5> header={0,0,0,0,0};
        StreamBase::packVarInt32(header.data(),static_cast<uint32_t>(length));
        size_t headerSize=0;
        while (headerSize<5)
        {
            if (header[headerSize]==0)
            {
                break;
            }
            ++headerSize;
        }

        /* write length to container */
        for (size_t i=0;i<headerSize;i++)
        {
            *(buf->data()+headerCursor+i)=header[i];
        }

        /* memmove data in container overwriting reserved space */
        int paddingSize=5-static_cast<int>(headerSize);
        if (paddingSize>0)
        {
            char* offset1=buf->data()+headerCursor+headerSize;
            char* offset2=buf->data()+headerCursor+5;
            for (uint32_t i=0;i<length;i++)
            {
                *(offset1+i)=*(offset2+i);
            }
            wired.incSize(-paddingSize);
            buf->resize(buf->size()-paddingSize);
        }

        /* ok */
        return true;
    }

    /**  Store field to wire */
    virtual bool doStore(WireData& wired) const override
    {
        return serialize(wired);
    }

    /** Get expected field size */
    size_t fieldSize() const noexcept
    {
        return this->count()*sizeof(type)+5;
    }

    /** Get expected field size */
    virtual size_t maxPackedSize() const noexcept override
    {
        return fieldSize();
    }
};

/**  Template class for repeated dataunit field compatible with unpacked repeated fields of Google Protocol Buffers for ordinary types*/
template <typename Type, int Id, typename DefaultTraits>
struct RepeatedFieldProtoBufUnpackedTmpl : public RepeatedFieldTmpl<Type,Id,DefaultTraits>
{
    using base=RepeatedFieldTmpl<Type,Id,DefaultTraits>;
    using type=typename RepeatedTraits<Type>::valueType;
    using fieldType=typename RepeatedTraits<Type>::fieldType;

    using RepeatedFieldTmpl<Type,Id,DefaultTraits>::RepeatedFieldTmpl;

    constexpr static WireType fieldWireType() noexcept
    {
        return fieldType::fieldWireType();
    }
    //! Get wire type of the field
    virtual WireType wireType() const noexcept override
    {
        return fieldWireType();
    }

    /** Get expected field size */
    virtual size_t maxPackedSize() const noexcept override
    {
        return fieldSize();
    }

    size_t fieldSize() const noexcept
    {
        if (base::isSizeIterateNeeded)
        {
            size_t result=0;
            for (auto&& it:this->vector)
            {
                result+=fieldType::valueSize(it)+4;
            }
            return result;
        }
        return this->count()*(sizeof(type)+4);
    }

    /**  Load fields from wire */
    template <typename BufferT>
    bool deserialize(BufferT& wired, const AllocatorFactory* factory)
    {
        if (factory==nullptr)
        {
            factory=this->unit()->factory();
        }

        auto& field=this->createAndAppendValue();
        if (!fieldType::deserialize(field,wired,factory))
        {
            return false;
        }

        /* ok */
        this->markSet();
        return this->isSet();
    }

    /**  Load fields from wire */
    virtual bool doLoad(WireData& wired, const AllocatorFactory* factory) override
    {
        return deserialize(wired,factory);
    }

    constexpr static bool fieldRepeatedUnpackedProtoBuf() noexcept
    {
        return true;
    }

    /** Check if this field is compatible of repeated unpacked type with Google Protocol Buffers */
    virtual bool isRepeatedUnpackedProtoBuf() const noexcept override
    {
        return fieldRepeatedUnpackedProtoBuf();
    }

    template <typename BufferT>
    bool serialize(BufferT& wired) const
    {
        constexpr auto tagWireType=fieldType::fieldWireType();
        constexpr auto id=Id;

        /* append each field with tag */
        for (size_t i=0;i<this->count();i++)
        {
            const auto& field=this->vector[i];

            /* append tag to sream */
            constexpr uint32_t tag=(id<<3)|static_cast<int>(tagWireType);
            if (base::CanChainBlocks && !wired.isSingleBuffer())
            {
                wired.appendUint32(tag);
            }
            else
            {
                wired.incSize(Stream<uint32_t>::packVarInt(wired.mainContainer(),tag));
            }

            /* append field to stream */
            if (!fieldType::serialize(field,wired))
            {
                return false;
            }
        }

        /* ok */
        return true;
    }

    /**  Store field to wire */
    virtual bool doStore(WireData& wired) const override
    {
        return serialize(wired);
    }
};

template <typename FieldName,typename Type,int Id,typename Tag,typename DefaultTraits=DefaultValue<Type>,bool Required=false>
struct RepeatedField : public FieldConf<
                           RepeatedFieldTmpl<Type,Id,DefaultTraits>,
                           Id,FieldName,Tag,Required>
{
    using FieldConf<
        RepeatedFieldTmpl<Type,Id,DefaultTraits>,
        Id,FieldName,Tag,Required>::FieldConf;
};

template <typename FieldName,typename Type,int Id,typename Tag,typename DefaultTraits=DefaultValue<Type>,bool Required=false>
struct RepeatedFieldProtoBufPacked : public FieldConf<
                                         RepeatedFieldProtoBufPackedTmpl<Type,Id,DefaultTraits>,
                                         Id,FieldName,Tag,Required>
{
    using FieldConf<
        RepeatedFieldProtoBufPackedTmpl<Type,Id,DefaultTraits>,
        Id,FieldName,Tag,Required>::FieldConf;
};

template <typename FieldName,typename Type,int Id,typename Tag,typename DefaultTraits=DefaultValue<Type>,bool Required=false>
struct RepeatedFieldProtoBufUnpacked : public FieldConf<
                                           RepeatedFieldProtoBufUnpackedTmpl<Type,Id,DefaultTraits>,
                                           Id,FieldName,Tag,Required>
{
    using FieldConf<
        RepeatedFieldProtoBufUnpackedTmpl<Type,Id,DefaultTraits>,
        Id,FieldName,Tag,Required>::FieldConf;
};

enum class RepeatedMode : int
{
    Auto,
    ProtobufPacked,
    ProtobufUnpacked,
    Counted
};

template <typename Type, RepeatedMode Mode, typename=hana::when<true>>
struct SelectRepeatedType
{
    template <typename FieldName,typename Type1,int Id,typename Tag,typename DefaultTraits=DefaultValue<Type>,bool Required=false>
    using type=RepeatedFieldProtoBufPacked<FieldName,Type1,Id,Tag,DefaultTraits,Required>;
};

template <typename Type, RepeatedMode Mode>
struct SelectRepeatedType<Type,Mode,hana::when<Mode==RepeatedMode::Counted>>
{
    template <typename FieldName,typename Type1,int Id,typename Tag,typename DefaultTraits=DefaultValue<Type>,bool Required=false>
    using type=RepeatedField<FieldName,Type1,Id,Tag,DefaultTraits,Required>;
};

template <typename Type, RepeatedMode Mode>
struct SelectRepeatedType<Type,Mode,hana::when<
                                          Mode==RepeatedMode::ProtobufPacked
                                                 ||
                                          (Mode==RepeatedMode::Auto && Type::isPackedProtoBufCompatible::value)
                                        >>
{
    template <typename FieldName,typename Type1,int Id,typename Tag,typename DefaultTraits=DefaultValue<Type>,bool Required=false>
    using type=RepeatedFieldProtoBufPacked<FieldName,Type1,Id,Tag,DefaultTraits,Required>;
};

template <typename Type, RepeatedMode Mode>
struct SelectRepeatedType<Type,Mode,hana::when<
                                          Mode==RepeatedMode::ProtobufUnpacked
                                                 ||
                                          (Mode==RepeatedMode::Auto && !Type::isPackedProtoBufCompatible::value)
                                    >>
{
    template <typename FieldName,typename Type1,int Id,typename Tag,typename DefaultTraits=DefaultValue<Type>,bool Required=false>
    using type=RepeatedFieldProtoBufUnpacked<FieldName,Type1,Id,Tag,DefaultTraits,Required>;
};

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITREPEATEDFIELDS_H
