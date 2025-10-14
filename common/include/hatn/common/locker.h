/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/
/****************************************************************************/
/** @file common/locker.h
 *
 *     Locker classes.
 *
 */
/****************************************************************************/

#ifndef HATNLOCKER_H
#define HATNLOCKER_H

#include <atomic>
#include <mutex>

#include <hatn/common/common.h>
#include <hatn/common/thread.h>

HATN_COMMON_NAMESPACE_BEGIN

/**
 * Locker template class that can be either mutex or spinlock on atomic_flag
 */
template <bool Spinlock=true>
class Locker
{
};

//! Locker specialization with atomic spinlock
template <>
class Locker<true>
{
    public:

        constexpr static const bool Atomic=true;

        //! Ctor
        Locker()
#ifndef _MSC_VER
            : flag(false)
#endif
        {
#ifdef _MSC_VER
			flag.clear();
#endif
		}

        //! Lock
        inline void lock() noexcept
        {
            while (flag.test_and_set(std::memory_order_acquire)){}
        }

        //! Unlock
        inline void unlock() noexcept
        {
            flag.clear(std::memory_order_release);
        }

    private:

        std::atomic_flag flag;
};

//! Locker specialization with mutex
template <>
class Locker<false>
{
    public:

        constexpr static const bool Atomic=false;

        //! Lock
        inline void lock()
        {
            mutex.lock();
        }

        //! Unlock
        inline void unlock()
        {
            mutex.unlock();
        }

    private:

        std::mutex mutex;
};

//! Helper class to lock within scope
template <bool Spinlock=true> class ScopedLock final
{
    public:

        //! Ctor
        ScopedLock(
                Locker<Spinlock>& locker
            ) : m_locker(locker)
        {
            locker.lock();
        }

        //! Dtor
        ~ScopedLock()
        {
            m_locker.unlock();
        }

        ScopedLock(const ScopedLock&)=delete;
        ScopedLock(ScopedLock&&) =delete;
        ScopedLock& operator=(const ScopedLock&)=delete;
        ScopedLock& operator=(ScopedLock&&) =delete;

    private:

        Locker<Spinlock>& m_locker;
};

using SpinLock=Locker<true>;
using MutexLock=Locker<false>;

using SpinScopedLock=ScopedLock<true>;
using MutexScopedLock=ScopedLock<false>;

class LockerAnyMode final
{
    public:

        //! Ctor
        LockerAnyMode(bool spinned=false):m_spinned(spinned)
        {}

        //! Set mode
        inline void setSpinnedMode(bool enable) noexcept
        {
            m_spinned=enable;
        }

        //! Get mode
        inline bool isSpinnedMode() const noexcept
        {
            return m_spinned;
        }

        //! Lock
        inline void lock()
        {
            if (m_spinned)
            {
                m_spin.lock();
            }
            else
            {
                m_mutex.lock();
            }
        }

        //! Unlock
        inline void unlock()
        {
            if (m_spinned)
            {
                m_spin.unlock();
            }
            else
            {
                m_mutex.unlock();
            }
        }

    private:

        SpinLock m_spin;
        MutexLock m_mutex;

        bool m_spinned;
};

HATN_COMMON_NAMESPACE_END

#endif // HATNLOCKER_H
