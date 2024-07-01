/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file datauint/detail/types.h
  *
  *      Internal implementation of list of standard types of dataunit fields.
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITTYPESIMPL_H
#define HATNDATAUNITTYPESIMPL_H

#include <hatn/common/bytearray.h>
#include <hatn/common/logger.h>
#include <hatn/common/datetime.h>
#include <hatn/common/daterange.h>
#include <hatn/common/pmr/pmrtypes.h>
#include <hatn/common/meta/constructwithargordefault.h>

#include <hatn/dataunit/dataunit.h>
#include <hatn/dataunit/unit.h>
#include <hatn/dataunit/allocatorfactory.h>
#include <hatn/dataunit/valuetypes.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

namespace types
{

template <typename Type, typename isBasic, ValueType valueType>
struct BaseType
{
    using type=Type;
    using BasicType=isBasic;
    using isEnum=std::false_type;
    using Enum=std::false_type;
    using isPackedProtoBufCompatible=std::true_type;    
    using isUnitType=std::false_type;
    using isBytesType=std::false_type;
    using isStringType=std::false_type;
    using isRepeatedType=std::false_type;
    using maxSize=std::integral_constant<int,sizeof(type)>;

    constexpr static const bool isSizeIterateNeeded=false;
    constexpr static const ValueType typeId=valueType;
};

//! Definition of bool field type
struct HATN_DATAUNIT_EXPORT TYPE_BOOL : public BaseType<bool,std::true_type,ValueType::Bool>
{
};
//! Definition of signed int8 type
struct HATN_DATAUNIT_EXPORT TYPE_INT8 : public BaseType<int8_t,std::true_type,ValueType::Int8>
{
};
//! Definition of signed int16 type
struct HATN_DATAUNIT_EXPORT TYPE_INT16 : public BaseType<int16_t,std::true_type,ValueType::Int16>
{
};
//! Definition of signed int32 type
struct HATN_DATAUNIT_EXPORT TYPE_INT32 : public BaseType<int32_t,std::true_type,ValueType::Int32>
{
};
//! Definition of signed int32 type with fixed wire size
struct HATN_DATAUNIT_EXPORT TYPE_FIXED_INT32 : public BaseType<int32_t,std::true_type,ValueType::Int32>
{
};
//! Definition of signed int64 type
struct HATN_DATAUNIT_EXPORT TYPE_INT64 : public BaseType<int64_t,std::true_type,ValueType::Int64>
{
};
//! Definition of signed int64 type with fixed wire size
struct HATN_DATAUNIT_EXPORT TYPE_FIXED_INT64 : public BaseType<int64_t,std::true_type,ValueType::Int64>
{
};
//! Definition of unsigned int8 type
struct HATN_DATAUNIT_EXPORT TYPE_UINT8 : public BaseType<uint8_t,std::true_type,ValueType::UInt8>
{
};
//! Definition of unsigned int16 type
struct HATN_DATAUNIT_EXPORT TYPE_UINT16 : public BaseType<uint16_t,std::true_type,ValueType::UInt16>
{
};
//! Definition of unsigned int32 type
struct HATN_DATAUNIT_EXPORT TYPE_UINT32 : public BaseType<uint32_t,std::true_type,ValueType::UInt32>
{
};
//! Definition of unsigned int32 type
struct HATN_DATAUNIT_EXPORT TYPE_FIXED_UINT32 : public BaseType<uint32_t,std::true_type,ValueType::UInt32>
{
};
//! Definition of unsigned int64 type
struct HATN_DATAUNIT_EXPORT TYPE_UINT64 : public BaseType<uint64_t,std::true_type,ValueType::UInt64>
{
};
//! Definition of unsigned int64 type with fixed wire size
struct HATN_DATAUNIT_EXPORT TYPE_FIXED_UINT64 : public BaseType<uint64_t,std::true_type,ValueType::UInt64>
{
};
//! Definition of float type
struct HATN_DATAUNIT_EXPORT TYPE_FLOAT : public BaseType<float,std::true_type,ValueType::Float>
{
};
//! Definition of double type
struct HATN_DATAUNIT_EXPORT TYPE_DOUBLE : public BaseType<double,std::true_type,ValueType::Double>
{
};

namespace detail
{

template <typename sharedT, typename onstackT, typename managedT>
struct BytesTraits
{
    using onstackType=onstackT;
    using sharedType=sharedT;
    using managedType=managedT;

