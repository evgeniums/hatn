/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/stream.сpp
  *
  *      Classes for serializing data to stream and deserializing data from stream
  *
  */

#include <hatn/dataunit/stream.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

namespace {

//---------------------------------------------------------------
template <typename T>
bool unpackVarIntByteImpl
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
}
} // anonymous namespace

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
        //! @todo collect errors
        // HATN_WARN(dataunit,"Unexpected end of buffer");
        return -1;
    }

    // check if more bytes left
    if (moreBytesLeft)
    {
        //! @todo collect errors
        // HATN_WARN(dataunit,"Unterminated VarInt");
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
        //! @todo handle errors
        // HATN_WARN(dataunit,"Unexpected end of buffer");
        return -1;
    }

    // check if more bytes left
    if (moreBytesLeft)
    {
        //! @todo handle errors
        // HATN_WARN(dataunit,"Unterminated VarInt");
        return -1;
    }

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
    }
}

//---------------------------------------------------------------
HATN_DATAUNIT_NAMESPACE_END
