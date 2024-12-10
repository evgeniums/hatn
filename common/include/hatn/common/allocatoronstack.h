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

#include <hatn/thirdparty/shortallocator/short_alloc.h>

#include <hatn/common/common.h>

#include <hatn/common/pmr/pmrtypes.h>
#include <hatn/common/stdwrappers.h>
#include <hatn/common/flatmap.h>

HATN_COMMON_NAMESPACE_BEGIN

/********************** AllocatorOnStack **************************/

template <class T,
     std::size_t N,
     std::size_t Align = alignof(std::max_align_t),
     typename AllocatorT=std::allocator<T>>
using AllocatorOnStack=salloc::short_alloc<T,N*Align,Align,AllocatorT>;

/********************** StringOnStack **************************/

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

struct StringOnStackTag
{};

template <size_t PreallocatedSize=DefaultPreallocatedStringSize, typename FallbackAllocatorT=std::allocator<char>>
class StringOnStackT : public ArenaWrapperT<PreallocatedSize,FallbackAllocatorT>,
                       public PreallocatedStringT<PreallocatedSize,FallbackAllocatorT>
{
    public:

        using hana_tag=StringOnStackTag;

        using ArenaHolderT=ArenaWrapperT<PreallocatedSize,FallbackAllocatorT>;
        using BaseT=PreallocatedStringT<PreallocatedSize,FallbackAllocatorT>;
        using AllocaT=typename BaseT::allocator_type;

        template <typename ...Args>
        StringOnStackT(Args&&... args) : ArenaHolderT(),
                                         BaseT(std::forward<Args>(args)...,AllocaT{this->m_arena})
        {
            this->reserve(PreallocatedSize);
        }

        StringOnStackT() : ArenaHolderT(),
                           BaseT(AllocaT{this->m_arena})
        {
            this->reserve(PreallocatedSize);
        }

        ~StringOnStackT()
        {
            this->clear();
            this->shrink_to_fit();
        }

        StringOnStackT(const StringOnStackT& other) : ArenaHolderT(),
                                                      BaseT(AllocaT{this->m_arena})
        {
#ifdef HATN_CHECK_STRING_STACK_CTORS
            std::cout<<"Copy constructor StringOnStackT"<<std::endl;
#endif
            this->reserve(PreallocatedSize);
            this->append(other);
        }

        StringOnStackT(StringOnStackT&& other) : ArenaHolderT(),
                                                 BaseT(AllocaT{this->m_arena})
        {
#ifdef HATN_CHECK_STRING_STACK_CTORS
            std::cout<<"Move constructor StringOnStackT"<<std::endl;
#endif
            this->reserve(PreallocatedSize);
            this->append(other);
            other.clear();
        }

        StringOnStackT& operator =(const StringOnStackT& other)
        {
            if (this==&other)
            {
                return *this;
            }
#ifdef HATN_CHECK_STRING_STACK_CTORS
            std::cout<<"Copy assignment StringOnStackT"<<std::endl;
#endif
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

        using BaseT::append;

        template <typename ContiguousRange>
        void append(const ContiguousRange& range)
        {
            BaseT::append(lib::string_view{range.data(),range.size()});
        }

        operator lib::string_view() const noexcept
        {
            return lib::string_view{this->data(),this->size()};
        }
};

using StringOnStack=StringOnStackT<>;

namespace pmr
{

//! @todo pmr allocator with custom memory resource
template <size_t PreallocatedSize=DefaultPreallocatedStringSize>
using StringOnStackT=common::StringOnStackT<PreallocatedSize,polymorphic_allocator<char>>;

using StringOnStack=StringOnStackT<DefaultPreallocatedStringSize>;
}

/********************** VectorOnStack **************************/

struct VectorOnStackTag
{};

constexpr size_t DefaultPreallocatedVectorSize=8;

template <typename T, size_t PreallocatedSize=DefaultPreallocatedVectorSize, typename FallbackAllocatorT=std::allocator<T>>
using PreallocatedVectorT = std::vector<T,AllocatorOnStack<T,PreallocatedSize,alignof(std::max_align_t),FallbackAllocatorT>>;

template <typename T, size_t PreallocatedSize=DefaultPreallocatedVectorSize, typename FallbackAllocatorT=std::allocator<T>>
class VectorOnStackT : public ArenaWrapperT<PreallocatedSize,FallbackAllocatorT>,
                       public PreallocatedVectorT<T,PreallocatedSize,FallbackAllocatorT>
{
public:

    using hana_tag=VectorOnStackTag;

    using ArenaHolderT=ArenaWrapperT<PreallocatedSize,FallbackAllocatorT>;
    using BaseT=PreallocatedVectorT<T,PreallocatedSize,FallbackAllocatorT>;
    using AllocaT=typename BaseT::allocator_type;    

    template <typename ...Args>
    VectorOnStackT(Args&&... args) : ArenaHolderT(),
        BaseT(std::forward<Args>(args)...,AllocaT{this->m_arena})
    {
        this->reserve(PreallocatedSize);
    }

    VectorOnStackT(std::initializer_list<T> items) : ArenaHolderT(),
        BaseT(std::move(items),AllocaT{this->m_arena})
    {
        this->reserve(PreallocatedSize);
    }

    VectorOnStackT(const std::vector<T>& items) : ArenaHolderT(),
        BaseT(AllocaT{this->m_arena})
    {
        this->reserve(std::max(PreallocatedSize,items.size()));
        for (auto&& it:items)
        {
            this->push_back(it);
        }
    }

    VectorOnStackT(std::vector<T>&& items) : ArenaHolderT(),
        BaseT(AllocaT{this->m_arena})
    {
        this->reserve(std::max(PreallocatedSize,items.size()));

        std::copy(std::make_move_iterator(items.begin()),
                  std::make_move_iterator(items.end()),
                  std::inserter(*this, this->end()));
    }

    VectorOnStackT(T&& item) : ArenaHolderT(),
        BaseT(AllocaT{this->m_arena})
    {
        this->reserve(PreallocatedSize);
        this->emplace_back(std::move(item));
    }

    VectorOnStackT() : ArenaHolderT(),
        BaseT(AllocaT{this->m_arena})
    {
        this->reserve(PreallocatedSize);
    }

    ~VectorOnStackT()
    {
        this->clear();
        this->shrink_to_fit();
    }

    VectorOnStackT(const VectorOnStackT& other) : ArenaHolderT(),
        BaseT(AllocaT{this->m_arena})
    {
        this->reserve(PreallocatedSize);
        this->append(other);
    }

    VectorOnStackT(VectorOnStackT&& other) : ArenaHolderT(),
        BaseT(AllocaT{this->m_arena})
    {
        this->reserve(PreallocatedSize);
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
//! @todo pmr allocator with custom memory resource
template <typename T, size_t PreallocatedSize=DefaultPreallocatedVectorSize>
using VectorOnStack=VectorOnStackT<T,PreallocatedSize,polymorphic_allocator<char>>;
}

/********************** FlatMapOnStack **************************/

constexpr size_t DefaultPreallocatedMapSize=8;

template <typename KeyT, typename ValueT,
         size_t PreallocatedSize=DefaultPreallocatedMapSize,
         typename CompareT=std::less<KeyT>,
         typename FallbackAllocatorT=std::allocator<std::pair<KeyT,ValueT>>
         >
using PreallocatedFlatMapT = FlatMap<KeyT,ValueT,CompareT,AllocatorOnStack<std::pair<KeyT,ValueT>,PreallocatedSize,alignof(std::max_align_t),FallbackAllocatorT>>;

template <typename KeyT, typename ValueT,
         size_t PreallocatedSize=DefaultPreallocatedMapSize,
         typename CompareT=std::less<KeyT>,
         typename FallbackAllocatorT=std::allocator<std::pair<KeyT,ValueT>>
         >
class FlatMapOnStackT : public ArenaWrapperT<PreallocatedSize,FallbackAllocatorT>,
                        public PreallocatedFlatMapT<KeyT,ValueT,PreallocatedSize,CompareT,FallbackAllocatorT>
{
    public:

        using ArenaHolderT=ArenaWrapperT<PreallocatedSize,FallbackAllocatorT>;
        using BaseT=PreallocatedFlatMapT<KeyT,ValueT,PreallocatedSize,CompareT,FallbackAllocatorT>;
        using AllocaT=typename BaseT::allocator_type;

        template <typename ...Args>
        FlatMapOnStackT(Args&&... args) : ArenaHolderT(),
            BaseT(std::forward<Args>(args)...,AllocaT{this->m_arena})
        {
            this->reserve(PreallocatedSize);
        }

        FlatMapOnStackT(std::initializer_list<typename BaseT::ItemT> elements) : ArenaHolderT(),
            BaseT(std::move(elements),AllocaT{this->m_arena})
        {
            this->reserve(PreallocatedSize);
        }

        FlatMapOnStackT() : ArenaHolderT(),
            BaseT(AllocaT{this->m_arena})
        {
            this->reserve(PreallocatedSize);
        }

        ~FlatMapOnStackT()
        {
            this->clear();
            this->shrinkToFit();
        }

        FlatMapOnStackT(const FlatMapOnStackT& other) : ArenaHolderT(),
            BaseT(AllocaT{this->m_arena})
        {
            this->reserve(PreallocatedSize);
            append(other);
        }

        FlatMapOnStackT(FlatMapOnStackT&& other) : ArenaHolderT(),
            BaseT(AllocaT{this->m_arena})
        {
            this->reserve(PreallocatedSize);
            append(other);
            other.clear();
        }

        FlatMapOnStackT& operator =(const FlatMapOnStackT& other)
        {
            if (this==&other)
            {
                return *this;
            }

            this->clear();
            append(other);
            return *this;
        }

        FlatMapOnStackT& operator =(FlatMapOnStackT&& other)
        {
            if (this==&other)
            {
                return *this;
            }

            this->clear();
            append(other);
            other.clear();
            return *this;
        }

    private:

        template <typename T1>
        void append(T1&& other)
        {
            this->beginRawInsert(other.size());
            for (size_t i=0;i<other.size();i++)
            {
                this->rawInsert(other.at(i));
            }
            this->endRawInsert(false);
        }
};

template <typename KeyT, typename ValueT,
         size_t PreallocatedSize=DefaultPreallocatedVectorSize,
         typename CompareT=std::less<KeyT>
         >
using FlatMapOnStack=FlatMapOnStackT<KeyT,ValueT,PreallocatedSize,CompareT>;

namespace pmr
{
//! @todo pmr allocator with custom memory resource
template <typename KeyT, typename ValueT,
         size_t PreallocatedSize=DefaultPreallocatedMapSize,
         typename CompareT=std::less<KeyT>
         >
using FlatMapOnStack=FlatMapOnStackT<KeyT,ValueT,PreallocatedSize,CompareT,polymorphic_allocator<char>>;
}

/********************** FlatSetOnStack **************************/

constexpr size_t DefaultPreallocatedSetSize=DefaultPreallocatedVectorSize;

template <typename T,
         size_t PreallocatedSize=DefaultPreallocatedSetSize,
         typename CompareT=std::less<T>,
         typename FallbackAllocatorT=std::allocator<T>
         >
using PreallocatedFlatSetT = FlatSet<T,CompareT,AllocatorOnStack<T,PreallocatedSize,alignof(std::max_align_t),FallbackAllocatorT>>;

template <typename T,
         size_t PreallocatedSize=DefaultPreallocatedVectorSize,
         typename CompareT=std::less<T>,
         typename FallbackAllocatorT=std::allocator<T>
         >
class FlatSetOnStackT : public ArenaWrapperT<PreallocatedSize,FallbackAllocatorT>,
                       public PreallocatedFlatSetT<T,PreallocatedSize,CompareT,FallbackAllocatorT>
{
public:

    using ArenaHolderT=ArenaWrapperT<PreallocatedSize,FallbackAllocatorT>;
    using BaseT=PreallocatedFlatSetT<T,PreallocatedSize,CompareT,FallbackAllocatorT>;
    using AllocaT=typename BaseT::allocator_type;

    template <typename ...Args>
    FlatSetOnStackT(Args&&... args) : ArenaHolderT(),
        BaseT(std::forward<Args>(args)...,AllocaT{this->m_arena})
    {
        this->reserve(PreallocatedSize);
    }

    FlatSetOnStackT(std::initializer_list<T> elements) : ArenaHolderT(),
        BaseT(std::move(elements),AllocaT{this->m_arena})
    {
        this->reserve(PreallocatedSize);
    }

    FlatSetOnStackT() : ArenaHolderT(),
        BaseT(AllocaT{this->m_arena})
    {
        this->reserve(PreallocatedSize);
    }

    ~FlatSetOnStackT()
    {
        this->clear();
        this->shrinkToFit();
    }

    FlatSetOnStackT(const FlatSetOnStackT& other) : ArenaHolderT(),
        BaseT(AllocaT{this->m_arena})
    {
        this->reserve(PreallocatedSize);
        append(other);
    }

    FlatSetOnStackT(FlatSetOnStackT&& other) : ArenaHolderT(),
        BaseT(AllocaT{this->m_arena})
    {
        this->reserve(PreallocatedSize);
        append(other);
        other.clear();
    }

    FlatSetOnStackT& operator =(const FlatSetOnStackT& other)
    {
        if (this==&other)
        {
            return *this;
        }

        this->clear();
        append(other);
        return *this;
    }

    FlatSetOnStackT& operator =(FlatSetOnStackT&& other)
    {
        if (this==&other)
        {
            return *this;
        }

        this->clear();
        append(other);
        other.clear();
        return *this;
    }

    private:

        template <typename T1>
        void append(T1&& other)
        {
            this->beginRawInsert(other.size());
            for (size_t i=0;i<other.size();i++)
            {
                this->rawInsert(other.at(i));
            }
            this->endRawInsert(false);
        }
};

template <typename T,
         size_t PreallocatedSize=DefaultPreallocatedSetSize,
         typename CompareT=std::less<T>
         >
using FlatSetOnStack=FlatSetOnStackT<T,PreallocatedSize,CompareT>;

namespace pmr
{
//! @todo pmr allocator with custom memory resource
template <typename T,
         size_t PreallocatedSize=DefaultPreallocatedSetSize,
         typename CompareT=std::less<T>
         >
using FlatSetOnStack=FlatSetOnStackT<T,PreallocatedSize,CompareT,polymorphic_allocator<char>>;
}

HATN_COMMON_NAMESPACE_END

#endif // HATNALLOCATORONSTACK_H
