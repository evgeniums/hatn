/*
    Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/bytearay.cpp
  *
  *     Byte array that uses memory pool for data buffers.
  *
  */

#include <iostream>

#include <hatn/common/pmr/withstaticallocator.h>
#include <hatn/common/pmr/withstaticallocator.ipp>
#define HATN_WITH_STATIC_ALLOCATOR_SRC
#include <hatn/common/bytearray.h>
#undef HATN_WITH_STATIC_ALLOCATOR_SRC

#include <hatn/common/utils.h>
#include <hatn/common/fileutils.h>

#include <hatn/common/pmr/allocatorfactory.h>

#include <hatn/common/thread.h>
#include <hatn/common/memorypool/poolcachegen.h>

#include <hatn/common/pmr/withstaticallocator.ipp>

HATN_COMMON_NAMESPACE_BEGIN

/********************** ByteArray **************************/

//---------------------------------------------------------------
ByteArray::ByteArray(
        pmr::memory_resource* resource
    ) noexcept : d(resource)
{
}

//---------------------------------------------------------------
ByteArray::ByteArray(
        const char *data
    )
{
    append(data);
}

//---------------------------------------------------------------
ByteArray::ByteArray(
        const char *data,
        size_t size,
        bool inlineRawBuffer,
        pmr::memory_resource* resource
    ) : d(resource)
{
    if (inlineRawBuffer)
    {
        d.buf=const_cast<char*>(data);
        d.size=size;
        d.capacity=size;
        d.offset=0;
        d.externalStorage=true;
    }
    else
    {
        load(data,size);
    }
}

//---------------------------------------------------------------
ByteArray::~ByteArray()
{
    reset();
}

//---------------------------------------------------------------
void ByteArray::load(
        const char *ptr,
        size_t size
    )
{
    d.externalStorage=false;
    clear();
    if (size>0)
    {
        resize(size);
        std::copy(ptr,ptr+size,data());
    }
}

//---------------------------------------------------------------
void ByteArray::resize(
        size_t size
    )
{
    if (size==d.size)
    {
        return;
    }
    if (size>d.size)
    {
        size_t diff=size-d.size;
        if (diff>capacityForAppend())
        {
            // in normal resize the space before offset is not used
            // so, new space is required even if there is enough capacity when taking into account the space before offset
            reserve(size);
        }
    }
    d.size=size;
}

//---------------------------------------------------------------
void ByteArray::rresize(
        size_t size
    )
{
    if (size==d.size)
    {
        return;
    }
    // in reverse resize either the space before offset or padding space will be used
    if (size>d.capacity)
    {
        // new space is required only when there is not ehough total capacity
        reserve(size,true,0,true);
    }
    else
    {
        if (size>d.size)
        {
            // new size is greater than the current value
            auto diff=size-d.size;
            if (diff<=d.offset)
            {
                // enough space before offset
                // just decrease offset, no moving is needed
                d.offset-=diff;
            }
            else
            {
                // not enough space before offset
                // need to shift data backward
                std::copy(d.buf+d.offset,d.buf+d.offset+d.size,d.buf+diff);
                d.offset=0;
            }
        }
        else
        {
            // new size is less than the current value, just increase offset without moving data
            d.offset+=d.size-size;
        }
    }
    d.size=size;
}

