/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/bytearray.h
  *
  *     Byte array that uses memory pool for data buffers.
  *
  */

/****************************************************************************/

#ifndef HATNBYTEARRAY_H
#define HATNBYTEARRAY_H

#include <iostream>
#include <map>
#include <functional>
#include <string>
#include <array>

#include <hatn/common/types.h>

#include <hatn/common/common.h>
#include <hatn/common/error.h>
#include <hatn/common/pmr/pmrtypes.h>
#include <hatn/common/pmr/withstaticallocator.h>

#include <hatn/common/sharedptr.h>
#include <hatn/common/managedobject.h>

HATN_COMMON_NAMESPACE_BEGIN

namespace memorypool {
class MemBlockSize;
}

 /**
 * @brief Byte array that uses memory pool for data buffers
 *
 *  Three kinds of data buffers can be used:
 *      1. Preallocated. If data size is less than preallocated size then preallocated buffer is used.
 *      2. Allocator. If data size is more than preallocated size then allocator is used.
 *                    ByteArray will reserve buffer with capacity that can be gretaer than requested size.
 *                    Capacity is calculated using static memBlockSize() object shared for all ByteArrays.
 *      3. Raw inline. Inline raw data buffer that points to external memory region and does not own the data.
 */
class HATN_COMMON_EXPORT ByteArray
{
    public:

        constexpr static const size_t PREALLOCATED_SIZE=20;
        using value_type=char;

        //! Default ctor
        ByteArray(
            pmr::memory_resource* resource=pmr::get_default_resource() //!< Memory resource for allocator
        ) noexcept;

        //! Ctor from null-terminated const char* string
        ByteArray(
            const char* data //!< Null-terminated char string
        );

        //! Ctor from data buffer
        ByteArray(
            const char* data, //!< Data buffer to copy data from
            size_t size //!< Data size
        ) : ByteArray(data,size,false,pmr::get_default_resource())
        {}

        //! Ctor from data buffer
        ByteArray(
            const char* data, //!< Data buffer to copy data from
            size_t size, //!< Data size
            pmr::memory_resource* resource
        ) : ByteArray(data,size,false,resource)
        {}

        //! Ctor inline in raw data buffer
        ByteArray(
            const char* data, //!< Data buffer
            size_t size, //!< Data size
            bool inlineRawBuffer //!< Use raw buffer inline without allocating data
        ) : ByteArray(data,size,inlineRawBuffer,pmr::get_default_resource())
        {}

        //! Ctor inline in raw data buffer
        ByteArray(
            const char* data, //!< Data buffer
            size_t size, //!< Data size
            bool inlineRawBuffer, //!< Use raw buffer inline without allocating data
            pmr::memory_resource* resource
        );

        /**
         * @brief Copy ctor
         */
        ByteArray(
            const ByteArray& other
        ) noexcept : d(other.d.mResource)
        {
            if (other.d.externalStorage)
            {
                d.externalStorage=other.d.externalStorage;
                d.capacity=other.d.capacity;
                d.size=other.d.size;
                d.offset=0;
                d.buf=other.d.buf;
            }
            else
            {
                copy(other);
            }
        }

        /**
         * @brief Move ctor
         */
        ByteArray(
            ByteArray&& other
        ) noexcept : d(std::move(other.d))
        {}

        //! Move assignment operator
        inline ByteArray& operator=(ByteArray&& other) noexcept
        {
            if(this != &other)
            {
                reset();
                d=std::move(other.d);
            }

            return *this;
        }

        //! Assignment operator
        inline ByteArray& operator=(const ByteArray& other)
        {
            if(this != &other)
            {
                reset();
                if (other.d.externalStorage)
                {
                    d.externalStorage=other.d.externalStorage;
                    d.capacity=other.d.capacity;
                    d.size=other.d.size;
                    d.offset=0;
                    d.buf=other.d.buf;
                }
                else
                {
                    d.mResource=other.d.mResource;
                    copy(other);
                }
            }
            return *this;
        }

