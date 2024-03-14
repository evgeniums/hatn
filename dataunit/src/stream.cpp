/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** \file dataunit/stream.сpp
  *
  *      Classes for serializing data to stream and deserializing data from stream
  *
  */

#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/stream.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

    namespace {
//---------------------------------------------------------------
template <typename T> bool unpackVarIntByteImpl
    (
        const char *buf,
        T& result,
        size_t availableSize,
        int &processedSize,
        bool &overflow,
        bool& moreBytesLeft,
        uint32_t &shift,
        uint32_t maxShift
    ) noexcept
{
    T b=0;
    if (static_cast<int>(availableSize)<processedSize)
    {
        overflow=true;
        return false;
    }
    b = static_cast<uint8_t>(*(buf+processedSize));
    moreBytesLeft=(b & 0x80) != 0;
    b &= 0x7F;
    result  |= b << shift;
    ++processedSize;
    if (!moreBytesLeft)
    {
        return false;
    }
    if (shift>maxShift)
    {
        return false;
    }
    shift+=7;
    return true;
HATN_DATAUNIT_NAMESPACE_END

//---------------------------------------------------------------
bool StreamBase::unpackVarIntByte(
        const char *buf,
        uint32_t& result,
		size_t availableSize,
		int&processedSize,
        bool &overflow,
        bool& moreBytesLeft,
        uint32_t &shift,
        uint32_t maxShift
    ) noexcept
{
    return unpackVarIntByteImpl(buf,result,availableSize,processedSize,overflow,moreBytesLeft,shift,maxShift);
}

//---------------------------------------------------------------
int StreamBase::unpackVarInt32(
        const char *buf,
		size_t availableSize,
        uint32_t& value,
        bool& moreBytesLeft
    ) noexcept
{
    value=0;
    bool overflow=false;
    uint32_t shift=0;
    int processedSize=0;

    // iterate bytes
    while (unpackVarIntByteImpl<uint32_t>(buf,value,availableSize,processedSize,overflow,moreBytesLeft,shift,28)){}

    // check for overflow
    if (overflow)
    {
        HATN_WARN(dataunit,"Unexpected end of buffer");
        return -1;
    }

    // check if more bytes left
    if (moreBytesLeft)
    {
        HATN_WARN(dataunit,"Unterminated VarInt");
        return -1;
    }

    // return count of consumed bytes
    return processedSize;
}

//---------------------------------------------------------------
int StreamBase::unpackVarInt64(
        const char *buf,
		size_t availableSize,
        uint64_t& value,
        bool& moreBytesLeft
    ) noexcept
{
    value=0;
    bool overflow=false;
    uint32_t shift=0;
    int processedSize=0;

    while (unpackVarIntByteImpl<uint64_t>(buf,value,availableSize,processedSize,overflow,moreBytesLeft,shift,56)){}

    // check for overflow
    if (overflow)
    {
        HATN_WARN(dataunit,"Unexpected end of buffer");
        return -1;
    }

    // check if more bytes left
    if (moreBytesLeft)
    {
        HATN_WARN(dataunit,"Unterminated VarInt");
        return -1;
    }

    // return count of consumed bytes
    return processedSize;
}

//---------------------------------------------------------------
int StreamBase::packVarInt32(
        common::ByteArray* buf,
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
int StreamBase::packVarInt64(
        common::ByteArray* buf,
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

//---------------------------------------------------------------
void StreamBase::packVarInt32(
        char* buf,
        const uint32_t& value
    )
{
    char* target=buf;

    // fill buffer
    target[0]=static_cast<uint8_t>(value | 0x80);
    if (value >= (1 << 7))
    {
        target[1]=static_cast<uint8_t>((value >>  7) | 0x80);
        if (value >= (1 << 14))
        {
            target[2]=static_cast<uint8_t>((value >> 14) | 0x80);
            if (value >= (1 << 21))
            {
                target[3]=static_cast<uint8_t>((value >> 21) | 0x80);
                if (value >= (1 << 28))
                {
                    target[4]=static_cast<uint8_t>(value >> 28);
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
    HATN_DATAUNIT_NAMESPACE_END

//---------------------------------------------------------------
HATN_DATAUNIT_NAMESPACE_END
