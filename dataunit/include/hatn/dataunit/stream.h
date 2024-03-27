/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/stream.h
  *
  *      Classes for serializing data to stream and deserializing data from stream
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITSTREAM_H
#define HATNDATAUNITSTREAM_H

#include <functional>
#include <tuple>
#include <type_traits>

#include <boost/endian/conversion.hpp>

#include <hatn/common/metautils.h>
#include <hatn/common/bytearray.h>
#include <hatn/common/pmr/pmrtypes.h>
#include <hatn/common/sharedptr.h>
#include <hatn/common/logger.h>

#include <hatn/dataunit/dataunit.h>
#include <hatn/dataunit/types.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

//! Base serializer/deserializer
struct HATN_DATAUNIT_EXPORT StreamBase
{
    static bool unpackVarIntByte
    (
        const char *buf,
        uint32_t& result,
		size_t availableSize,
		int&processedSize,
        bool &overflow,
        bool& moreBytesLeft,
        uint32_t &shift,
        uint32_t maxShift
    ) noexcept;

    static int unpackVarInt32(
        const char* buf,
		size_t availableSize,
        uint32_t& value,
        bool& moreBytesLeft
    ) noexcept;

    static int unpackVarInt64(
        const char* buf,
		size_t availableSize,
        uint64_t& value,
        bool& moreBytesLeft
    ) noexcept;

    static int packVarInt32(
        common::ByteArray* buf,
        const uint32_t& value
    );

    /**
     * @brief Pack value to wired
     * @param buf Must be 5 bytes length
     * @param value Value to pack
     */
    static void packVarInt32(
        char* buf,
        const uint32_t& value
    );

    static int packVarInt64(
        common::ByteArray* buf,
        const uint64_t& value
    );

    // ZigZag Transform:  Encodes signed integers so that they can be
    // effectively used with varint encoding.
    //
    // varint operates on unsigned integers, encoding smaller numbers into
    // fewer bytes.  If you try to use it on a signed integer, it will treat
    // this number as a very large unsigned integer, which means that even
    // small signed numbers like -1 will take the maximum number of bytes
    // (10) to encode.  ZigZagEncode() maps signed integers to unsigned
    // in such a way that those with a small absolute value will have smaller
    // encoded values, making them appropriate for encoding using varint.
    //
    //       int32 ->     uint32
    // -------------------------
    //           0 ->          0
    //          -1 ->          1
    //           1 ->          2
    //          -2 ->          3
    //         ... ->        ...
    //  2147483647 -> 4294967294
    // -2147483648 -> 4294967295
    //
    //        >> encode >>
    //        << decode <<

    inline static uint32_t zigZagEncode32(int32_t n) noexcept
    {
      // Note:  the right-shift must be arithmetic
      return (n << 1) ^ (n >> 31);
    }

    inline static int32_t zigZagDecode32(uint32_t n) noexcept
    {
      return (n >> 1) ^ -static_cast<int32_t>(n & 1);
    }

    inline static uint64_t zigZagEncode64(int64_t n) noexcept
    {
      // Note:  the right-shift must be arithmetic
      return (n << 1) ^ (n >> 63);
    }

    inline static int64_t zigZagDecode64(uint64_t n) noexcept
    {
      return (n >> 1) ^ -static_cast<int64_t>(n & 1);
    }
};

//! Serialize/deserialize unsigned integer types
template <typename Type> struct StreamUnsigned
{
    /**
     * @brief Unpack VarInt and copy it to field
     * @param buf Buffer with VarInt to unpack
     * @param availableSize Size available in wired buffer
     * @param field Field to copy result to
     * @param fieldSize Size of the field
     * @return Number of bytes occupied by VarInt or -1 if data mailformed and available size is not enough to unpack VarInt
     */
    static inline int unpackVarInt(
        const char* buf,
        size_t availableSize,
        Type& field
    ) noexcept
    {
        uint32_t value=0;
        bool moreBytesLeft=false;
        auto processedSize=StreamBase::unpackVarInt32(buf,availableSize,value,moreBytesLeft);
        if (processedSize<0||moreBytesLeft)
        {
            return -1;
        }
#ifdef _MSC_VER
#pragma warning(disable:4800)
#endif
        field=static_cast<Type>(value);
#ifdef _MSC_VER
#pragma warning(default:4800)
#endif

        return processedSize;
    }

    /**
     * @brief Pack VarInt to buffer
     * @param buf Buffer to pack to
     * @param field Field to pack
     * @return Number of bytes occupied by VarInt or -1 or on failure
     */
    static inline int packVarInt(
        common::ByteArray* buf,
        const Type& field
    )
    {
        uint32_t value=static_cast<uint32_t>(field);
        auto processedSize=StreamBase::packVarInt32(buf,value);
        return processedSize;
    }
};

