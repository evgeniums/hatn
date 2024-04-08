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

#include <functional>
#include <tuple>
#include <type_traits>
#include <vector>
#include <array>

#include <boost/hana.hpp>

#include <hatn/common/containerutils.h>

#include <hatn/dataunit/fields/fieldtraits.h>
#include <hatn/dataunit/fields/subunit.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

/**
*  Helper for repeated fields
*/
template <typename Type> struct RepeatedTraits
{
   using fieldType=FieldTmpl<Type>;
   using valueType=typename Type::type;
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
           return ::hatn::common::ConstructWithArgsOrDefault<valueType,Unit*>::f(
                           std::forward<Unit*>(parentUnit)
                       );
       }
   };

   template <typename DefaultV> inline static valueType createValue(Unit* parentUnit)
   {
       return Ctor<DefaultV>::f(parentUnit);
   }
};
template <typename Type> struct RepeatedTraits<SharedUnitFieldTmpl<Type>>
{
   using fieldType=SharedUnitFieldTmpl<Type>;
   using valueType=typename Type::shared_type;
   constexpr static const bool isSizeIterateNeeded=true;
   constexpr static const ValueType typeId=Type::typeId;

   template <typename> inline static valueType createValue(Unit* parentUnit)
   {
       auto val=Type::createManagedObject(parentUnit->factory(),parentUnit);
       if (val.isNull())
       {
           HATN_ERROR(dataunit,"Cannot create managed object in shared dataunit field!");
           Assert(!val.isNull(),"Shared dataunit field is not set!");
       }
       return val;
   }
};