    BytesTraits()=default;

    BytesTraits(
                Unit* parentUnit
            ) : onstack(
                        common::ConstructWithArgsOrDefault<onstackType,decltype(parentUnit->factory()->dataMemoryResource())>::f(
                            std::forward<decltype(parentUnit->factory()->dataMemoryResource())>(parentUnit->factory()->dataMemoryResource())
                        )
                    )
    {}

    inline onstackType& byteArray() noexcept
    {
        return onstack;
    }

    inline sharedType& byteArrayShared() noexcept
    {
        return shared;
    }

    inline const onstackType& byteArray() const noexcept
    {
        return onstack;
    }

    inline const sharedType& byteArrayShared() const noexcept
    {
        return shared;
    }

    inline void setSharedByteArray(sharedType val)
    {
        shared=std::move(val);
    }

    inline void clear()
    {
        if (!byteArrayShared().isNull())
        {
            byteArrayShared().reset();
        }
        else
        {
            byteArray().clear();
        }
    }

    inline void reset()
    {
        if (!byteArrayShared().isNull())
        {
            byteArrayShared().reset();
        }
        else
        {
            byteArray().reset();
        }
    }

    inline size_t size() const noexcept
    {
        if (!byteArrayShared().isNull())
        {
            // return size of data plus size of unpacked length field
            return byteArrayShared()->size()+sizeof(uint32_t);
        }
        // return size of data plus size of unpacked length field
        return byteArray().size()+sizeof(uint32_t);
    }

    inline onstackType* buf() noexcept
    {
        onstackType* arr=shared.get();
        if (arr==nullptr)
        {
            arr=&onstack;
        }
        return arr;
    }

    inline const onstackType* buf() const noexcept
    {
        const onstackType* arr=shared.get();
        if (arr==nullptr)
        {
            arr=&onstack;
        }
        return arr;
    }

    //! Set data from buffer
    inline void set(const char* ptr,size_t size)
    {
        buf()->load(ptr,size);
    }

    //! Set data from null-terminated c-string
    inline void set(const char* ptr)
    {
        buf()->load(ptr);
    }

    //! Set data from string
    inline void set(
        const std::string& str
        )
    {
        buf()->load(str);
    }

    //! Set data from container
    template <typename ContainerT>
    inline void set(
        const ContainerT& container
        )
    {
        buf()->load(container);
    }

    //! Get C-string
    inline const char* c_str() const noexcept
    {
        return buf()->c_str();
    }

    //! Get pointer to data
    inline char* dataPtr() noexcept
    {
        return buf()->data();
    }

    //! Get pointer to data
    inline char* dataPtr() const noexcept
    {
        return buf()->data();
    }

    //! Get data size
    inline size_t dataSize() const noexcept
    {
        return buf()->size();
    }

    private:

        sharedType shared;
        onstackType onstack;
};

struct BytesType : public BytesTraits<common::ByteArrayShared, common::ByteArray, common::ByteArrayManaged>
{
    using isBytesType=std::true_type;
    using canChainBlocks=std::true_type;
    using maxSize=std::integral_constant<int,-1>;

    using BytesTraits<common::ByteArrayShared, common::ByteArray, common::ByteArrayManaged>::BytesTraits;

