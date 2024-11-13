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

#include <vector>
#include <string>
#include <iostream>

#include <hatn/common/pmr/pmrtypes.h>
#include <hatn/thirdparty/shortallocator/short_alloc.h>

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

template <class T,
     std::size_t N,
     std::size_t Align = alignof(std::max_align_t),
     typename AllocatorT=std::allocator<T>>
using AllocatorOnStack=salloc::short_alloc<T,N*Align,Align,AllocatorT>;

constexpr size_t DefaultPreallocatedStringSize=64;

template <size_t PreallocatedSize=DefaultPreallocatedStringSize, typename FallbackAllocatorT=std::allocator<char>>
using PreallocatedStringT = std::basic_string<char,std::char_traits<char>,
                                              AllocatorOnStack<char,PreallocatedSize,alignof(std::max_align_t),FallbackAllocatorT>
                                              >;

template <size_t PreallocatedSize, typename FallbackAllocatorT>
class ArenaWrapperT
{
    public:

        using AllocaT=AllocatorOnStack<char,PreallocatedSize,alignof(std::max_align_t),FallbackAllocatorT>;

    protected:

        typename AllocaT::arena_type m_arena;
};

template <size_t PreallocatedSize=DefaultPreallocatedStringSize, typename FallbackAllocatorT=std::allocator<char>>
class StringOnStackT : public ArenaWrapperT<PreallocatedSize,FallbackAllocatorT>,
                       public PreallocatedStringT<PreallocatedSize,FallbackAllocatorT>
{
    public:

        using ArenaHolderT=ArenaWrapperT<PreallocatedSize,FallbackAllocatorT>;
        using BaseT=PreallocatedStringT<PreallocatedSize,FallbackAllocatorT>;
        using AllocaT=typename BaseT::allocator_type;

        template <typename ...Args>
        StringOnStackT(Args&&... args) : ArenaHolderT(),
                                         BaseT(std::forward<Args>(args)...,AllocaT{this->m_arena})
        {
        }

        StringOnStackT() : ArenaHolderT(),
                           BaseT(AllocaT{this->m_arena})
        {}

        ~StringOnStackT()
        {
            this->clear();
            this->shrink_to_fit();
        }

        StringOnStackT(const StringOnStackT& other) : ArenaHolderT(),
                                                      BaseT(AllocaT{this->m_arena})
        {
            this->append(other);
        }

        StringOnStackT(StringOnStackT&& other) : ArenaHolderT(),
                                                 BaseT(AllocaT{this->m_arena})
        {
            this->append(other);
            other.clear();
        }

        StringOnStackT& operator =(const StringOnStackT& other)
        {
            if (this==&other)
            {
                return *this;
            }

            this->clear();
            this->append(other);
            return *this;
        }

        StringOnStackT& operator =(StringOnStackT&& other)
        {
            if (this==&other)
            {
                return *this;
            }

            this->clear();
            this->append(other);
            other.clear();
            return *this;
        }
};

using StringOnStack=StringOnStackT<>;

namespace pmr
{
using StringOnStack=StringOnStackT<DefaultPreallocatedStringSize,polymorphic_allocator<char>>;
}


constexpr size_t DefaultPreallocatedVectorSize=8;

template <typename T, size_t PreallocatedSize=DefaultPreallocatedVectorSize, typename FallbackAllocatorT=std::allocator<T>>
using PreallocatedVectorT = std::vector<T,AllocatorOnStack<T,PreallocatedSize,alignof(std::max_align_t),FallbackAllocatorT>>;

template <typename T, size_t PreallocatedSize=DefaultPreallocatedVectorSize, typename FallbackAllocatorT=std::allocator<T>>
class VectorOnStackT : public ArenaWrapperT<PreallocatedSize,FallbackAllocatorT>,
                       public PreallocatedVectorT<T,PreallocatedSize,FallbackAllocatorT>
{
public:

    using ArenaHolderT=ArenaWrapperT<PreallocatedSize,FallbackAllocatorT>;
    using BaseT=PreallocatedVectorT<T,PreallocatedSize,FallbackAllocatorT>;
    using AllocaT=typename BaseT::allocator_type;

    template <typename ...Args>
    VectorOnStackT(Args&&... args) : ArenaHolderT(),
        BaseT(std::forward<Args>(args)...,AllocaT{this->m_arena})
    {
    }

    VectorOnStackT(std::initializer_list<T> elements) : ArenaHolderT(),
        BaseT(std::move(elements),AllocaT{this->m_arena})
    {
    }

    VectorOnStackT() : ArenaHolderT(),
        BaseT(AllocaT{this->m_arena})
    {}

    ~VectorOnStackT()
    {
        this->clear();
        this->shrink_to_fit();
    }

    VectorOnStackT(const VectorOnStackT& other) : ArenaHolderT(),
        BaseT(AllocaT{this->m_arena})
    {
        this->append(other);
    }

    VectorOnStackT(VectorOnStackT&& other) : ArenaHolderT(),
        BaseT(AllocaT{this->m_arena})
    {
        this->append(other);
        other.clear();
    }

    VectorOnStackT& operator =(const VectorOnStackT& other)
    {
        if (this==&other)
        {
            return *this;
        }

        this->clear();
        this->append(other);
        return *this;
    }

    VectorOnStackT& operator =(VectorOnStackT&& other)
    {
        if (this==&other)
        {
            return *this;
        }

        this->clear();
        this->append(other);
        other.clear();
        return *this;
    }

    template <typename T1>
    void append(const T1& other)
    {
        this->insert(std::end(*this), std::begin(other), std::end(other));
    }
};

template <typename T, size_t PreallocatedSize=DefaultPreallocatedVectorSize>
using VectorOnStack=VectorOnStackT<T, PreallocatedSize>;

namespace pmr
{
template <typename T, size_t PreallocatedSize=DefaultPreallocatedVectorSize>
using VectorOnStack=VectorOnStackT<T,PreallocatedSize,polymorphic_allocator<char>>;
}



HATN_COMMON_NAMESPACE_END

#endif // HATNALLOCATORONSTACK_H
