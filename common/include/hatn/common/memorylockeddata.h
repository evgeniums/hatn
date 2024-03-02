/*
    Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    
*/

/****************************************************************************/
/** @file common/memorylockeddata.h
 *
 *     Types for secure data containers locked in memory.
 *
 */
/****************************************************************************/

#ifndef HATNMEMORYLOCKEDDATA_H
#define HATNMEMORYLOCKEDDATA_H

#define __STDC_WANT_LIB_EXT1__ 1
#include <cstring>

#include <sstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <mutex>

#include <hatn/common/logger.h>
#include <hatn/common/error.h>

DECLARE_LOG_MODULE_EXPORT(mlockdata,HATN_COMMON_EXPORT)

HATN_COMMON_NAMESPACE_BEGIN

/**
 * Controller for locking data in memory.
 * Can be used to lock/unlock the same page multiple times.
 * MemoryLocker counts lock/unlock requests.
 *
 * Uses mlock/munlock on unix/linux systems and VirtualLock/VirtualUnlock on Windows.
 */
class HATN_COMMON_EXPORT MemoryLocker
{
    public:

        /**
         * Lock region [p; p+n) in RAM.
         *
         * Throws an exception if failed.
         */
        static void lockRegion(void *p, size_t n);

        /**
         * Unlock region [p; p+n) in RAM.
         */
        static void unlockRegion(void *p, size_t n);

    private:

        static void doLockRegion(void *p, size_t n);
        static void doUnlockRegion(void *p, size_t n);

        static const size_t pageSize;

        typedef size_t pagenum_t;
        static pagenum_t addr2pagenum(void *p);
        static void* pagenumFirstByte(pagenum_t page);

        static std::map<pagenum_t, unsigned long> lockedCounter;
        static std::mutex mutex;
};


/**
 * MemoryLockedDataAllocator manages buffers locked in memory.
 * Those buffers can not be swapped to permanent storage and are zeroed afte deallocation.
 */
template<typename T>
class MemoryLockedDataAllocator
{
    public:

        using value_type = T;

        //! Ctor
        MemoryLockedDataAllocator()=default;

        //! Copy ctor
        template <class U> MemoryLockedDataAllocator(const MemoryLockedDataAllocator<U>&) noexcept
        {}

        //! Allocate data
        value_type* allocate(std::size_t n)
        {
            value_type* p = static_cast<value_type*>(::operator new(n*sizeof(value_type)));
            HATN_DEBUG_LVL(mlockdata, 10,"+ " << reinterpret_cast<void*>(p) << "-" << reinterpret_cast<void*>(p+n));
            try
            {
                MemoryLocker::lockRegion(p, sizeof(value_type)*n);
            }
            catch (...)
            {
                ::operator delete(p);
                throw;
            }
            return p;
        }

        //! Deallocate data
        void deallocate(value_type* p, std::size_t n) noexcept
        {
            HATN_DEBUG_LVL(mlockdata, 10,"- " << reinterpret_cast<void*>(p) << "-" << reinterpret_cast<void*>(p+n));
            auto&& count=sizeof(value_type) * n;
#ifdef __STDC_LIB_EXT1__
            memset_s(p, count, 0, count);
#else
            memset(p, 0, count);
#endif
            MemoryLocker::unlockRegion(p, sizeof(value_type)*n);
            ::operator delete(p);
        }
};

template <class T, class U>
bool operator==(const MemoryLockedDataAllocator<T>&, const MemoryLockedDataAllocator<U>&) noexcept
{
    return true;
}

template <class T, class U>
bool operator!=(const MemoryLockedDataAllocator<T>& x, const MemoryLockedDataAllocator<U>& y) noexcept
{
    return !(x == y);
}

/**
 * @brief MemoryLockedDataString
 *
 * String locked in memory
 */
typedef std::basic_string<char,
    std::char_traits<char>,
    MemoryLockedDataAllocator<char>> MemoryLockedDataString;
/**
 * @brief MemoryLockedDataStringStream
 *
 * Sring stream locked in memory
 */
typedef std::basic_stringstream<char,
     std::char_traits<char>,
     MemoryLockedDataAllocator<char>> MemoryLockedDataStringStream;

/**
 * @brief MemoryLockedDataVector
 *
 * Vector of chars locked in memory
 */
typedef std::vector<char,
     MemoryLockedDataAllocator<char>> MemoryLockedDataVector;

/**
 * @brief The MemoryLockedArray class that has some interfaces of ByteArray and uses MemoryLockedDataVector as container
 */
class HATN_COMMON_EXPORT MemoryLockedArray
{
    public:

        using value_type=char;

        //! Default ctor
        MemoryLockedArray()=default;

        ~MemoryLockedArray()=default;

        //! Ctor from data buffer
        MemoryLockedArray(
            const char* data, //!< Data buffer to copy data from
            size_t size //!< Data size
        ) : d(data,data+size)
        {
        }

