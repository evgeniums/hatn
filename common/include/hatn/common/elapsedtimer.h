/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/elapsedtimer.h
  *
  *     Timer to measure elapsed time
  *
  */

/****************************************************************************/

#ifndef HATNELAPSEDTIMER_H
#define HATNELAPSEDTIMER_H

#include <chrono>
#include <string>

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

    /**
     * @brief Duration in time units, the most significant part is hour and the least significant part is millisecond
     */
    struct TimeDuration
    {
        uint32_t hours;
        uint8_t minutes;
        uint8_t seconds;
        uint16_t milliseconds;

        uint64_t totalMilliseconds;
    };

    /**
     * @brief Timer to measure elapsed duration
     */
    class HATN_COMMON_EXPORT ElapsedTimer final
    {
        public:

            using Clock = std::chrono::high_resolution_clock;
            using TimePoint = std::chrono::time_point<Clock>;

            //! Ctor
            ElapsedTimer():tp(std::chrono::high_resolution_clock::now())
            {}

            //! Reset timer
            inline void reset() noexcept
            {
                tp=std::chrono::high_resolution_clock::now();
            }

            //! Get time duration since last reset()
            inline TimeDuration elapsed() const noexcept
            {
                auto duration=std::chrono::high_resolution_clock::now()-tp;
                auto totalMilliseconds=std::chrono::duration_cast<std::chrono::milliseconds>(duration);

                auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
                duration -= hours;
                auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
                duration -= minutes;
                auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
                duration -= seconds;
                auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

                TimeDuration result={static_cast<uint32_t>(hours.count()),static_cast<uint8_t>(minutes.count()),static_cast<uint8_t>(seconds.count()),static_cast<uint16_t>(milliseconds.count()),static_cast<uint64_t>(totalMilliseconds.count())};
                return result;
            }

            /**
             * @brief Format elapsed time as string
             * @param totalMilliseconds If set then only total milliseconds are returned
             * @return String formated as hhh:mm:ss.ms or ms if totalMilliseconds set
             */
            std::string toString(bool totalMilliseconds=false) const;

            uint64_t elapsedMs() const noexcept
            {
                auto duration=std::chrono::high_resolution_clock::now()-tp;
                return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
            }

            uint32_t elapsedSecs() const noexcept
            {
                auto duration=std::chrono::high_resolution_clock::now()-tp;
                return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::seconds>(duration).count());
            }

        private:

            TimePoint tp;
    };

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END

#endif // HATNELAPSEDTIMER_H
