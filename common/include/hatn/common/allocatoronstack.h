/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/
/****************************************************************************/
/** @file common/allocatoronstack.h
 *
 *     Defines AllocatorOnStack.
 *
 */
/****************************************************************************/

#ifndef HATNALLOCATORONSTACK_H
#define HATNALLOCATORONSTACK_H

#include <stdexcept>
#include <type_traits>

#include <hatn/common/common.h>
#include <hatn/common/utils.h>

HATN_COMMON_NAMESPACE_BEGIN

/**
 * @brief The AllocatorOnStack class.
 *
 * Note that it can oly allocate/deallocate last data blocks thus appending/cutting blocks from the right.
 * For example, when using with std::vector only single reserve() must be called on the whole capacity
 * before vector use.
 */
template <typename T, size_t Size>
class AllocatorOnStack
{
    public:

        // constexpr static size_t Size=256;
        using value_type = T;

        AllocatorOnStack():m_occupied(0)
        {}

        ~AllocatorOnStack()=default;

        template<class U>
        struct rebind {
            using other = AllocatorOnStack<U, Size>;
        };

        template <class U>
        AllocatorOnStack(const AllocatorOnStack<U,Size>& other)
        {
            Assert(other.m_occupied==0,"Do not call copy constructor for AllocatorOnStack that is already in use");
        }

        // Move constructor is defined only in order to compile it.
        // You must never call it explicitly with allocator that is already in use.
        template <class U>
        AllocatorOnStack(AllocatorOnStack<U,Size>&& other)
        {
            Assert(other.m_occupied==0,"Do not call move constructor for AllocatorOnStack that is already in use");
        }

        template <class U>
        AllocatorOnStack& operator =(const AllocatorOnStack<U,Size>&)=delete;
        template <class U>
        AllocatorOnStack& operator =(AllocatorOnStack<U,Size>&&)=delete;

        //! Allocate data
        value_type* allocate(std::size_t n)
        {            
            if ((n+m_occupied)>Size)
            {
                throw std::bad_alloc();
            }
            value_type* p = reinterpret_cast<value_type*>(&m_buffer[m_occupied]);
            m_occupied+=n;
            return p;
        }

        //! Deallocate data
        void deallocate(value_type* p, std::size_t n)
        {
            auto ptr=reinterpret_cast<uintptr_t>(p);
            auto ptr1=reinterpret_cast<uintptr_t>(&m_buffer[m_occupied]);
            if ((ptr+n)!=ptr1)
            {
                throw std::runtime_error("Only last elements can be deallocated");
            }
            m_occupied-=n;
        }

    private:

        alignas(T) char m_buffer[sizeof(T)*Size];
        size_t m_occupied;
};

template <class T, class U, size_t Size>
bool operator==(const AllocatorOnStack<T,Size>& l, const AllocatorOnStack<U,Size>& r) noexcept
{
    return &l==&r;
}

template <class T, class U, size_t Size>
bool operator!=(const AllocatorOnStack<T,Size>& x, const AllocatorOnStack<U,Size>& y) noexcept
{
    return !(x == y);
}

HATN_COMMON_NAMESPACE_END

#endif // HATNALLOCATORONSTACK_H
