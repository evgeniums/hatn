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

#include <boost/endian/conversion.hpp>

#include <hatn/dataunit/dataunit.h>

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

    template <typename BufT>
    static int packVarInt32(
        BufT* buf,
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

    template <typename BufT>
    static int packVarInt64(
        BufT* buf,
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
template <typename Type>
struct StreamUnsigned
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
    template <typename BufT>
    static int packVarInt(
        BufT* buf,
        const Type& field
    )
    {
        uint32_t value=static_cast<uint32_t>(field);
        auto processedSize=StreamBase::packVarInt32(buf,value);
        return processedSize;
    }
};

//! Serialize/deserialize signed integer types
template <typename Type>
struct StreamSigned
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
    template <typename BufT>
    static int packVarInt(
        BufT* buf,
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
template <typename Type>
struct Stream : public StreamUnsigned<Type>
{
};

template <>
struct Stream<uint32_t>
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
    template <typename BufT>
    static int packVarInt(
        BufT* buf,
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

template <>
struct Stream<int8_t> : public StreamSigned<int8_t>
{
};
template <>
struct Stream<int16_t> : public StreamSigned<int16_t>
{
};
template <>
struct Stream<int32_t> : public StreamSigned<int32_t>
{
};

template <>
struct Stream<uint64_t>
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
    template <typename BufT>
    static int packVarInt(
        BufT* buf,
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

template <>
struct Stream<int64_t>
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
    template <typename BufT>
    static int packVarInt(
        BufT* buf,
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

//---------------------------------------------------------------
template <typename BufT>
int StreamBase::packVarInt32(
        BufT* buf,
        const uint32_t& value
    )
{
    // prepare buffer
    auto accumulatedSize=buf->size();
    int processedSize=0;
    buf->resize(accumulatedSize+5);
    char* target=buf->data()+accumulatedSize;

    // fill buffer
    target[processedSize++]=static_cast<uint8_t>(value | 0x80);
    if (value >= (1 << 7))
    {
        target[processedSize++]=static_cast<uint8_t>((value >>  7) | 0x80);
        if (value >= (1 << 14))
        {
            target[processedSize++]=static_cast<uint8_t>((value >> 14) | 0x80);
            if (value >= (1 << 21))
            {
                target[processedSize++]=static_cast<uint8_t>((value >> 21) | 0x80);
                if (value >= (1 << 28))
                {
                    target[processedSize++]=static_cast<uint8_t>(value >> 28);
                }
                else
                {
                    target[3] &= 0x7F;
                }
            }
            else
            {
                target[2] &= 0x7F;
            }
        }
        else
        {
            target[1] &= 0x7F;
        }
    }
    else
    {
        target[0] &= 0x7F;
    }

    // resize buffer back
    if (processedSize!=5)
    {
        buf->resize(accumulatedSize+processedSize);
    }

    // return count of consumed bytes
    return processedSize;
}

//---------------------------------------------------------------
template <typename BufT>
int StreamBase::packVarInt64(
        BufT* buf,
        const uint64_t& value
    )
{
    int processedSize=0;
    char* target=nullptr;
    bool alreadyAppended=false;
    auto appendBytes=[&buf,&target,&processedSize,&alreadyAppended]()
    {
        if (!alreadyAppended)
        {
            alreadyAppended=true;
            auto initialSize=buf->size();
            buf->resize(initialSize+processedSize);
            target=buf->data()+initialSize;
        }
    };

    /************** Snippet from Google Protobuf ********************/

    uint32_t part0 = static_cast<uint32_t>(value      );
    uint32_t part1 = static_cast<uint32_t>(value >> 28);
    uint32_t part2 = static_cast<uint32_t>(value >> 56);

    int& size=processedSize;

    if (part2 == 0) {
        if (part1 == 0) {
            if (part0 < (1 << 14)) {
                if (part0 < (1 << 7)) {
                    size = 1; goto size1;
                } else {
                    size = 2; goto size2;
                }
            } else {
                if (part0 < (1 << 21)) {
                    size = 3; goto size3;
                } else {
                    size = 4; goto size4;
                }
            }
        } else {
            if (part1 < (1 << 14)) {
                if (part1 < (1 << 7)) {
                    size = 5; goto size5;
                } else {
                    size = 6; goto size6;
                }
            } else {
                if (part1 < (1 << 21)) {
                    size = 7; goto size7;
                } else {
                    size = 8; goto size8;
                }
            }
        }
    } else {
        if (part2 < (1 << 7)) {
            size = 9; goto size9;
        } else {
            size = 10; goto size10;
        }
    }

size10: appendBytes(); target[9] = static_cast<uint8_t>((part2 >>  7) | 0x80);
size9 : appendBytes(); target[8] = static_cast<uint8_t>((part2      ) | 0x80);
size8 : appendBytes(); target[7] = static_cast<uint8_t>((part1 >> 21) | 0x80);
size7 : appendBytes(); target[6] = static_cast<uint8_t>((part1 >> 14) | 0x80);
size6 : appendBytes(); target[5] = static_cast<uint8_t>((part1 >>  7) | 0x80);
size5 : appendBytes(); target[4] = static_cast<uint8_t>((part1      ) | 0x80);
size4 : appendBytes(); target[3] = static_cast<uint8_t>((part0 >> 21) | 0x80);
size3 : appendBytes(); target[2] = static_cast<uint8_t>((part0 >> 14) | 0x80);
size2 : appendBytes(); target[1] = static_cast<uint8_t>((part0 >>  7) | 0x80);
size1 : appendBytes(); target[0] = static_cast<uint8_t>((part0      ) | 0x80);

    target[size-1] &= 0x7F;

    // return count of consumed bytes
    return processedSize;
}

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITSTREAM_H
