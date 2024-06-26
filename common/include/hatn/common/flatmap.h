/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/flatmap.h
  *
  *     Flat map based on vector for fast lookups
  *
  */

/****************************************************************************/

#ifndef HATNFLATMAP_H
#define HATNFLATMAP_H

#include <algorithm>
#include <vector>
#include <functional>
#include <type_traits>

#include <hatn/common/common.h>
#include <hatn/common/pmr/pmrtypes.h>

HATN_COMMON_NAMESPACE_BEGIN

//---------------------------------------------------------------

namespace detail {

template<typename _ForwardIterator, typename _Tp, typename _Compare>
  _ForwardIterator
  lowerBound(_ForwardIterator __first, _ForwardIterator __last,
        const _Tp& __val, _Compare __comp)
  {
    typedef typename std::iterator_traits<_ForwardIterator>::difference_type
  _DistanceType;

    _DistanceType __len = std::distance(__first, __last);

  while (__len > 0)
  {
    _DistanceType __half = __len >> 1;
    _ForwardIterator __middle = __first;
    std::advance(__middle, __half);
    if (__comp(*__middle, __val))
      {
        __first = __middle;
        ++__first;
        __len = __len - __half - 1;
      }
#if 1
    // not looking for the first element if there is more than one key
    // in FlatMap there can not be duplicates
    // actually, sometimes it is even slower than in std version
    else if (__comp(__val,*__middle))
    {
        __len = __half;
    }
    else
    {
        return __middle;
    }
#else
    // original version from the std, looking for the first element if there are duplicare keys
    else
    {
        __len = __half;
    }
#endif
  }
    return __first;
  }


    template<class ForwardIt, class T, class Compare=std::less<>>
    ForwardIt binary_find(ForwardIt first, ForwardIt last, const T& value, Compare comp={})
    {
        first = lowerBound(first, last, value, comp);
        if (first!=last && !comp(value, *first))
        {
            return first;
        }
        return last;
    }

    template <typename VectorT, typename ForwardIt>
    size_t vector_iterator_index(const VectorT& vec, const ForwardIt& it)
    {
        return it-vec.begin();
    }
}

//---------------------------------------------------------------

template <typename VectorT, typename ItemT>
class FlatContainerIteratorBase
{
    public:

        using iterator_category = std::random_access_iterator_tag;
        using value_type = ItemT;
        using pointer = ItemT*;
        using const_pointer = const ItemT*;
        using reference = ItemT&;
        using const_reference = const ItemT&;
        using difference_type = size_t;

    public:

        FlatContainerIteratorBase(
                VectorT* vec,
                size_t idx=0
            )  : m_vec(vec),
                 m_idx(idx)
        {
            if (idx>vec->size())
            {
                throw std::out_of_range("Index of flat container's vector item is invalid");
            }
        }

        FlatContainerIteratorBase& operator=(size_t idx) noexcept
        {
            m_idx = idx;
            return *this;
        }

        operator bool() const noexcept
        {
            return m_idx<m_vec->size();
        }
        bool operator==(const FlatContainerIteratorBase& other) const noexcept
        {
            return (m_idx == other.m_idx);
        }

        FlatContainerIteratorBase& operator+=(const difference_type& diff)
        {
            m_idx+=diff;
            if (m_idx>m_vec->size())
            {
                m_idx-=diff;
                throw std::out_of_range("End of flat container is reached");
            }
            return *this;
        }
        FlatContainerIteratorBase& operator-=(const difference_type& diff)
        {
            if (m_idx<diff)
            {
                throw std::out_of_range("Begin of flat container is reached");
            }
            m_idx-=diff;
            return *this;
        }
        FlatContainerIteratorBase& operator++()
        {
            ++m_idx;
            if (m_idx>m_vec->size())
            {
                --m_idx;
                throw std::out_of_range("End of flat container is reached");
            }
            return *this;
        }
        FlatContainerIteratorBase& operator--()
        {
            if (m_idx==0)
            {
                throw std::out_of_range("Begin of flat container is reached");
            }
            --m_idx;
            return *this;
        }
        FlatContainerIteratorBase operator+(const difference_type& diff)
        {
            auto it=FlatContainerIterator(m_vec,m_idx);
            it+=diff;
            return it;
        }
        FlatContainerIteratorBase operator-(const difference_type& diff)
        {
            auto it=FlatContainerIterator(m_vec,m_idx);
            it-=diff;
            return it;
        }

    protected:

        VectorT* m_vec;
        size_t m_idx;

        template <typename ContainerItemT,
                  typename AllocT,
                  typename CompareItemT,
                  typename CompareKeyT
                  >
        friend class FlatContainer;
};

template <typename VectorT, typename ItemT>
class FlatContainerIterator : public FlatContainerIteratorBase<VectorT,ItemT>
{
    public:

