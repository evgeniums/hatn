/*
    Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file common/pointers/std/weakptr.h
  *
  *     Weak pointer for managed objects
  *
  */

/****************************************************************************/

#ifndef HATNMANAGEDWEAKPTR_STD_H
#define HATNMANAGEDWEAKPTR_STD_H

#include <memory>

#include <hatn/common/common.h>

#include <hatn/common/pointers/std/sharedptr.h>

HATN_COMMON_NAMESPACE_BEGIN
namespace pointers_std {

//! Weak pointer
template <typename T> class WeakPtr final
{
    public:

        //! Ctor from object
        WeakPtr(
                const SharedPtr<T>& sharedPtr=SharedPtr<T>()
            ):p(sharedPtr.p)
        {
        }

        //! Move ctor
        WeakPtr(WeakPtr<T>&& ptr) noexcept
            :p(std::move(ptr.p))
        {
        }

        //! Copy ctor
        WeakPtr(const WeakPtr<T>& ptr):p(ptr.p)
        {
        }

        //! Assignment operator
        inline WeakPtr& operator=(const WeakPtr<T>& ptr) noexcept
        {
            if(this != &ptr)
            {
               p=ptr.p;
            }            
            return *this;
        }

        //! Move assignment operator
        inline WeakPtr& operator=(WeakPtr<T>&& ptr) noexcept
        {
            if(this != &ptr)
            {
               p=std::move(ptr.p);
            }
            return *this;
        }

        //! Assignment operator to underlying object
        inline WeakPtr& operator=(T* obj) noexcept
        {
            p=obj->sharedFromThis().p;
            return *this;
        }

        //! Assignment operator to shared pointer
        inline WeakPtr& operator=(SharedPtr<T> sharedPtr) noexcept
        {
            p=sharedPtr.p;
            return *this;
        }

        //! Dtor
        ~WeakPtr()
        {
            p.reset();
        }

        //! Reset pointer
        inline void reset() noexcept
        {
            p.reset();
        }

        //! Check if shared pointer exists and lock it
        inline SharedPtr<T> lock() noexcept
        {
            return p.lock();
        }

        //! Check if it is a null pointer
        inline bool isNull() const noexcept
        {
            return p.expired();
        }

    private:

        std::weak_ptr<T> p;
};

//---------------------------------------------------------------
        } // namespace pointers_std
    HATN_COMMON_NAMESPACE_END
#endif // HATNMANAGEDWEAKPTR_STD_H