    static sharedType createShared(::hatn::dataunit::AllocatorFactory* factory)
    {
        return factory->createObject<managedType>(factory->dataMemoryResource());
    }
};

template <size_t length> struct FixedString : public BytesTraits<common::SharedPtr<common::FixedByteArrayManaged<length,true>>,
                                                                                common::FixedByteArray<length,true>,
                                                                                common::FixedByteArrayManaged<length,true>
                                                                                >
{
    using isStringType=std::true_type;

    using canChainBlocks=std::false_type;
    using maxSize=std::integral_constant<int,length>;

    using managedType=common::FixedByteArrayManaged<length,true>;
    using sharedType=common::SharedPtr<managedType>;

    using BytesTraits<common::SharedPtr<common::FixedByteArrayManaged<length,true>>,
            common::FixedByteArray<length,true>,
            common::FixedByteArrayManaged<length,true>
            >::BytesTraits;

    static sharedType createShared(::hatn::dataunit::AllocatorFactory* factory)
    {
        return factory->createObject<managedType>();
    }
};

}

//! Definition of bytes type
struct HATN_DATAUNIT_EXPORT TYPE_BYTES : public BaseType<detail::BytesType,std::true_type,ValueType::Bytes>
{
    using isBytesType=std::true_type;

    using isPackedProtoBufCompatible=std::false_type;
    constexpr static const bool isSizeIterateNeeded=true;
};

//! Definition of string type
struct HATN_DATAUNIT_EXPORT TYPE_STRING : public TYPE_BYTES
{
    using isStringType=std::true_type;
    constexpr static const ValueType typeId=ValueType::String;
};

//! Definition of fixed string type
template <int length> struct HATN_DATAUNIT_EXPORT TYPE_FIXED_STRING
        : public BaseType<
                        detail::FixedString<length>,
                        std::true_type,
                        ValueType::String
                    >
{
    static_assert( \
            length==8 || \
            length==16 || \
            length==20 || \
            length==24 || \
            length==32 || \
            length==40 || \
            length==64 || \
            length==128 || \
            length==256 || \
            length==512 || \
            length==1024 \
    ,"Length of fixed string must be one of the following: [8,16,20,32,40,64,128,256,512,1024]");

    using isBytesType=std::true_type;
    using isStringType=std::true_type;

    using isPackedProtoBufCompatible=std::false_type;
    constexpr static const bool isSizeIterateNeeded=true;

    using maxSize=std::integral_constant<int,length>;
};

/**
 * @brief  Definition of embedded DataUnit type
 *
 * This DataUnit type must be used as alias of outer unit's type.
 *
 */
struct HATN_DATAUNIT_EXPORT TYPE_DATAUNIT : public BaseType<Unit,std::false_type,ValueType::Dataunit>
{
    using isUnitType=std::true_type;
    using isPackedProtoBufCompatible=std::false_type;

    using base_shared_type=Unit;
    using shared_type=::hatn::common::SharedPtr<Unit>;

    static shared_type createManagedObject(AllocatorFactory*,Unit* parentUnit)
    {
        return parentUnit->createManagedUnit();
    }
};

//! Definition of enum type
template <typename EnumT>
struct TYPE_ENUM : public BaseType<uint32_t,std::true_type,ValueType::Int8>
{
    using isEnum=std::true_type;
    using Enum=EnumT;
};

//! Definition of DateTime type
struct HATN_DATAUNIT_EXPORT TYPE_DATETIME : public BaseType<common::DateTime,std::true_type,ValueType::DateTime>
{
    using CustomType=std::true_type;
};

//! Definition of Date type
struct HATN_DATAUNIT_EXPORT TYPE_DATE : public BaseType<common::Date,std::true_type,ValueType::Date>
{
    using CustomType=std::true_type;
};

//! Definition of Time type
struct HATN_DATAUNIT_EXPORT TYPE_TIME : public BaseType<common::Time,std::true_type,ValueType::Time>
{
    using CustomType=std::true_type;
};

//! Definition of DateRange type
struct HATN_DATAUNIT_EXPORT TYPE_DATE_RANGE : public BaseType<common::DateRange,std::true_type,ValueType::DateRange>
{
    using CustomType=std::true_type;
};

}

HATN_DATAUNIT_NAMESPACE_END

#endif// HATNDATAUNITTYPESIMPL_H
