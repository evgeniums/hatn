/*
    Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file common/pointers/mempool/uniqueptr.h
  *
  *     Unique pointer with allocator
  *
  */

/****************************************************************************/

#ifndef HATNUNIQUEPTR_H
#define HATNUNIQUEPTR_H

#include <hatn/common/common.h>

#include <hatn/common/pmr/pmrtypes.h>

HATN_COMMON_NAMESPACE_BEGIN
namespace pmr {

//! Unique pointer that can be used with polimorphic allocator
template <typename T>
class UniquePtr final
{
    public:

        UniquePtr(T* ptr=nullptr, polymorphic_allocator<T>* alloc=nullptr) noexcept : m_ptr(ptr),m_alloc(alloc)
        {}

        UniquePtr(UniquePtr&& other) noexcept
            : m_ptr(std::exchange(other.m_ptr,nullptr)),
              m_alloc(std::exchange(other.m_alloc,nullptr))
        {
        }
        UniquePtr& operator=(UniquePtr&& other) noexcept
        {
            if (this!=&other)
            {
                m_ptr=std::exchange(other.m_ptr,nullptr);
                m_alloc=std::exchange(other.m_alloc,nullptr);
            }
            return *this;
        }

        UniquePtr(const UniquePtr& other)=delete;
        UniquePtr& operator=(const UniquePtr& other)=delete;

        ~UniquePtr()
        {
            doReset();
        }

        T* operator-> () noexcept
        {
            return m_ptr;
        }
        const T* operator-> () const noexcept
        {
            return m_ptr;
        }
        T* get() noexcept
        {
            return m_ptr;
        }
        const T* get() const noexcept
        {
            return m_ptr;
        }

        T& operator* () noexcept
        {
            return *m_ptr;
        }
        const T& operator* () const noexcept
        {
            return *m_ptr;
        }

        void reset(T* ptr=nullptr, polymorphic_allocator<T>* alloc=nullptr) noexcept
        {
            doReset();
            m_ptr=ptr;
            m_alloc=alloc;
        }

        bool isNull() const noexcept
        {
            return m_ptr==nullptr;
        }

        inline operator bool() const noexcept
        {
            return !isNull();
        }

    private:

        void doReset() noexcept
        {
            if (m_ptr)
            {
                if (m_alloc)
                {
                    destroyDeallocate(m_ptr,*m_alloc);
                }
                else
                {
                    delete m_ptr;
                }
            }
        }

        T* m_ptr;
        polymorphic_allocator<T>* m_alloc;
};

//! Make unique object
template <typename T,typename ... Args>
constexpr UniquePtr<T> makeUnique(Args&&... args)
{
    return UniquePtr<T>(new T(std::forward<Args>(args)...));
}
//! Allocate unique object using allocator
template <typename T,typename ... Args>
constexpr UniquePtr<T> allocateUnique(pmr::polymorphic_allocator<T>& allocator,Args&&... args)
{
    return UniquePtr<T>(allocateConstruct(allocator,std::forward<Args>(args)...),&allocator);
}

//---------------------------------------------------------------
        } // namespace pointers_mempool
    HATN_COMMON_NAMESPACE_END
#endif // HATNUNIQUEPTR_H