        //! Assignment operator
        inline ByteArray& operator=(const char* data)
        {
            clear();
            // codechecker_false_positive [misc-unconventional-assign-operator] append() will return *this as required
            return append(data);
        }

        //! Assignment operator
        inline ByteArray& operator=(const std::string& data)
        {
            clear();
            // codechecker_false_positive [misc-unconventional-assign-operator] append() will return *this as required
            return append(data);
        }

        //! Dtor
        virtual ~ByteArray();

        //! Get data buffer
        inline char* data() const noexcept
        {
            return d.buf+d.offset;
        }

        //! Const data buffer
        inline char* data() noexcept
        {
            return d.buf+d.offset;
        }

        //! Const data buffer
        inline const char* constData() const noexcept
        {
            return d.buf+d.offset;
        }

        //! Buffer size
        inline size_t size() const noexcept
        {
            return d.size;
        }

        //! Total buffer capacity
        inline size_t capacity() const noexcept
        {
            return d.capacity;
        }

        /**
         * @brief Buffer capacity available for appending data
         * @return Capacity without offset
         */
        inline size_t capacityForAppend() const noexcept
        {
            return d.capacity-d.size-d.offset;
        }

        /**
         * @brief Get buffer offset
         * @return Size of reserved data in the beginning of the buffer
         *
         * Buffer offset shows size of reserved data in the beginning of the buffer that can be uses later,
         * e.g. to prepend headers or wrapper data to buffers
         *
         */
        inline size_t offset() const noexcept
        {
            return d.offset;
        }

        //! Load data inline
        void loadInline(
            char* data, //!< Data buffer
            size_t size //!< Data size
        ) noexcept
        {
            reset();
            d.externalStorage=true;
            d.buf=data;
            d.size=size;
            d.capacity=size;
        }

        //! Load data inline
        void loadInline(
            const char* data, //!< Data buffer
            size_t size //!< Data size
        ) noexcept
        {
            reset();
            d.externalStorage=true;
            d.buf=const_cast<char*>(data);
            d.size=size;
        }

        //! Load data
        void load(
            const char* data,
            size_t size
        );

        //! Load from null-terminated c-string
        inline void load(
            const char* data
        )
        {
            load(data,strlen(data));
        }

        //! Load from container
        template <typename ContainerT>
        void load(
            const ContainerT& other
        )
        {
            load(other.data(),other.size());
        }

        //! Load from ByteArray
        void load(
            const ByteArray& other
        )
        {
            if (this!=&other)
            {
                load(other.data(),other.size());
            }
        }

        //! Copy from other container
        template <typename ContainerT>
        void copy(
            const ContainerT& other
        )
        {
            load(other);
        }

        /** Clear array
         *  Clears content but doesn't deallocate buffer
         */
        inline void clear() noexcept
        {
            d.size=0;
        }

        /** Reset buffer
         *  Clear content and deallocate buffer
         */
        inline void reset() noexcept
        {
            d.reset();
        }

        /**
         * @brief Reserve data buffer
         * @param capacity Capacity to reserve. If reverse=false then actual reserved capacity will be capacity+offset()
         * @param reverse If true then space will be reserved at the beginning of the buffer and offset will be set
         * @param maxOffset Meaningful only when reverse=true. If maxOffset is not zero then the offset will be limited to maxOffset.
         * @param shiftRData Shift data right after reserving
         */
        void reserve(size_t capacity, bool reverse=false, size_t maxOffset=0, bool shiftRData=false, bool forceZeroOffset=false);

        /**
         * @brief Resize data buffer
         * @param size
         *
         * If new size is less than previous size then data will be truncated from the end.
         * Othervise the space is appended to the end.
         */
        void resize(size_t size);

        /**
         * @brief Reverse resize data buffer
         * @param size
         *
         * If new size is less than previous size then data will be truncated from the beginning.
         * Othervise the space is prepended to the beginning.
         */
        void rresize(size_t size);