//! Serialize/deserialize signed integer types
template <typename Type> struct StreamSigned
{
    /**
     * @brief Unpack VarInt and copy it to field
     * @param buf Buffer with VarInt to unpack
     * @param availableSize Size available in wired buffer
     * @param field Field to copy result to
     * @param fieldSize Size of the field
     * @return Number of bytes occupied by VarInt or -1 if data mailformed and available size is not enough to unpack VarInt
     */
    static inline int unpackVarInt(
        const char* buf,
		size_t availableSize,
        Type& field
    ) noexcept
    {
        uint32_t value=0;
        bool moreBytesLeft=false;
        auto processedSize=StreamBase::unpackVarInt32(buf,availableSize,value,moreBytesLeft);
        if (processedSize<0||moreBytesLeft)
        {
            return -1;
        }
        auto&& signedVal=StreamBase::zigZagDecode32(value);
        field=static_cast<Type>(signedVal);
        return processedSize;
    }

    /**
     * @brief Pack VarInt to buffer
     * @param buf Buffer to pack to
     * @param field Field to pack
     * @return Number of bytes occupied by VarInt or -1 or on failure
     */
    static inline int packVarInt(
        common::ByteArray* buf,
        const Type& field
    )
    {
        uint32_t value=StreamBase::zigZagEncode32(static_cast<int32_t>(field));
        auto processedSize=StreamBase::packVarInt32(buf,value);
        if (processedSize<0)
        {
            return -1;
        }
        return processedSize;
    }
};

//! Template classes of streams
template <typename Type> struct Stream : public StreamUnsigned<Type>
{
};

template <> struct Stream<uint32_t>
{
    /**
     * @brief Unpack VarInt and copy it to field
     * @param buf Buffer with VarInt to unpack
     * @param availableSize Size available in wired buffer
     * @param field Field to copy result to
     * @param fieldSize Size of the field
     * @return Number of bytes occupied by VarInt or -1 if data mailformed and available size is not enough to unpack VarInt
     */
    static inline int unpackVarInt(
        const char* buf,
		size_t availableSize,
        uint32_t& field
    ) noexcept
    {
        bool moreBytesLeft=false;
        auto processedSize=StreamBase::unpackVarInt32(buf,availableSize,field,moreBytesLeft);
        if (processedSize<0||moreBytesLeft)
        {
            return -1;
        }
        return processedSize;
    }

    /**
     * @brief Pack VarInt to buffer
     * @param buf Buffer to pack to
     * @param field Field to pack
     * @return Number of bytes occupied by VarInt or -1 or on failure
     */
    static inline int packVarInt(
        common::ByteArray* buf,
        const uint32_t& field
    )
    {
        auto processedSize=StreamBase::packVarInt32(buf,field);
        if (processedSize<0)
        {
            return -1;
        }
        return processedSize;
    }
};

template <> struct Stream<int8_t> : public StreamSigned<int8_t>
{
};
template <> struct Stream<int16_t> : public StreamSigned<int16_t>
{
};
template <> struct Stream<int32_t> : public StreamSigned<int32_t>
{
};

template <> struct Stream<uint64_t>
{
    /**
     * @brief Unpack VarInt and copy it to field
     * @param buf Buffer with VarInt to unpack
     * @param availableSize Size available in wired buffer
     * @param field Field to copy result to
     * @param fieldSize Size of the field
     * @return Number of bytes occupied by VarInt or -1 if data mailformed and available size is not enough to unpack VarInt
     */
    static inline int unpackVarInt(
        const char* buf,
		size_t availableSize,
        uint64_t& field
    ) noexcept
    {
        bool moreBytesLeft=false;
        auto processedSize=StreamBase::unpackVarInt64(buf,availableSize,field,moreBytesLeft);
        if (processedSize<0||moreBytesLeft)
        {
            return -1;
        }
        return processedSize;
    }

    /**
     * @brief Pack VarInt to buffer
     * @param buf Buffer to pack to
     * @param field Field to pack
     * @return Number of bytes occupied by VarInt or -1 or on failure
     */
    static inline int packVarInt(
        common::ByteArray* buf,
        const uint64_t& field
    )
    {
        auto processedSize=StreamBase::packVarInt64(buf,field);
        if (processedSize<0)
        {
            return -1;
        }
        return processedSize;
    }
};

template <> struct Stream<int64_t>
{
    /**
     * @brief Unpack VarInt and copy it to field
     * @param buf Buffer with VarInt to unpack
     * @param availableSize Size available in wired buffer
     * @param field Field to copy result to
     * @param fieldSize Size of the field
     * @return Number of bytes occupied by VarInt or -1 if data mailformed and available size is not enough to unpack VarInt
     */
    static inline int unpackVarInt(
        const char* buf,
		size_t availableSize,
        int64_t& field
    ) noexcept
    {
        bool moreBytesLeft=false;
        uint64_t value=0;
        auto processedSize=StreamBase::unpackVarInt64(buf,availableSize,value,moreBytesLeft);
        if (processedSize<0||moreBytesLeft)
        {
            return -1;
        }
        field=StreamBase::zigZagDecode64(value);
        return processedSize;
    }

    /**
     * @brief Pack VarInt to buffer
     * @param buf Buffer to pack to
     * @param field Field to pack
     * @return Number of bytes occupied by VarInt or -1 or on failure
     */
    static inline int packVarInt(
        common::ByteArray* buf,
        const int64_t& field
    )
    {
        uint64_t value=StreamBase::zigZagEncode64(field);
        auto processedSize=StreamBase::packVarInt64(buf,value);
        if (processedSize<0)
        {
            return -1;
        }
        return processedSize;
    }
};

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITSTREAM_H
