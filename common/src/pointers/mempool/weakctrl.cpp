/*
    Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*

*/
/** @file common/pointers/mempool/weakctrl.h
  *
  *     Control block of weak pointer for managed objects
  *
  */

/****************************************************************************/

#ifndef HATNMANAGEDWEAKCTRL_MP_H
#define HATNMANAGEDWEAKCTRL_MP_H

#include <limits>
#include <atomic>

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN
namespace pointers_mempool {

class ManagedObject;

//! Control block for weak pointer
class WeakCtrl final
{
public:

    WeakCtrl(
        ManagedObject* sharedObj
        ) noexcept : m_refCount(0),
        m_sharedObj(sharedObj),
        m_lockState(0)
    {}

    ~WeakCtrl()=default;
    WeakCtrl(const WeakCtrl&)=delete;
    WeakCtrl(WeakCtrl&&) =delete;
    WeakCtrl& operator=(const WeakCtrl&)=delete;
    WeakCtrl& operator=(WeakCtrl&&) =delete;

    //! Lock control for deleting pointer
    inline bool lockForDelete() noexcept
    {
        int currentState=0;
        return m_lockState.compare_exchange_strong(currentState,std::numeric_limits<int>::min(),std::memory_order_acquire,std::memory_order_acquire);
    }

    //! Lock control for cloning pointer
    inline bool lockForClone() noexcept
    {
        return m_lockState.fetch_add(1,std::memory_order_acquire)>=0;
    }
    //! Unlock control after cloning pointer
    inline void unlockForClone() noexcept
    {
        m_lockState.fetch_sub(1,std::memory_order_release);
    }

    //! Get shared object
    inline const ManagedObject* sharedObject() const noexcept
    {
        return m_sharedObj;
    }

    //! Get shared object
    inline ManagedObject* sharedObject() noexcept
    {
        return m_sharedObj;
    }

    //! Clear shared object
    inline void clearSharedPtr() noexcept
    {
        m_sharedObj=nullptr;
    }

    //! Add reference count
    inline void addRef() noexcept
    {
        m_refCount.fetch_add(1, std::memory_order_release);
    }

    //! Reset memory block
    inline void reset() noexcept;

private:

    std::atomic<int> m_refCount;
    ManagedObject* m_sharedObj;
    std::atomic<int> m_lockState;
};

//---------------------------------------------------------------
} // namespace pointers_mempool
HATN_COMMON_NAMESPACE_END
#endif // HATNMANAGEDWEAKCTRL_MP_H