        /**
         * @brief Resize to the right part of data buffer truncating from the beginning
         * @param size New size of the buffer, if size is greater than current size then the whole buffer will stay as is
         * @return Resulting buffer size
         *
         */
        size_t right(size_t size)
        {
            if (size>=d.size)
            {
                return d.size;
            }
            rresize(size);
            return d.size;
        }

        /**
         * @brief Resize to the left part of data buffer truncating from the end
         * @param size New size of the buffer, if size is greater than current size then the whole buffer will stay as is
         * @return Resulting buffer size
         *
         */
        size_t left(size_t size)
        {
            if (size>=d.size)
            {
                return d.size;
            }
            resize(size);
            return d.size;
        }

        //! Shrink capacity to used size
        inline void shrink()
        {
            reserve(d.size,false,0,0,true);
        }

        //! Append a char to buffer
        inline ByteArray& append(
            char data
        )
        {
            return append(&data,1);
        }

        //! Push back a char
        inline void push_back(
            char data
        )
        {
            append(data);
        }

        //! Append data to buffer
        ByteArray& append(
            const char* data,
            size_t size
        );

        //! Append null-terminated c-string to buffer
        inline ByteArray& append(
            const char* data
        )
        {
            return append(data,strlen(data));
        }

        //! Append container to buffer
        template <typename ContainerT>
        inline ByteArray& append(
            const ContainerT& other
        )
        {
            return append(other.data(),other.size());
        }

        //! Append other ByteArray to buffer
        inline ByteArray& append(
            const ByteArray& other
        )
        {
            if (this==&other)
            {
                size_t prevSize=size();
                resize(prevSize*2);
                std::copy(data(),data()+prevSize,data()+prevSize);
                return *this;
            }
            return append(other.data(),other.size());
        }

        //! Overload + operator with char*
        inline ByteArray operator+ (const char* data) const
        {
            return ByteArray(*this).append(data);
        }

        //! Overload + operator with char*
        inline ByteArray operator+ (char data) const
        {
            return ByteArray(*this).append(data);
        }

        //! Overload + operator with other container
        template <typename ContainerT>
        inline ByteArray operator+ (const ContainerT& other) const
        {
            return ByteArray(*this).append(other);
        }

        //! Overload += operator with char
        inline ByteArray& operator += (char data)
        {
            return append(data);
        }

        //! Overload += operator with char
        inline ByteArray& operator += (const char* data)
        {
            return append(data);
        }

        //! Overload += operator with std string
        template <typename ContainerT>
        inline ByteArray&  operator += (const ContainerT& other)
        {
            return append(other);
        }

        //! Overload == operator with char*
        inline bool operator == (const char* ptr) const noexcept
        {
            bool ok=d.size==strlen(ptr)&&(memcmp(constData(),ptr,d.size)==0);
            return ok;
        }

        //! Overload != operator with char*
        inline bool operator != (const char* data) const noexcept
        {
            return !(*this==data);
        }

        //! Overload == operator
        template <typename T> inline bool operator == (const T& other) const noexcept
        {
            return isEqual(other.data(),other.size());
        }

        //! Overload != operator
        template <typename T> inline bool operator != (const T& other) const noexcept
        {
            return !isEqual(other.data(),other.size());
        }

        //! Check if content is less than some data
        inline bool isLess(const char* ptr, size_t length) const noexcept
        {
            if (size()==length)
            {
                return memcmp(data(),ptr,length)<0;
            }
            return size()<length;
        }

        //! Prepend a char to buffer
        inline ByteArray& prepend(
            char data
        )
        {
            return prepend(&data,1);
        }

        //! Push front a char
        inline void push_front(
            char data
        )
        {
            prepend(data);
        }

        //! Prepend data to buffer
        ByteArray& prepend(
            const char* data,
            size_t size
        );

        //! Prepend null-terminated c-string to buffer
        inline ByteArray& prepend(
            const char* data
        )
        {
            prepend(data,strlen(data));
            return *this;
        }

