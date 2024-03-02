/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/pointers/mempool/weakptr.h
  *
  *     Weak pointer for managed objects
  *
  */

/****************************************************************************/

#ifndef HATNMANAGEDWEAKPTR_MP_H
#define HATNMANAGEDWEAKPTR_MP_H

#include <memory>
#include <atomic>

#include <hatn/common/common.h>

#include <hatn/common/pointers/mempool/sharedptr.h>
#include <hatn/common/pointers/mempool/weakctrl.h>
#include <hatn/common/pointers/mempool/weakpool.h>

HATN_COMMON_NAMESPACE_BEGIN
namespace pointers_mempool {

//! Weak pointer
template <typename T> class WeakPtr final
{
    public:

        //! Default ctor
        WeakPtr() noexcept
            :d(nullptr),obj(nullptr)
        {
        }

        //! Ctor from object
        WeakPtr(
                const SharedPtr<T>& sharedPtr
            ):d(nullptr),obj(nullptr)
        {
            loadFromShared(sharedPtr);
        }

        //! Move ctor
        WeakPtr(WeakPtr<T>&& ptr) noexcept
            :d(ptr.d),obj(ptr.obj)
        {
            ptr.d=nullptr;
            ptr.obj=nullptr;
        }

        //! Copy ctor
        WeakPtr(const WeakPtr<T>& ptr) noexcept
            :d(ptr.d),obj(ptr.obj)
        {
            addRef();
        }

        //! Assignment operator
        WeakPtr& operator=(const WeakPtr<T>& ptr) noexcept
        {
            if(this != &ptr)
            {
                reset();
                d=ptr.d;
                obj=ptr.obj;
                addRef();
            }
            return *this;
        }

        //! Move assignment operator
        WeakPtr& operator=(WeakPtr<T>&& ptr) noexcept
        {
            if(this != &ptr)
            {
                reset();
                d=ptr.d;
                obj=ptr.obj;
                ptr.d=nullptr;
                ptr.obj=nullptr;
            }
            return *this;
        }

        //! Assignment operator to underlying object if it contains sharedFromThis()
        template <typename T1> WeakPtr& operator=(T1* ptr)
        {
            if (static_cast<T*>(ptr)!=obj)
            {
                reset();
                if (ptr!=nullptr)
                {
                    loadFromShared(CheckSharedFromThis<T1>::createShared(ptr).template staticCast<T>());
                }
            }
            return *this;
        }

        //! Assignment operator to shared pointer
        WeakPtr& operator=(SharedPtr<T> sharedPtr)
        {
            loadFromShared(sharedPtr);
            return *this;
        }

        //! Dtor
        ~WeakPtr()
        {
            reset();
        }

        //! Reset pointer
        inline void reset() noexcept
        {
            if (d!=nullptr)
            {
                d->reset();
                d=nullptr;
                obj=nullptr;
            }
        }

        //! Check if shared pointer exists and lock it
        inline SharedPtr<T> lock() noexcept
        {
            SharedPtr<T> result;
            if (d!=nullptr)
            {
                if (d->lockForClone())
                {
                    auto&& m=d->sharedObject();
                    if (m!=nullptr)
                    {
                        result.reset(obj,m);
                    }
                    d->unlockForClone();
                }
            }
            return result;
        }

        //! Check if it is a null pointer
        bool isNull() const noexcept
        {
            return d==nullptr || d->sharedObject()==nullptr;
        }

#ifndef HATN_TEST_SMART_POINTERS
    private:
#endif
        //! Get control structure
        inline const WeakCtrl* ctrl() const noexcept
        {
            return d;
        }

    private:

        WeakCtrl* d;
        T* obj; // keep typed pointer here to avoid dynamic casting in lock()

        //! Increment reference count
        inline void addRef() noexcept
        {
            if (d!=nullptr)
            {
                d->addRef();
            }
        }

        void loadFromShared(const SharedPtr<T>& sharedPtr)
        {
            if (sharedPtr.isNull())
            {
                reset();
                return;
            }
            if (d!=nullptr)
            {
                if (d == sharedPtr.weakCtrl())
                {
                    return;
                }
                reset();
            }
            obj=sharedPtr.get();
            if (sharedPtr.weakCtrl()!=nullptr)
            {
                d=sharedPtr.weakCtrl();
            }
            else
            {
                d=WeakPool::instance()->allocate(SharedPtrBase<T>::template m(&sharedPtr));
                addRef(); // this one is for shared pointer
            }
            addRef(); // this one is for weak pointer
        }
};

//---------------------------------------------------------------
        } // namespace pointers_mempool
    HATN_COMMON_NAMESPACE_END
#endif // HATNMANAGEDWEAKPTR_MP_H
