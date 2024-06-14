/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/scalar.h
  *
  *      DataUnit fields of scalar types
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITSCALARFIELDS_H
#define HATNDATAUNITSCALARFIELDS_H

#include <type_traits>

#include <hatn/dataunit/stream.h>
#include <hatn/dataunit/fields/fieldtraits.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

//---------------------------------------------------------------

//! Helper fpr comparation of scalar fields.
template <typename LeftT, typename RightT, typename=void>
struct ScalarCompare
{
    constexpr static bool less(const LeftT& left, const RightT& right) noexcept
    {
        return left < right;
    }
    constexpr static bool equal(const LeftT& left, const RightT& right) noexcept
    {
        return left == right;
    }
};

template <typename LeftT, typename RightT>
struct ScalarCompare<LeftT,RightT,
                        std::enable_if_t<
                            std::is_signed<LeftT>::value && 
                            !std::is_floating_point<LeftT>::value && 
                            std::is_unsigned<RightT>::value &&
                            !std::is_same<RightT, bool>::value
                        >
                    >
{
    constexpr static bool less(const LeftT& left, const RightT& right) noexcept
    {
        if (left < 0)
        {
            return true;
        }
        return static_cast<std::make_unsigned_t<LeftT>>(left) < right;
    }
    constexpr static bool equal(const LeftT& left, const RightT& right) noexcept
    {
        return static_cast<std::make_unsigned_t<LeftT>>(left) == right;
    }
};

template <typename LeftT, typename RightT>
struct ScalarCompare<LeftT, RightT,
        std::enable_if_t<
            std::is_unsigned<LeftT>::value && 
            std::is_signed<RightT>::value && 
            !std::is_floating_point<RightT>::value &&
            !std::is_same<LeftT, bool>::value>
    >
{
    constexpr static bool less(const LeftT& left, const RightT& right) noexcept
    {
        if (right < 0)
        {
            return false;
        }
        return left < static_cast<std::make_unsigned_t<RightT>>(right);
    }
    constexpr static bool equal(const LeftT& left, const RightT& right) noexcept
    {
        return left == static_cast<std::make_unsigned_t<RightT>>(right);
    }
};

template <typename LeftT,typename RightT>
struct ScalarCompare<LeftT,RightT,
                    std::enable_if_t<!std::is_same<LeftT, bool>::value && std::is_same<RightT, bool>::value>
                >
{
    constexpr static bool less(const LeftT& left, const RightT& right) noexcept
    {
        return static_cast<bool>(left) < right;
    }
    constexpr static bool equal(const LeftT& left, const RightT& right) noexcept
    {
        return static_cast<bool>(left)==right;
    }
};
template <typename LeftT, typename RightT>
struct ScalarCompare<LeftT, RightT,
        std::enable_if_t<std::is_same<LeftT, bool>::value && !std::is_same<RightT, bool>::value>
    >
{
    constexpr static bool less(const LeftT& left, const RightT& right) noexcept
    {
        return left < static_cast<bool>(right);
    }

    constexpr static bool equal(const LeftT& left, const RightT& right) noexcept
    {
        return left==static_cast<bool>(right);
    }
};

//---------------------------------------------------------------

//! Base template class for scalar fields.
template <typename Type>
class Scalar : public Field
{
    public:

        using type=typename FTraits<Type,false>::type;
        using base=typename FTraits<Type,false>::base;

        using isRepeatedType=std::false_type;
        using isEnum=typename Type::isEnum;
        using Enum=typename Type::Enum;
        using selfType=Scalar<Type>;
        constexpr static auto typeId=Type::typeId;
        constexpr static auto isArray=isRepeatedType{};

        //! Ctor with default value
        Scalar(Unit* unit,const type& defaultValue)
            : Field(Type::typeId,unit),
              m_value(defaultValue)
        {
        }

        //! Ctor with parent unit
        explicit Scalar(Unit* unit)
            : Field(Type::typeId,unit),
              m_value(common::ConstructWithArgsOrDefault<type,type>::f(static_cast<type>(0)))
        {
        }