        //! Ctor from null-terminated const char* string
        MemoryLockedArray(
            const char* data //!< Null-terminated char string
        )
        {
            size_t size=strlen(data);
            load(data,size);
        }

        /**
         * @brief Copy ctor
         */
        MemoryLockedArray(
            const MemoryLockedArray& other
        ) noexcept
        {
            copy(other);
        }

        /**
         * @brief Move ctor
         */
        MemoryLockedArray(
            MemoryLockedArray&& other
        ) noexcept : d(std::move(other.d))
        {}

        //! Copy assignment operator
        MemoryLockedArray& operator=(const MemoryLockedArray& other)
        {
            if (this!=&other)
            {
                copy(other);
            }
            return *this;
        }
        //! Move assignment operator
        MemoryLockedArray& operator=(MemoryLockedArray&& other) noexcept
        {
            if (this!=&other)
            {
                d=std::move(other.d);
            }
            return *this;
        }

        //! Copy from other container
        template <typename ContainerT>
        void copy(const ContainerT& other)
        {
            load(other.data(),other.size());
        }
        //! Load from other container
        template <typename ContainerT>
        void load(const ContainerT& other)
        {
            load(other.data(),other.size());
        }
        //! Load from buffer
        void load(
                const char* data,
                size_t size
            )
        {
            d.resize(size);
            std::copy(data,data+size,d.data());
        }
        //! Load from buffer
        void load(const char* data)
        {
            load(data,strlen(data));
        }

        //! Get pointer to underlying data
        inline char* data() noexcept
        {
            return d.data();
        }
        //! Get pointer to underlying data
        inline const char* data() const noexcept
        {
            return d.data();
        }
        //! Get const pointer to underlying data
        inline const char* constData() const noexcept
        {
            return d.data();
        }

        //! Get size of array
        inline size_t size() const noexcept
        {
            return d.size();
        }

        //! Get pointer to null-terminated c-string
        inline const char* c_str() const
        {
            auto* self=const_cast<MemoryLockedArray*>(this);
            if (d.capacity()==d.size())
            {
                self->reserve(size()+1);
            }
            *(self->data()+size())='\0';
            return d.data();
        }

        //! Get container capacity
        inline size_t capacity() const noexcept
        {
            return d.capacity();
        }

        //! Resize array
        inline void resize(size_t size)
        {
            d.resize(size);
        }

        //! Reserver sace in container
        inline void reserve(size_t size)
        {
            d.reserve(size);
        }

        //! Shring container capacity to fit used data volume
        inline void shrink()
        {
            d.shrink_to_fit();
        }

        //! Overload operator []
        inline const char& operator[] (std::size_t index) const
        {
            return d[index];
        }

        //! Overload operator []
        inline char& operator[] (std::size_t index)
        {
            return d[index];
        }

        //! Get char by index
        inline const char& at(std::size_t index) const
        {
            return d.at(index);
        }

        //! Get char by index
        inline char& at(std::size_t index)
        {
            return d.at(index);
        }

        //! Get string view
        inline lib::string_view stringView() const noexcept
        {
            return lib::string_view(data(),size());
        }

        //! Check if content is equal to some data
        inline bool isEqual(const char* ptr, size_t size) const noexcept
        {
            return d.size()==size&&(memcmp(constData(),ptr,size)==0);
        }

        //! Check if array is empty
        inline bool isEmpty() const noexcept
        {
            return size()==0;
        }

        inline bool empty() const noexcept
        {
            return isEmpty();
        }

        //! Push back a char
        inline void push_back(
            char data
        )
        {
            d.push_back(data);
        }

        //! Append a char
        inline void append(
            char data
        )
        {
            d.push_back(data);
        }

        //! Append c-string
        inline void append(
            const char* data
        )
        {
            append(data,strlen(data));
        }

        //! Append data
        inline void append(
            const char* data,
            size_t size
        )
        {
            size_t prevSize=d.size();
            d.resize(d.size()+size);
            std::copy(data,data+size,d.data()+prevSize);
        }

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
         * @param fileName Filename, must be null-terminated
         * @return Operation status
         */
        inline Error saveToFile(
            const std::string& fileName
        ) const noexcept
        {
            return saveToFile(fileName.c_str());
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

        //! Clear container
        inline void clear() noexcept
        {
            d.clear();
            shrink();
        }

        //! Fill container with char
        inline void fill(const char ch) noexcept
        {
            memset(d.data(),static_cast<int>(ch),d.size());
        }

        //! Fill container with char
        inline void fill(const char ch, size_t offset) noexcept
        {
            memset(data()+offset,static_cast<int>(ch),size()-offset);
        }

    private:

        MemoryLockedDataVector d;
};

HATN_COMMON_NAMESPACE_END

#endif // HATNMEMORYLOCKEDDATA_H
