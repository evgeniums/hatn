/*
    Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file common/timer.h
  *
  *     Base generic timer class
  *
  */

/****************************************************************************/

#ifndef HATNTIMER_H
#define HATNTIMER_H

#include <cstdint>
#include <functional>

#include <hatn/common/common.h>
#include <hatn/common/objecttraits.h>

HATN_COMMON_NAMESPACE_BEGIN

struct TimerTypes
{
    //! Timer status
    enum Status : int
    {
        Timeout=0,
        Cancel=1
    };

    using HandlerType=std::function<void(Status)>;
};

/**
 * @brief Base generic timer class
 *
 * By default it is a single shot timer that runs once. Call setSingleShot(false) to make it run in repititive manner.
 */
template <typename Traits>
class Timer : public WithTraits<Traits>, public TimerTypes
{
    public:

        //! Ctor
        template <typename ... Args>
        Timer(
            TimerTypes::HandlerType handler,
            Args&& ...traitsArgs
        ) noexcept :
            WithTraits<Traits>(std::forward<Args>(traitsArgs)...),
            m_handler(std::move(handler)),m_singleShot(true)
        {}

        //! Ctor
        template <typename ... Args>
        Timer(
            Args&& ...traitsArgs
        ) noexcept :
            WithTraits<Traits>(std::forward<Args>(traitsArgs)...),
            m_singleShot(true)
        {}

        //! Dtor is intentionally not virtual
        ~Timer()
        {
            reset();
        }

        Timer(const Timer&)=delete;
        Timer(Timer&&)=default;
        Timer& operator=(const Timer&)=delete;
        Timer& operator=(Timer&&)=default;

        //! Start timer
        inline void start()
        {
            this->traits().start();
        }

        //! Start timer with explicit callbacks
        inline void start(
            TimerTypes::HandlerType handler
        )
        {
            setHandler(std::move(handler));
            start();
        }

        //! Stop timer
        inline void stop() noexcept
        {
            this->traits().stop();
        }

        //! Cancel timer == alias for stop
        inline void cancel() noexcept
        {
            stop();
        }

        //! Set period in microseconds
        inline void setPeriodUs(uint64_t microseconds) noexcept
        {
            this->traits().setPeriodUs(microseconds);
        }

        //! Get period in microseconds
        inline uint64_t periodUs() const noexcept
        {
            return this->traits().getPeriodUs();
        }

        /**
        * @brief Set handler
        * @param HandlerType Callback handler
        *
        * No restart is performed. Handler will be used on the next timer invokation.
        * Not thread safe. Call it only either in timer's thread or when a thread is stopped.
        */
        inline void setHandler(
            TimerTypes::HandlerType handler
        ) noexcept
        {
            m_handler=std::move(handler);
        }

        //! Check if the timer is running
        inline bool isRunning() const noexcept
        {
            return this->traits().isRunning();
        }

        /**
         * @brief Reset handler
         *
         * Not thread safe. Call it only either in timer's thread or when a thread is stopped.
         */
        inline void reset() noexcept
        {            
            m_handler=TimerTypes::HandlerType();
        }

        /**
         * @brief Check if it is single shot timer
         * @return True if single shot
         */
        inline bool isSingleShot() const noexcept
        {
            return m_singleShot;
        }

        //! Set singleshot mode
        inline void setSingleShot(bool enable) noexcept
        {
            m_singleShot=enable;
        }

        inline TimerTypes::HandlerType& handler() noexcept
        {
            return m_handler;
        }

    private:

        TimerTypes::HandlerType m_handler;
        bool m_singleShot;
};

//---------------------------------------------------------------
    HATN_COMMON_NAMESPACE_END

#endif // HATNTIMER_H