        using base=FlatContainerIteratorBase<VectorT,ItemT>;
        using base::FlatContainerIteratorBase;

        typename base::reference operator*()
        {
            if (this->m_idx==this->m_vec->size())
            {
                throw std::out_of_range("Iterator of flat container is invalid");
            }
            return (*this->m_vec)[this->m_idx];
        }

        typename base::pointer operator->()
        {
            if (this->m_idx==this->m_vec->size())
            {
                throw std::out_of_range("Iterator of flat container is invalid");
            }
            return &((*this->m_vec)[this->m_idx]);
        }
};

template <typename VectorT, typename ItemT>
class FlatContainerIteratorConst : public FlatContainerIteratorBase<VectorT,ItemT>
{
    public:

        using base=FlatContainerIteratorBase<VectorT,ItemT>;
        using base::FlatContainerIteratorBase;

        typename base::const_reference operator*() const
        {
            if (this->m_idx==this->m_vec->size())
            {
                throw std::out_of_range("Iterator of flat container is invalid");
            }
            return (*this->m_vec)[this->m_idx];
        }

        typename base::const_pointer operator->() const
        {
            if (this->m_idx==this->m_vec->size())
            {
                throw std::out_of_range("Iterator of flat container is invalid");
            }
            return &((*this->m_vec)[this->m_idx]);
        }
};

//---------------------------------------------------------------

template <typename ItemT,
          typename AllocT=std::allocator<ItemT>,
          typename CompareItemT=std::less<ItemT>,
          typename CompareKeyT=CompareItemT
          >
class FlatContainer
{
    public:

        using vector_type=std::vector<ItemT,AllocT>;
        using iterator=FlatContainerIterator<vector_type,ItemT>;
        using const_iterator=FlatContainerIteratorConst<const vector_type,ItemT>;

        FlatContainer()
        {}
        FlatContainer(const AllocT& alloc) : m_vec(alloc)
        {}
        FlatContainer(size_t reserveSize,const AllocT& alloc=AllocT()) : m_vec(alloc)
        {
            m_vec.reserve(reserveSize);
        }

        void reserve(size_t n)
        {
            m_vec.reserve(n);
        }
        void shrinkToFit()
        {
            m_vec.shrink_to_fit();
        }
        size_t size() const noexcept
        {
            return m_vec.size();
        }
        size_t capacity() const noexcept
        {
            return m_vec.capacity();
        }
        bool empty() const noexcept
        {
            return m_vec.empty();
        }
        void clear() noexcept
        {
            m_vec.clear();
        }

        iterator begin() noexcept
        {
            return iterator(&m_vec,0);
        }
        const_iterator begin() const noexcept
        {
            return const_iterator(&m_vec,0);
        }

        iterator end() noexcept
        {
            return iterator(&m_vec,m_vec.size());
        }
        const_iterator end() const noexcept
        {
            return const_iterator(&m_vec,m_vec.size());
        }

        template <typename KeyT, typename CompareT=CompareKeyT>
        iterator find(const KeyT& item)
        {
            auto it=detail::binary_find(m_vec.begin(),m_vec.end(),item,CompareT());
            return iterator(&m_vec,detail::vector_iterator_index(m_vec,it));
        }
        template <typename KeyT, typename CompareT=CompareKeyT>
        const_iterator find(const KeyT& item) const
        {
            auto it=detail::binary_find(m_vec.begin(),m_vec.end(),item,CompareT());
            return const_iterator(&m_vec,detail::vector_iterator_index(m_vec,it));
        }

        iterator erase(iterator position)
        {
            auto it=m_vec.erase(m_vec.begin()+position.m_idx);
            return iterator(&m_vec,detail::vector_iterator_index(m_vec,it));
        }
        template <typename KeyT>
        iterator erase(const KeyT& item)
        {
            iterator it=find(item);
            if (it!=end())
            {
                return erase(it);
            }
            return end();
        }

        template <typename Arg>
        std::pair<iterator,bool> insert(Arg&& val,
                    typename std::enable_if_t<
                                        std::is_same<
                                            std::decay_t<Arg>,ItemT
                                            >::value,
                                        void*
                                        > =nullptr
                )
        {
            auto it=detail::lowerBound(m_vec.begin(),m_vec.end(),val,CompareItemT());
            bool created=true;
            if (it!=m_vec.end() && !CompareItemT()(val,*it))
            {
                created=false;
                *it=std::forward<Arg>(val);
            }
            else
            {
                it=m_vec.insert(it,std::forward<Arg>(val));
            }
            auto idx=detail::vector_iterator_index(m_vec,it);
            return std::make_pair(iterator(&m_vec,idx),created);
        }