        //! Get field
        inline auto get() noexcept
        {
            return m_value;
        }

        //! Get const field
        inline auto get() const noexcept
        {
            return m_value;
        }

        //! Get value
        inline auto value() const noexcept
        {
            return m_value;
        }

        //! Set field
        inline void set(const type& val)
        {
            this->markSet(true);
            m_value=val;
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
            return DummyConst<type>::f();
        }

        //! Clear field
        void fieldClear()
        {
            m_value=fieldDefaultValue();
        }

        //! Reset field
        void fieldReset()
        {
            fieldClear();
            this->markSet(false);
        }

        //! Get field size
        virtual size_t size() const noexcept override
        {
            return fieldSize();
        }

        //! Get size of value
        static size_t valueSize(const type&) noexcept
        {
            return sizeof(type);
        }

        constexpr static size_t fieldSize() noexcept
        {
            return sizeof(type);
        }

        //! Prepare shared form of value storage for parsing from wire
        inline static void prepareSharedStorage(type& /*value*/,AllocatorFactory*)
        {
        }

        //! Create value
        virtual type createValue(AllocatorFactory* =AllocatorFactory::getDefault()) const
        {
            return type();
        }

        //! Format as JSON element
        template <typename Y> inline static bool formatJSON(const Y& val,
                               json::Writer* writer
                )
        {
            return json::Fieldwriter<Y>::write(val,writer);
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

        template <typename T>
        inline bool lessThan(const T &val) const noexcept
        {
            return ScalarCompare<type,T>::less(m_value,val);
        }
        template <typename T>
        inline bool equalTo(const T &val) const noexcept
        {
            return ScalarCompare<type,T>::equal(m_value, val);
        }

        template <typename T>
        inline void setVal(const T &val) noexcept
        {
            m_value=static_cast<decltype(m_value)>(val);
        }
        template <typename T>
        inline void getVal(T &val) const noexcept
        {
            val=static_cast<T>(m_value);
        }

        virtual bool less(bool val) const override {return lessThan(val);}
        virtual bool less(uint8_t val) const override {return lessThan(val);}
        virtual bool less(uint16_t val) const override {return lessThan(val);}
        virtual bool less(uint32_t val) const override {return lessThan(val);}
        virtual bool less(uint64_t val) const override {return lessThan(val);}
        virtual bool less(int8_t val) const override {return lessThan(val);}
        virtual bool less(int16_t val) const override {return lessThan(val);}
        virtual bool less(int32_t val) const override {return lessThan(val);}
        virtual bool less(int64_t val) const override {return lessThan(val);}
        virtual bool less(float val) const override {return lessThan(val);}
        virtual bool less(double val) const override {return lessThan(val);}

        virtual bool equals(bool val) const override {return equalTo(val);}
        virtual bool equals(uint8_t val) const override {return equalTo(val);}
        virtual bool equals(uint16_t val) const override {return equalTo(val);}
        virtual bool equals(uint32_t val) const override {return equalTo(val);}
        virtual bool equals(uint64_t val) const override {return equalTo(val);}
        virtual bool equals(int8_t val) const override {return equalTo(val);}
        virtual bool equals(int16_t val) const override {return equalTo(val);}
        virtual bool equals(int32_t val) const override {return equalTo(val);}
        virtual bool equals(int64_t val) const override {return equalTo(val);}
        virtual bool equals(float val) const override {return equalTo(val);}
        virtual bool equals(double val) const override {return equalTo(val);}

        virtual void setV(bool val) override {setVal(val);}
        virtual void setV(uint8_t val) override {setVal(val);}
        virtual void setV(uint16_t val) override {setVal(val);}
        virtual void setV(uint32_t val) override {setVal(val);}
        virtual void setV(uint64_t val) override {setVal(val);}
        virtual void setV(int8_t val) override {setVal(val);}
        virtual void setV(int16_t val) override {setVal(val);}
        virtual void setV(int32_t val) override {setVal(val);}
        virtual void setV(int64_t val) override {setVal(val);}
        virtual void setV(float val) override {setVal(val);}
        virtual void setV(double val) override {setVal(val);}

        virtual void getV(bool& val) const override {getVal(val);}
        virtual void getV(uint8_t& val) const override {getVal(val);}
        virtual void getV(uint16_t& val) const override {getVal(val);}
        virtual void getV(uint32_t& val) const override {getVal(val);}
        virtual void getV(uint64_t& val) const override {getVal(val);}
        virtual void getV(int8_t& val) const override {getVal(val);}
        virtual void getV(int16_t& val) const override {getVal(val);}
        virtual void getV(int32_t& val) const override {getVal(val);}
        virtual void getV(int64_t& val) const override {getVal(val);}
        virtual void getV(float& val) const override {getVal(val);}
        virtual void getV(double& val) const override {getVal(val);}

    protected:

        type m_value;
};

//---------------------------------------------------------------

//! Base class template for fixed size fields on wire.
template <typename Type>
class VarInt : public Scalar<Type>
{
    public:

        using Scalar<Type>::Scalar;
        using type=typename Type::type;

        template <typename BufferT>
        static bool serialize(const typename Type::type& val, BufferT& wired)
        {
            return VariableSer<typename Type::type>::serialize(val,wired);
        }

        template <typename BufferT>
        static bool deserialize(typename Type::type& val, BufferT& wired, AllocatorFactory*)
        {
            return VariableSer<typename Type::type>::deserialize(val,wired);
        }

        template <typename BufferT>
        bool serialize(BufferT& wired) const
        {
            return VariableSer<typename Type::type>::serialize(this->m_value,wired);
        }

        template <typename BufferT>
        bool deserialize(BufferT& wired, AllocatorFactory*)
        {
            this->markSet(VariableSer<typename Type::type>::deserialize(this->m_value,wired));
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
};

//---------------------------------------------------------------

//! Base class template for fixed size fields on wire
template <typename Type>
class IntEnum : public VarInt<Type>
{
    public:

        using VarInt<Type>::VarInt;

        //! Get const enum field
        inline auto get() const noexcept
        {
            return static_cast<typename Type::Enum>(this->m_value);
        }

        //! Get value
        inline auto value() const noexcept
        {
            return get();
        }

        //! Set enum field
        inline void set(const typename Type::Enum& val) noexcept
        {
            this->markSet(true);
            this->m_value=static_cast<uint32_t>(val);
        }
};

//---------------------------------------------------------------

//! Base class template for fixed size fields on wire
template <typename Type>
class Fixed : public Scalar<Type>
{
    public:

        using Scalar<Type>::Scalar;

        //! Serialize to wire.
        template <typename BufferT>
        static bool serialize(const typename Type::type& value, BufferT& wired)
        {
            return FixedSer<typename Type::type>::serialize(value,wired);
        }

        template <typename BufferT>
        bool serialize(BufferT& wired) const
        {
            return FixedSer<typename Type::type>::serialize(this->m_value,wired);
        }

        //! Deserialize from wire.
        template <typename BufferT>
        static bool deserialize(typename Type::type& value, BufferT& wired, AllocatorFactory*)
        {
            return FixedSer<typename Type::type>::deserialize(value,wired);
        }

        template <typename BufferT>
        bool deserialize(BufferT& wired, AllocatorFactory*)
        {
            this->markSet(FixedSer<typename Type::type>::deserialize(this->m_value,wired));
            return this->isSet();
        }

    protected:

        //! Load field from wire
        virtual bool doLoad(WireData& wired,AllocatorFactory *factory) override
        {
            return deserialize(this->m_value,wired,factory);
        }

