/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/asiotimer.h
  *
  *     Timer classes with asio backend
  *
  */

/****************************************************************************/

#ifndef HATNASIOTIMER_H
#define HATNASIOTIMER_H

#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/high_resolution_timer.hpp>

#include <hatn/common/common.h>
#include <hatn/common/timer.h>
#include <hatn/common/sharedptr.h>

HATN_COMMON_NAMESPACE_BEGIN

class Thread;

//! Traits for timer class using boost asio timers
template <typename NativeT,typename TimerT>
class AsioTimerTraits
{
    public:

        using HandlerType=TimerTypes::HandlerType;

        //! Ctor
        AsioTimerTraits(
            TimerT* timer,
            Thread* thread
        ) : m_timer(timer),
            m_native(thread->asioContextRef()),
            m_periodUs(0),
            m_running(false),
            m_autoAsyncGuard(true)
       {}

        //! Start timer
        void start();

        //! Stop timer
        inline void stop() noexcept
        {
            if (m_running.load())
            {
                m_running.store(false);
                m_native.cancel();
            }
        }

        //! Set period in microseconds
        inline void setPeriodUs(uint64_t microseconds) noexcept
        {
            m_periodUs=microseconds;
        }

        //! Get period in microseconds
        inline uint64_t periodUs() const noexcept
        {
            return m_periodUs;
        }

        //! Check if the timer is running
        inline bool isRunning() const noexcept
        {
            return m_running.load();
        }

        void setAutoAsyncGuardEnabled(bool enable) noexcept
        {
            m_autoAsyncGuard=enable;
        }

        bool isAutoAsyncGuardEnabled() const noexcept
        {
            return m_autoAsyncGuard;
        }

    private:

        TimerT* m_timer;
        NativeT m_native;
        uint64_t m_periodUs;
        std::atomic<bool> m_running;

        bool m_autoAsyncGuard;

        inline void invokeHandler(const boost::system::error_code& ec);
};

//! Timer class using boost asio timers
template <typename NativeT>
class AsioTimer : public Timer<AsioTimerTraits<NativeT,AsioTimer<NativeT>>>,
                  public EnableSharedFromThis<AsioTimer<NativeT>>
{
    public:

        using TimerT=Timer<AsioTimerTraits<NativeT,AsioTimer<NativeT>>>;
        using HandlerT=TimerTypes::HandlerType;

        AsioTimer(
            Thread* thread,
            HandlerT handler=HandlerT()
        ) : TimerT(
                std::move(handler),
                this,
                thread
            )
        {}
};

using AsioDeadlineTimer=AsioTimer<boost::asio::deadline_timer>;
using AsioHighResolutionTimer=AsioTimer<boost::asio::high_resolution_timer>;

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END

#endif // HATNASIOTIMER_H