//---------------------------------------------------------------
void ByteArray::reserve(
        size_t capacity,
        bool reverse,
        size_t maxOffset,
        bool shiftRData,
        bool forceZeroOffset
    )
{
    if (d.externalStorage && capacity<=d.capacity)
    {
        return;
    }

    Assert(!d.externalStorage,"It is illegal to reserve space in raw data inline arrays, call reset() first");

    // new capacity can not be less than size of used space
    if (capacity<d.size)
    {
        return;
    }

    // append offset to new buffer size if it is not reversed reserving
    if (!reverse)
    {
        auto offset=d.offset;
        if ((forceZeroOffset || maxOffset!=0)&&offset>maxOffset)
        {
            offset=maxOffset;
        }
        capacity+=offset;
    }

    // check if the capacity is already as of requested
    if (capacity==d.capacity)
    {
        return;
    }

    // init vars
    char* newBuf=d.buf;
    auto newCapacity=d.capacity;
    bool noCopy=false;

    // process different sizes in different manner
    if (capacity<=PREALLOCATED_SIZE)
    {
        if (d.capacity<=PREALLOCATED_SIZE)
        {
            // ignore because we are still in the same preallocated buffer
            noCopy=true;
        }
        else
        {
            newCapacity=PREALLOCATED_SIZE;
            if (newCapacity==d.capacity)
            {
                noCopy=true;
            }
            else
            {
                newBuf=d.preallocated.data();
            }
        }
    }
    else
    {
        newCapacity=memBlockSize().keyForSize(capacity).to;
        if (newCapacity==d.capacity)
        {
            noCopy=true;
        }
        else
        {
            if (d.mResource==nullptr)
            {
                d.mResource=common::pmr::AllocatorFactory::getDefault()->dataMemoryResource();
            }
            newBuf=pmr::polymorphic_allocator<char>(d.mResource).allocate(newCapacity);
        }
    }

    // calc new offset
    size_t newOffset=d.offset;
    size_t shiftOffsetLeft=0;
    if (reverse)
    {
        // special processing for reverse order

        if (newCapacity>d.capacity)
        {
            // new extra capacity is added to offset
            newOffset=newCapacity-d.capacity+d.offset;
            if (shiftRData)
            {
                // this is for rresize
                shiftOffsetLeft=capacity-d.size;
                if (shiftOffsetLeft>newOffset)
                {
                    newOffset=shiftOffsetLeft;
                }
            }
        }
        else
        {
            // difference in capacity is subtracted from offset until it becomes zero
            auto diff=d.capacity-newCapacity;
            newOffset=(diff>=d.offset)?0:(d.offset-diff);
        }
    }
    else if (newCapacity<d.capacity)
    {
        // in direct order there is special case for decreasing buffer capacity
        // extra space is removed from the padding until there is no more space left in the padding

        auto diff=d.capacity-newCapacity;
        auto padding=d.capacity-d.size-d.offset;
        if (diff>padding)
        {
            // when padding is less than subtracted size then offset is a difference between capacity and size
            newOffset=newCapacity-d.size;
        }
    }
    if ((forceZeroOffset || maxOffset!=0)&&newOffset>maxOffset)
    {
        newOffset=maxOffset;
    }

    // copy old data to new buffer
    if (!noCopy)
    {
        if (d.size>0)
        {
            // move data to new buffer with new offset
            std::copy(d.buf+d.offset,d.buf+d.offset+d.size,newBuf+newOffset);
        }

        // free old buffers
        freeBuffer();
    }

    // update vars
    d.buf=newBuf;
    d.capacity=newCapacity;
    d.offset=newOffset-shiftOffsetLeft;
}

//---------------------------------------------------------------
ByteArray& ByteArray::append(
        const char *data,
        size_t size
    )
{
    size_t oldSize=d.size;
    resize(oldSize+size);
    std::copy(data,data+size,d.buf+oldSize+d.offset);
    return *this;
}

//---------------------------------------------------------------
ByteArray& ByteArray::prepend(
        const char *ptr,
        size_t size
    )
{
    size_t oldSize=d.size;
    rresize(oldSize+size);
    std::copy(ptr,ptr+size,data());
    return *this;
}

//---------------------------------------------------------------
const char* ByteArray::c_str() const
{
    if (isEmpty())
    {
        return nullptr;
    }
    if (isRawBuffer())
    {
        auto res=data();
        auto sz=size();
        if (sz<=capacity() && *(res+sz)=='\0')
        {
            return res;
        }
        throw std::overflow_error("Can not return c-string from inline raw buffer");
        return nullptr;
    }
    ByteArray* self=const_cast<ByteArray*>(this);

    if ((self->d.offset+self->d.size)==self->d.capacity)
    {
        // append char for null-termination symbol
        self->reserve(self->d.capacity+1);
    }
    self->d.buf[self->d.offset+self->d.size]='\0';
    return (self->data());
}

//---------------------------------------------------------------
static memorypool::MemBlockSize MemBlockSize;
memorypool::MemBlockSize& ByteArray::memBlockSize() noexcept
{
    return MemBlockSize;
}

//---------------------------------------------------------------
Error ByteArray::loadFromFile(const char *fileName)
{
    return FileUtils::loadFromFile(*this,fileName);
}

//---------------------------------------------------------------
Error ByteArray::saveToFile(const char *fileName) const noexcept
{
    return FileUtils::saveToFile(*this,fileName);
}

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