        //! Append other buffer wrapper
        template <typename T> inline ByteArray& prepend(const T& other)
        {
            return prepend(other.data(),other.size());
        }

        //! Get string view
        inline lib::string_view stringView() const noexcept
        {
            return lib::string_view(data(),size());
        }

        //! Get string view of array's part
        inline lib::string_view stringView(size_t offset, size_t length=0) const noexcept
        {
            if (size()<(offset+length))
            {
                return lib::string_view();
            }

            length=(length==0)?(size()-offset):length;
            return lib::string_view(data()+offset,length);
        }

        //! Check if content is equal to some data
        template <typename DataT>
        inline bool isEqual(DataT ptr, size_t size) const noexcept
        {
            return d.size==size&&(memcmp(constData(),ptr,d.size)==0);
        }

        template <typename ContainerT>
        inline bool isEqual(const ContainerT& other) const noexcept
        {
            return isEqual(other.data(),other.size());
        }

        inline bool isEqual(const char* other) const noexcept
        {
            return isEqual(ConstDataBuf(other));
        }

        //! Check if array is empty
        inline bool isEmpty() const noexcept
        {
            return d.size==0;
        }

        inline bool empty() const noexcept
        {
            return isEmpty();
        }

        /**
         * @brief Get null-terminated C-string representation
         * @return Null-terminated C-string
         *
         * Can increase capacity if size()==capacity()
         *
         */
        const char* c_str() const;

        //! Create array from raw data inline without allocating memory
        inline static ByteArray fromRawData(
            const char* data,
            size_t size
        ) noexcept
        {
            ByteArray arr(data,size,true);
            return arr;
        }

        //! Check if it is a inline raw data buffer
        inline bool isRawBuffer() const noexcept
        {
            return d.externalStorage;
        }

        //! Create std::string
        inline std::string toStdString() const
        {
            return std::string(data(),size());
        }

        //! Overload operator []
        inline const char& operator[] (std::size_t index) const
        {
            return at(index);
        }

        //! Overload operator []
        inline char& operator[] (std::size_t index)
        {
            return at(index);
        }

        //! Get char by index
        inline const char& at(std::size_t index) const
        {
            if (index>size())
            {
                throw std::out_of_range("Index exceeds the size of ByteArray");
            }
            return d.buf[index+d.offset];
        }

        //! Get char by index
        inline char& at(std::size_t index)
        {
            if (index>size())
            {
                throw std::out_of_range("Index exceeds the size of ByteArray");
            }
            return d.buf[index+d.offset];
        }

        //! Set allocator's memory resource
        inline void setAllocatorResource(
            pmr::memory_resource* resource
        )
        {
            Assert(size()<=PREALLOCATED_SIZE,"New allocator's resource can not be set in ByteArray when other allocator is already used");
            d.mResource=resource;
        }

        //! Get allocator
        inline pmr::memory_resource* allocatorResource() const noexcept
        {
            return d.mResource;
        }

        static memorypool::MemBlockSize& memBlockSize() noexcept;

        /**
         * @brief Load byte array from file
         * @param fileName Filename, must be null-terminated
         * @return Operation status
         */
        Error loadFromFile(
            const char* fileName
        );

        /**
         * @brief Load byte array from file
         * @param fileName Filename, must be null-terminated
         * @return Operation status
         */
        inline Error loadFromFile(
            const std::string& fileName
        )
        {
            return loadFromFile(fileName.c_str());
        }

        /**
         * @brief Save byte array to file
         * @param fileName Filename, must be null-terminated
         * @return Operation status
         */
        Error saveToFile(
            const char* fileName
        ) const noexcept;

        /**
         * @brief Save byte array to file
         * @param fileName Filename
         * @return Operation status
         */
        inline Error saveToFile(
            const std::string& fileName
        ) const noexcept
        {
            return saveToFile(fileName.c_str());
        }

