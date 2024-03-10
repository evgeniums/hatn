/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/syncinvoker.h
  *
  *     helper classes for synchronized/unsynchronized invokation of handlers
  *
  */

/****************************************************************************/

#ifndef HATNSYNCINVOKER_H
#define HATNSYNCINVOKER_H

#include <functional>
#include <atomic>

#include <hatn/common/common.h>
#include <hatn/common/thread.h>
#include <hatn/common/objectid.h>
#include <hatn/common/locker.h>

HATN_COMMON_NAMESPACE_BEGIN

//! Substitution of synchronization primitives for unsynchronized use.
struct SyncNo
{
    struct Lock
    {
        constexpr static void lock() noexcept
        {}
        constexpr static void unlock() noexcept
        {}
    };
    using SpinLock=Lock;

    template <typename T>
    class Atomic
    {
        public:

            Atomic() noexcept
            {}

            Atomic(const T& val) noexcept : m_val(val)
            {}

            void store(const T& val,std::memory_order =std::memory_order_seq_cst) noexcept
            {
                m_val=val;
            }
            T load(std::memory_order =std::memory_order_seq_cst) const noexcept
            {
                return m_val;
            }
            T fetch_add(const T& val,std::memory_order =std::memory_order_seq_cst) noexcept
            {
                return std::exchange(m_val,m_val+val);
            }
            T fetch_sub(const T& val,std::memory_order =std::memory_order_seq_cst) noexcept
            {
                return std::exchange(m_val,m_val-val);
            }
            bool compare_exchange_strong(T& checkDst,const T& newVal,std::memory_order=std::memory_order_seq_cst) noexcept
            {
                if (m_val==checkDst)
                {
                    m_val=newVal;
                    return true;
                }
                checkDst=m_val;
                return false;
            }
            bool compare_exchange_weak(T& checkDst,const T& newVal,std::memory_order order=std::memory_order_seq_cst) noexcept
            {
                return compare_exchange_strong(checkDst,newVal,order);
            }

        private:

            T m_val;
    };

    template <typename T>
    inline static void atomic_store_explicit(Atomic<T>* atomicVal, const T& val, std::memory_order order=std::memory_order_seq_cst) noexcept
    {
        atomicVal->store(val,order);
    }

    template <typename T>
    inline static T atomic_load_explicit(const Atomic<T>* atomicVal, std::memory_order order=std::memory_order_seq_cst) noexcept
    {
        return atomicVal->load(order);
    }
};

//! Wrapper of synchronization primitives.
struct SyncYes
{
    using SpinLock=common::SpinLock;
    template <typename T> using Atomic=std::atomic<T>;

    template <typename T>
    inline static void atomic_store_explicit(std::atomic<T>* atomicVal, const T& val, std::memory_order order) noexcept
    {
        std::atomic_store_explicit(atomicVal,val,order);
    }

    template <typename T>
    inline static T atomic_load_explicit(const std::atomic<T>* atomicVal, std::memory_order order) noexcept
    {
        return std::atomic_load_explicit(atomicVal,order);
    }
};

//! Invoke unsynchronized in the same thread
class UnsynchronizedInvoker : public WithThread
{
    public:

        using SyncT=SyncNo;

        using WithThread::WithThread;

        template <typename RetT=void>
        RetT execSync(const std::function<RetT()>& handler)
        {
            return handler();
        }
};

//! Invoke synchronized in the same thread using mutex
class SynchronizedMutexInvoker : public WithThread
{
    public:

        using SyncT=SyncYes;

        using WithThread::WithThread;

        template <typename RetT=void>
        RetT execSync(const std::function<RetT()>& handler)
        {
            MutexScopedLock l(m_mutex);
            return handler();
        }

    private:

        MutexLock m_mutex;
};

//! Invoke synchronized in synchronization thread
class SynchronizedThreadInvoker : public WithThread
{
    public:

        using SyncT=SyncYes;

        using WithThread::WithThread;

        template <typename RetT=void>
        RetT execSync(std::function<RetT()> handler)
        {
            if (Thread::currentThreadOrMain()!=thread())
            {
                return thread()->execSync<RetT>(std::move(handler));
            }
            return handler();
        }
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
#endif // HATNSYNCINVOKER_H