template <typename Type> struct RepeatedTraits<EmbeddedUnitFieldTmpl<Type>>
{
   using fieldType=EmbeddedUnitFieldTmpl<Type>;
   using valueType=typename Type::type;
   constexpr static const bool isSizeIterateNeeded=true;
   constexpr static const ValueType typeId=Type::typeId;

   template <typename> inline static valueType createValue(
               Unit* parentUnit
           )
   {
       return common::ConstructWithArgsOrDefault<valueType,Unit*>::f(std::forward<Unit*>(parentUnit));
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
    constexpr static void bufCreateShared(ArrayT&,size_t,AllocatorFactory*)
    {
        Assert(false,"Invalid operation for field of this type");
    }
    template <typename RepeatedT>
    constexpr static void bufAddValue(RepeatedT*,const char*, size_t)
    {
        Assert(false,"Invalid operation for field of this type");
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
template <typename valueType> struct RepeatedGetterSetterNoScalar
{
    template <typename T>
    constexpr static void setVal(valueType&,const T&)
    {
        Assert(false,"Invalid operation for field of this type");
    }
    template <typename T>
    constexpr static void getVal(const valueType&,T &)
    {
        Assert(false,"Invalid operation for field of this type");
    }
    template <typename ArrayT,typename T>
    constexpr static void addVal(ArrayT&,const T&)
    {
        Assert(false,"Invalid operation for field of this type");
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
   constexpr static void setVal(valueType& arrayVal,const T& val) noexcept
   {
       arrayVal=static_cast<valueType>(val);
   }
   template <typename T>
   constexpr static void getVal(const valueType& arrayVal,T &val) noexcept
   {
       val=static_cast<T>(arrayVal);
   }
   template <typename ArrayT,typename T>
   constexpr static void addVal(ArrayT& array,const T& val) noexcept
   {
       array.push_back(static_cast<valueType>(val));
   }
};
template <typename Type>
    struct RepeatedGetterSetter<Type,
                        std::enable_if_t<Type::isBytesType::value>
                    > : public RepeatedGetterSetterNoScalar<typename Type::type>,
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
    constexpr static void bufCreateShared(ArrayT& array,size_t idx,AllocatorFactory* factory)
    {
        fieldType::prepareSharedStorage(array[idx],factory);
    }
    template <typename RepeatedT>
    constexpr static void bufAddValue(RepeatedT* field,const char* data, size_t length)
    {
        field->addValue(data,length);
    }
};
template <typename Type> struct RepeatedGetterSetter<Type,
                                std::enable_if_t<Type::isUnitType::value>
                                > : public RepeatedGetterSetterNoScalar<typename Type::type>,
                                    public RepeatedGetterSetterNoBytes
{
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
};

struct RepeatedType{};

/**  Template class for repeated dataunit field */
template <typename Type, int Id, typename DefaultTraits>
struct RepeatedFieldTmpl : public Field, public RepeatedType
{
    using type=typename RepeatedTraits<Type>::valueType;
    using fieldType=typename RepeatedTraits<Type>::fieldType;
    using vectorType=::hatn::common::pmr::vector<type>;
    using isRepeatedType=std::true_type;
    using selfType=RepeatedFieldTmpl<Type,Id,DefaultTraits>;

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
       m_parseToSharedArrays(false),
       m_parentUnit(parentUnit)
    {}

    ~RepeatedFieldTmpl()=default;

    /**  Copy ctor */
    RepeatedFieldTmpl(const RepeatedFieldTmpl& other)
       : Field(other),
         vector(other.vector,other.m_parentUnit->factory()->template dataAllocator<type>()),
         m_parseToSharedArrays(other.m_parseToSharedArrays),
         m_parentUnit(other.m_parentUnit)
    {}

    /** Assignment **/
    RepeatedFieldTmpl& operator=(const RepeatedFieldTmpl& other) noexcept
    {
       if (this!=&other)
       {
           Field::operator =(other);
           std::copy(other.vector.begin(), other.vector.end(),
                         std::back_inserter(vector));
           m_parseToSharedArrays=other.m_parseToSharedArrays;
           m_parentUnit=other.m_parentUnit;
       }
       return *this;
    }

    /** Move ctor **/
    RepeatedFieldTmpl(RepeatedFieldTmpl&& other) =default;

    /** Move assignment **/
    RepeatedFieldTmpl& operator=(RepeatedFieldTmpl&& other) =default;

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
           auto& val=field.createAndAddValue();
           val.buf()->load(data,size);
       }
    };

    inline size_t addValue(const char* data, size_t size)
    {
       AddStringHelper<Type>::f(*this,data,size);
       return vector.size();
    }

    inline size_t addValue(const char* data)
    {
       return addValue(data,strlen(data));
    }

    inline size_t addValue(const std::string& str)
    {
       return addValue(str.data(),str.size());
    }

    /**  Add value */
    inline size_t addValue(type value)
    {
       this->markSet();
       vector.push_back(std::move(value));
       return vector.size();
    }

    template <typename ... Vals>
    inline size_t appendValues(Vals&&... vals)
    {
       this->markSet();        
       common::ContainerUtils::addElements(vector,std::forward<Vals>(vals)...);
       return vector.size();
    }

    /**  Create and add value */
    type& createAndAddValue()
    {
        this->markSet();
        auto val=RepeatedTraits<Type>::template createValue<DefaultTraits>(m_parentUnit);
        if (fieldIsParseToSharedArrays())
        {
            fieldType::prepareSharedStorage(val,m_parentUnit->factory());
        }
        vector.push_back(std::move(val));
        return vector.back();
    }

    /**  Add number of values */
    void addValues(size_t n, bool onlySizeIterate=false)
    {
        if (isSizeIterateNeeded || (DefaultTraits::HasDefV::value && !onlySizeIterate))
        {
            vector.reserve(vector.size()+n);
            for (size_t i=0;i<n;i++)
            {
                createAndAddValue();
            }
        }
        else
        {
            vector.resize(vector.size()+n);
            markSet();
        }
    }

    /**  Get number of repeated fields */
    inline size_t count() const noexcept
    {
       return vector.size();
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
       vector.resize(size);
       this->markSet();
    }

    /**  Clear array */
    inline void clearArray() noexcept
    {
       vector.clear();
    }

    /** Get expected field size */
    virtual size_t size() const noexcept override
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
    bool deserialize(BufferT& wired, AllocatorFactory* factory)
    {
        fieldClear();
        if (factory==nullptr)
        {
            factory=m_parentUnit->factory();
        }

        /* get count of elements in array */
        uint32_t counter=0;
        auto* buf=wired.mainContainer();
        size_t availableBufSize=buf->size()-wired.currentOffset();
        auto consumed=Stream<uint32_t>::unpackVarInt(buf->data()+wired.currentOffset(),availableBufSize,counter);
        if (consumed<0)
        {
            return false;
        }
        wired.incCurrentOffset(consumed);

        /* add required number of elements */
        addValues(counter,true);

        /* fill each field */
        for (uint32_t i=0;i<counter;i++)
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
    virtual bool doLoad(WireData& wired, AllocatorFactory* factory) override
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

    void fieldSetParseToSharedArrays(bool enable,AllocatorFactory* =nullptr)
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
            addValues(size-vector.size());
        }
        else
        {
            resize(size);
        }
    }
    virtual void arrayReserve(size_t size) override {reserve(size);}

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

    virtual void arrayAdd(bool val) override {RepeatedGetterSetter<Type>::addVal(vector,val);}
    virtual void arrayAdd(uint8_t val) override {RepeatedGetterSetter<Type>::addVal(vector,val);}
    virtual void arrayAdd(uint16_t val) override {RepeatedGetterSetter<Type>::addVal(vector,val);}
    virtual void arrayAdd(uint32_t val) override {RepeatedGetterSetter<Type>::addVal(vector,val);}
    virtual void arrayAdd(uint64_t val) override {RepeatedGetterSetter<Type>::addVal(vector,val);}
    virtual void arrayAdd(int8_t val) override {RepeatedGetterSetter<Type>::addVal(vector,val);}
    virtual void arrayAdd(int16_t val) override {RepeatedGetterSetter<Type>::addVal(vector,val);}
    virtual void arrayAdd(int32_t val) override {RepeatedGetterSetter<Type>::addVal(vector,val);}
    virtual void arrayAdd(int64_t val) override {RepeatedGetterSetter<Type>::addVal(vector,val);}
    virtual void arrayAdd(float val) override {RepeatedGetterSetter<Type>::addVal(vector,val);}
    virtual void arrayAdd(double val) override {RepeatedGetterSetter<Type>::addVal(vector,val);}

    virtual void arrayBufResize(size_t idx,size_t size) override {RepeatedGetterSetter<Type>::bufResize(vector,idx,size);}
    virtual void arrayBufReserve(size_t idx,size_t size) override {RepeatedGetterSetter<Type>::bufReserve(vector,idx,size);}
    virtual const char* arrayBufCStr(size_t idx) const override {return RepeatedGetterSetter<Type>::bufCStr(vector,idx);}
    virtual const char* arrayBufData(size_t idx) const override {return RepeatedGetterSetter<Type>::bufData(vector,idx);}
    virtual char* arrayBufData(size_t idx) override {return RepeatedGetterSetter<Type>::bufData(vector,idx);}
    virtual size_t arrayBufSize(size_t idx) const override {return RepeatedGetterSetter<Type>::bufSize(vector,idx);}
    virtual size_t arrayBufCapacity(size_t idx) const override {return RepeatedGetterSetter<Type>::bufCapacity(vector,idx);}
    virtual bool arrayBufEmpty(size_t idx) const override {return RepeatedGetterSetter<Type>::bufEmpty(vector,idx);}
    virtual void arrayBufCreateShared(size_t idx) override {RepeatedGetterSetter<Type>::bufCreateShared(vector,idx,m_parentUnit->factory());}
    virtual void arrayBufSetValue(size_t idx,const char* data,size_t length) override {RepeatedGetterSetter<Type>::bufSetValue(vector,idx,data,length);}
    virtual void arrayBufAddValue(const char* data,size_t length) override {RepeatedGetterSetter<Type>::bufAddValue(this,data,length);}

    virtual Unit* arraySubunit(size_t idx) override {return RepeatedGetterSetter<Type>::unit(vector,idx);}
    virtual const Unit* arraySubunit(size_t idx) const override {return RepeatedGetterSetter<Type>::unit(vector,idx);}

   protected:

       bool m_parseToSharedArrays;
       Unit* m_parentUnit;
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
   bool deserialize(BufferT& wired, AllocatorFactory* factory)
   {
       this->fieldClear();

       if (factory==nullptr)
       {
           factory=this->m_parentUnit->factory();
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
           HATN_WARN(dataunit,"Unexpected end of buffer");
           return false;
       }

       /* fill each field */
       size_t lastOffset=wired.currentOffset()+length;
       while(wired.currentOffset()<lastOffset)
       {
           auto& field=this->createAndAddValue();
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
   virtual bool doLoad(WireData& wired, AllocatorFactory* factory) override
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
   virtual size_t size() const noexcept override
   {
       return this->count()*sizeof(type)+5;
   }
};

/**  Template class for repeated dataunit field compatible with unpacked repeated fields of Google Protocol Buffers for ordinary types*/
template <typename Type, int Id, typename DefaultTraits>
struct RepeatedFieldProtoBufOrdinaryTmpl : public RepeatedFieldTmpl<Type,Id,DefaultTraits>
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
   virtual size_t size() const noexcept override
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
   bool deserialize(BufferT& wired, AllocatorFactory* factory)
   {
       if (factory==nullptr)
       {
           factory=this->m_parentUnit->factory();
       }

       auto& field=this->createAndAddValue();
       if (!fieldType::deserialize(field,wired,factory))
       {
           return false;
       }

       /* ok */
       this->markSet();
       return this->isSet();
   }

   /**  Load fields from wire */
   virtual bool doLoad(WireData& wired, AllocatorFactory* factory) override
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
    struct RepeatedFieldProtoBufOrdinary : public FieldConf<
            RepeatedFieldProtoBufOrdinaryTmpl<Type,Id,DefaultTraits>,
            Id,FieldName,Tag,Required>
{
    using FieldConf<
        RepeatedFieldProtoBufOrdinaryTmpl<Type,Id,DefaultTraits>,
        Id,FieldName,Tag,Required>::FieldConf;
};

enum class RepeatedMode : int
{
    Auto,
    ProtobufPacked,
    ProtobufOrdinary
};

template <RepeatedMode Mode>
struct SelectRepeatedType
{
    template <typename FieldName,typename Type,int Id,typename Tag,typename DefaultTraits=DefaultValue<Type>,bool Required=false>
    using type=RepeatedField<FieldName,Type,Id,Tag,DefaultTraits,Required>;
};

template <>
struct SelectRepeatedType<RepeatedMode::ProtobufPacked>
{
    template <typename FieldName,typename Type,int Id,typename Tag,typename DefaultTraits=DefaultValue<Type>,bool Required=false>
    using type=RepeatedFieldProtoBufPacked<FieldName,Type,Id,Tag,DefaultTraits,Required>;
};

template <>
struct SelectRepeatedType<RepeatedMode::ProtobufOrdinary>
{
    template <typename FieldName,typename Type,int Id,typename Tag,typename DefaultTraits=DefaultValue<Type>,bool Required=false>
    using type=RepeatedFieldProtoBufOrdinary<FieldName,Type,Id,Tag,DefaultTraits,Required>;
};

enum class RepeatedContentType : int
{
    Auto,
    External,
    Embedded
};

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITREPEATEDFIELDS_H