        //! Fill container with char
        inline void fill(const char ch) noexcept
        {
            memset(data(),static_cast<int>(ch),size());
        }

        //! Fill container with char
        inline void fill(const char ch, size_t offset) noexcept
        {
            if (offset<size())
            {
                memset(data()+offset,static_cast<int>(ch),size()-offset);
            }
        }

    private:

        struct BufCtrl final
        {
            std::array<char,PREALLOCATED_SIZE> preallocated;
            size_t size;
            size_t capacity;
            char* buf;
            bool externalStorage;            
            pmr::memory_resource* mResource;
            size_t offset;

            BufCtrl(const BufCtrl& other)=delete;
            BufCtrl& operator =(const BufCtrl& other)=delete;
            ~BufCtrl()=default;

            BufCtrl(BufCtrl&& other) noexcept
                : size(other.size),
                  capacity(other.capacity),
                  buf(preallocated.data()),
                  externalStorage(other.externalStorage),
                  mResource(other.mResource),
                  offset(other.offset)
            {
                if (other.capacity>PREALLOCATED_SIZE || other.externalStorage)
                {
                    buf=other.buf;
                }
                else if (other.size>0)
                {
                    std::copy(other.preallocated.begin(),other.preallocated.begin()+other.size,preallocated.begin());
                }
                move(std::move(other));
            }

            inline BufCtrl& operator =(BufCtrl&& other) noexcept
            {
                if (this!=&other)
                {
                    reset();

                    if (other.capacity>PREALLOCATED_SIZE || other.externalStorage)
                    {
                        buf=other.buf;
                    }
                    else if (other.size>0)
                    {
                        std::copy(other.preallocated.begin(),other.preallocated.begin()+other.size,preallocated.begin());
                    }

                    size=other.size;
                    capacity=other.capacity;
                    externalStorage=other.externalStorage;
                    mResource=other.mResource;
                    offset=other.offset;

                    move(std::move(other));
                }

                return *this;
            }

            inline void move(BufCtrl&& other) noexcept
            {
                other.buf=other.preallocated.data();
                other.size=0;
                other.offset=0;
                other.capacity=PREALLOCATED_SIZE;
                other.externalStorage=false;
            }

            BufCtrl(pmr::memory_resource* resource) noexcept
                : size(0),capacity(PREALLOCATED_SIZE),buf(preallocated.data()),externalStorage(false),
                  mResource(resource),offset(0)
            {
            }

            BufCtrl() noexcept
                : size(0),capacity(PREALLOCATED_SIZE),buf(preallocated.data()),externalStorage(false),
                  mResource(pmr::get_default_resource()),offset(0)
            {
            }

            inline void reset() noexcept
            {
                if (externalStorage)
                {
                    externalStorage=false;
                }
                else
                {
                    freeBuffer();
                }
                buf=preallocated.data();
                size=0;
                offset=0;
                capacity=PREALLOCATED_SIZE;
            }
            inline void freeBuffer() noexcept
            {
                if (externalStorage)
                {
                    return;
                }

                if (capacity>PREALLOCATED_SIZE)
                {
                    pmr::polymorphic_allocator<char>(mResource).deallocate(buf,capacity);
                }
                else
                {
                    // content is in preallocated buffer
                }
            }

        };

        BufCtrl d;

        inline void freeBuffer() noexcept
        {
            d.freeBuffer();
        }
};

HATN_WITH_STATIC_ALLOCATOR_DECLARE(ByteArrayManaged,HATN_COMMON_EXPORT)

/**
 * @brief Managed version of ByteArray
 */
class HATN_COMMON_EXPORT ByteArrayManaged : public ManagedWrapper<ByteArray>,
                                                public WithStaticAllocator<ByteArrayManaged>
{
    using ManagedWrapper<ByteArray>::ManagedWrapper;
};
using ByteArrayShared=SharedPtr<ByteArrayManaged>;

//---------------------------------------------------------------
    HATN_COMMON_NAMESPACE_END
#endif // HATNBYTEARRAY_H