        template <typename KeyT, typename ValueT>
        std::pair<iterator,bool> emplace(KeyT&& key, ValueT&& val)
        {
            auto it=detail::lowerBound(m_vec.begin(),m_vec.end(),key,CompareKeyT{});
            bool created=true;
            if (it!=m_vec.end() && !CompareKeyT{}(key,it->first))
            {
                created=false;
                it->first=std::forward<KeyT>(key);
                it->second=std::forward<ValueT>(val);
            }
            else
            {
                it=m_vec.emplace(it,std::forward<KeyT>(key),std::forward<ValueT>(val));
            }
            auto idx=detail::vector_iterator_index(m_vec,it);
            return std::make_pair(iterator(&m_vec,idx),created);
        }

        void beginRawInsert(size_t itemCount=0)
        {
            if (itemCount!=0)
            {
                reserve(capacity()+itemCount);
            }
        }
        void rawInsert(const ItemT& val)
        {
            m_vec.push_back(val);
        }
        void rawInsert(ItemT&& val)
        {
            m_vec.push_back(std::move(val));
        }
        void endRawInsert()
        {
            std::sort(m_vec.begin(),m_vec.end(),CompareItemT());
        }

    private:

        vector_type m_vec;
};

//---------------------------------------------------------------

namespace detail
{

template <typename KeyT, typename ValueT, typename CompareKeyT=std::less<KeyT>>
struct FlatMapCompareItem
{
    constexpr bool operator() (const std::pair<KeyT,ValueT>& l,
                     const std::pair<KeyT,ValueT>& r) noexcept
    {
        return CompareKeyT{}(l.first,r.first);
    }

    template <typename T>
    constexpr bool operator() (const T& l,
                     const std::pair<KeyT,ValueT>& r) noexcept
    {
        return CompareKeyT{}(l,r.first);
    }

    template <typename T>
    constexpr bool operator() (const std::pair<KeyT,ValueT>& l,const T& r) noexcept
    {
        return CompareKeyT{}(l.first,r);
    }

    template <typename T1, typename T2>
    constexpr bool operator() (const T1& l,const T2& r) noexcept
    {
        return std::less<KeyT>{}(l,r);
    }
};
}

//---------------------------------------------------------------
/**
 * Implementation of flat map container that keeps its elements and keys in one solid array.
 */
template <typename KeyT, typename ValueT,
          typename CompareKeyT=std::less<KeyT>,
          typename AllocT=std::allocator<std::pair<KeyT,ValueT>>
          >
class FlatMap : public FlatContainer<std::pair<KeyT,ValueT>,
                                    AllocT,
                                    detail::FlatMapCompareItem<KeyT,ValueT,CompareKeyT>
                                    >
{
    public:

        using ItemT=std::pair<KeyT,ValueT>;
        using BaseContainer=FlatContainer<std::pair<KeyT,ValueT>,
                            AllocT,
                            detail::FlatMapCompareItem<KeyT,ValueT,CompareKeyT>
                            >;
        using iterator=typename BaseContainer::iterator;
        using const_iterator=typename BaseContainer::const_iterator;

        using BaseContainer::BaseContainer;

        std::pair<iterator,bool> insert_or_assign(const ItemT& val)
        {
            return this->insert(val);
        }
        std::pair<iterator,bool> insert_or_assign(ItemT&& val)
        {
            return this->insert(std::move(val));
        }

        ValueT& operator[](const KeyT& key)
        {
            auto it=this->find(key);
            if (it==this->end())
            {
                auto ret=this->insert(std::make_pair(key,ValueT()));
                it=ret.first;
            }
            return it->second;
        }
        ValueT& operator[](KeyT&& key)
        {
            auto it=this->find(key);
            if (it==this->end())
            {
                auto ret=this->insert(std::make_pair(std::move(key),ValueT()));
                it=ret.first;
            }
            return it->second;
        }
};

//---------------------------------------------------------------

template <typename ValueT,
          typename CompareT=std::less<ValueT>,
          typename AllocT=std::allocator<ValueT>
          >
class FlatSet : public FlatContainer<ValueT,AllocT,CompareT>
{
    public:

        using FlatContainer<ValueT,AllocT,CompareT>::FlatContainer;
};

//---------------------------------------------------------------

namespace pmr {

template <typename KeyT, typename ValueT,
          typename CompareKeyT=std::less<KeyT>
          >
using FlatMap=common::FlatMap<KeyT,ValueT,CompareKeyT,polymorphic_allocator<std::pair<KeyT,ValueT>>>;

template <typename ValueT,
          typename CompareT=std::less<ValueT>
          >
using FlatSet=common::FlatSet<ValueT,CompareT,polymorphic_allocator<ValueT>>;

} // namespace common

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
#endif // HATNFLATMAP_H
