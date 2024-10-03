/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file common/pmr/string.h
  *
  *  Defines pmr string with fixed preallocated buffer on stack.
  *
  */

/****************************************************************************/

#ifndef HATNPMRSTRING_H
#define HATNPMRSTRING_H

#include <hatn/common/common.h>

#include <hatn/common/pmr/pmrtypes.h>
#include <hatn/common/pmr/allocatorfactory.h>
#include <hatn/common/stdwrappers.h>

HATN_COMMON_NAMESPACE_BEGIN

namespace pmr
{

template <typename FactoryT=AllocatorFactory>
class ByteAllocator
{
    public:

        using value_type=char;

        ByteAllocator(
            common::pmr::memory_resource* resource=FactoryT::defaultDataMemoryResource()
            ) : m_resource(resource)
        {}

        ~ByteAllocator()=default;
        ByteAllocator(const ByteAllocator& other)=default;
        ByteAllocator& operator =(const ByteAllocator& other)=default;
        ByteAllocator(ByteAllocator&& other)=default;
        ByteAllocator& operator =(ByteAllocator&& other)=default;

        HATN_NODISCARD
        char* allocate(size_t n)
        {
            return static_cast<char*>(m_resource->allocate(n));
        }

        void deallocate(char* p, size_t n) noexcept
        {
            m_resource->deallocate(p, n);
        }

    private:

        common::pmr::memory_resource* m_resource;
};

constexpr size_t StringPreallocatedSize=64;

template <size_t PreallocatedSize=StringPreallocatedSize, typename AllocatorT=ByteAllocator<>>
struct StringT : public fmt::basic_memory_buffer<char,PreallocatedSize,AllocatorT>
{
    using BaseT=fmt::basic_memory_buffer<char,PreallocatedSize,AllocatorT>;

    StringT(
        const AllocatorT& alloc=AllocatorT{}
        )
        : BaseT(alloc)
    {}

    template <typename ContainerT>
    StringT(const ContainerT& container,
            const AllocatorT& alloc=AllocatorT{}
            )
        : BaseT(alloc)
    {
        this->append(container);
    }

    StringT(const char* str,
            const AllocatorT& alloc=AllocatorT{}
            )
        : StringT(
            lib::string_view{str},
            alloc)
    {}

    ~StringT()=default;
    StringT(StringT&& other)=default;
    StringT& operator=(StringT&& other)=default;

    StringT(const StringT& other) : StringT(other.get_allocator())
    {
        this->append(other);
    }

    StringT& operator=(const StringT& other)
    {
        if (&other!=this)
        {
            this->clear();
            this->append(other);
        }
        return *this;
    }

    lib::string_view toStringView() const noexcept
    {
        return lib::string_view{this->data(),this->size()};
    }

    std::string toString() const
    {
        return std::string{this->data(),this->size()};
    }

    operator lib::string_view() const noexcept
    {
        return toStringView();
    }

    operator std::string() const
    {
        return toString();
    }

    template <typename T>
    bool operator<(const T& other) const noexcept
    {
        return toStringView()<lib::string_view{other};
    }
    //! @todo Add <=> operator
};
using String=StringT<>;

} // namespace pmr

HATN_COMMON_NAMESPACE_END

#endif // HATNPMRSTRING_H