        //! Store field to wire
        virtual bool doStore(WireData& wired) const override
        {
            return serialize(this->m_value,wired);
        }
};

//---------------------------------------------------------------

/**  Base field template for fixed wire integer types */
template <typename Type, WireType wTp= WireType::Fixed32>
struct IntFixed : public Fixed<Type>
{
    using Fixed<Type>::Fixed;
    constexpr static WireType fieldWireType() noexcept
    {
        return wTp;
    }
    //! Get wire type of the field
    virtual WireType wireType() const noexcept override
    {
        return fieldWireType();
    }
};

/**  Field template for bool type */
template <>
    struct FieldTmpl<TYPE_BOOL>
        : public VarInt<TYPE_BOOL>
{
    using VarInt<TYPE_BOOL>::VarInt;
};

/**  Field template for float type */
template <>
    struct FieldTmpl<TYPE_FLOAT>
        : public Fixed<TYPE_FLOAT>
{
    using Fixed<TYPE_FLOAT>::Fixed;

    constexpr static WireType fieldWireType() noexcept
    {
        return WireType::Fixed32;
    }
    //! Get wire type of the field
    virtual WireType wireType() const noexcept override
    {
        return fieldWireType();
    }
};
//---------------------------------------------------------------

// specializations of field descriptors

template <>
struct FieldTmpl<TYPE_DOUBLE> : public Fixed<TYPE_DOUBLE>
{
    using Fixed<TYPE_DOUBLE>::Fixed;

    constexpr static WireType fieldWireType() noexcept
    {
        return WireType::Fixed64;
    }
    //! Get wire type of the field
    virtual WireType wireType() const noexcept override
    {
        return fieldWireType();
    }
};

template <>
struct FieldTmpl<TYPE_INT8> : public VarInt<TYPE_INT8>
{
    using VarInt<TYPE_INT8>::VarInt;
};

template <>
struct FieldTmpl<TYPE_INT16> : public VarInt<TYPE_INT16>
{
    using VarInt<TYPE_INT16>::VarInt;
};

template <>
struct FieldTmpl<TYPE_INT32> : public VarInt<TYPE_INT32>
{
    using VarInt<TYPE_INT32>::VarInt;
};

template <template <typename EnumType> class _Enum,typename EnumType>
struct FieldTmpl<_Enum<EnumType>> : public IntEnum<_Enum<EnumType>>
{
    using IntEnum<_Enum<EnumType>>::IntEnum;
};

template <> struct FieldTmpl<TYPE_FIXED_INT32> : public IntFixed<TYPE_FIXED_INT32,WireType::Fixed32>
{
    using IntFixed<TYPE_FIXED_INT32,WireType::Fixed32>::IntFixed;
};

template <>
struct FieldTmpl<TYPE_INT64> : public VarInt<TYPE_INT64>
{
    using VarInt<TYPE_INT64>::VarInt;
};

template <>
struct FieldTmpl<TYPE_FIXED_INT64> : public IntFixed<TYPE_FIXED_INT64,WireType::Fixed64>
{
    using IntFixed<TYPE_FIXED_INT64,WireType::Fixed64>::IntFixed;
};

template <>
struct FieldTmpl<TYPE_UINT8> : public VarInt<TYPE_UINT8>
{
    using VarInt<TYPE_UINT8>::VarInt;
};

template <>
struct FieldTmpl<TYPE_UINT16> : public VarInt<TYPE_UINT16>
{
    using VarInt<TYPE_UINT16>::VarInt;
};

template <>
struct FieldTmpl<TYPE_UINT32> : public VarInt<TYPE_UINT32>
{
    using VarInt<TYPE_UINT32>::VarInt;
};

template <>
struct FieldTmpl<TYPE_FIXED_UINT32> : public IntFixed<TYPE_FIXED_UINT32,WireType::Fixed32>
{
    using IntFixed<TYPE_FIXED_UINT32,WireType::Fixed32>::IntFixed;
};

template <>
struct FieldTmpl<TYPE_UINT64> : public VarInt<TYPE_UINT64>
{
    using VarInt<TYPE_UINT64>::VarInt;
};

template <>
struct FieldTmpl<TYPE_FIXED_UINT64> : public IntFixed<TYPE_FIXED_UINT64,WireType::Fixed64>
{
    using IntFixed<TYPE_FIXED_UINT64,WireType::Fixed64>::IntFixed;
};

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITSCALARFIELDS_H
