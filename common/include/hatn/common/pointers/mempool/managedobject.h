/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/pointers/mempool/managedobject.h
  *
  *     Base class for managed objects stored in memory pools
  *
  */

/****************************************************************************/

#ifndef HATNMANAGEDOBJECT_MP_H
#define HATNMANAGEDOBJECT_MP_H

#include <memory>
#include <atomic>
#include <functional>

#include <hatn/common/common.h>
#include <hatn/common/pmr/pmrtypes.h>

HATN_COMMON_NAMESPACE_BEGIN

namespace pointers_mempool {

class WeakCtrl;
class ManagedObject_p;

//! Base managed object
class ManagedObject
{
    public:

        using isManagedObject=std::true_type;

        //! Ctor
        ManagedObject() noexcept :
            m_weakCtrl(nullptr),
            m_refCount(0),
            m_memoryResource(nullptr),
            m_typeSize(0),
            m_typeAlign(0)
        {
        }

        virtual ~ManagedObject()=default;
        ManagedObject(const ManagedObject&)=delete;
        ManagedObject(ManagedObject&&) =delete;
        ManagedObject& operator=(const ManagedObject&)=delete;
        ManagedObject& operator=(ManagedObject&&) =delete;

        //! Get reference count
        inline std::atomic<int>& refCount() noexcept
        {
            return m_refCount;
        }

        //! Get control structure for weak pointers
        inline WeakCtrl* weakCtrl() const noexcept
        {
            return m_weakCtrl.load(std::memory_order_relaxed);
        }

        //! Set control structure for weak pointers
        inline WeakCtrl* setWeakCtrl(WeakCtrl* weakCtrl) noexcept
        {
            WeakCtrl* oldObj=nullptr;
            if (!m_weakCtrl.compare_exchange_strong(oldObj,weakCtrl,std::memory_order_acquire,std::memory_order_acquire))
            {
                return oldObj;
            }
            return weakCtrl;
        }

        //! Reset control structure for weak pointers
        inline void resetWeakCtrl() noexcept
        {
            WeakCtrl* oldObj=m_weakCtrl.load(std::memory_order_acquire);
            if (m_weakCtrl.compare_exchange_strong(oldObj,nullptr,std::memory_order_acquire,std::memory_order_acquire))
            {
                m_refCount.store(0,std::memory_order_release);
            }
        }

        //! Keep memory resource
        inline void setMemoryResource(pmr::memory_resource* memoryResource) noexcept
        {
            m_memoryResource=memoryResource;
        }
        //! Get allocator
        inline pmr::memory_resource* memoryResource() const noexcept
        {
            return m_memoryResource;
        }

        //! Keep getter of object's type info
        template <typename T>
        inline void setTypeInfo() noexcept
        {
            m_typeSize=sizeof(T);
            m_typeAlign=alignof(T);
        }
        //! Get getter of object's type info
        inline std::pair<size_t,size_t> typeInfo() const noexcept
        {
            return std::make_pair(m_typeSize,m_typeAlign);
        }

        //! Comparation operator
        inline bool operator==(const ManagedObject& other) const noexcept
        {
            return this==&other;
        }

        //! Comparation operator
        inline bool operator!=(const ManagedObject& other) const noexcept
        {
            return !(this==&other);
        }

    private:

        std::atomic<WeakCtrl*> m_weakCtrl;
        std::atomic<int> m_refCount;

        pmr::memory_resource* m_memoryResource;

        size_t m_typeSize;
        size_t m_typeAlign;
};

template <typename T>
class EnableManaged : public ManagedObject
{
};

//! Wrapper of unmanaged object with managed object
template <typename T>
class ManagedWrapper : public ManagedObject, public T
{
    public:

        using T::T;

        /** Get reference to self **/
        inline const ManagedWrapper& value() const noexcept
        {
            return *this;
        }

        /** Get mutable pointer to self **/
        inline ManagedWrapper* mutableValue() noexcept
        {
            return this;
        }
};

//---------------------------------------------------------------
        } // namespace pointers_mempool
HATN_COMMON_NAMESPACE_END
#endif // HATNMANAGEDOBJECT_MP_H
