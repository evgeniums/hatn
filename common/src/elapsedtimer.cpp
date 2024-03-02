/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/elapsedtimer.cpp
  *
  *     Timer to measure elapsed time
  *
  */

#include <hatn/common/format.h>

#include <hatn/common/elapsedtimer.h>

HATN_COMMON_NAMESPACE_BEGIN

//---------------------------------------------------------------
std::string ElapsedTimer::toString(bool totalMilliseconds) const
{
    auto duration=elapsed();
    if (totalMilliseconds)
    {
        return fmt::format("{}ms",duration.totalMilliseconds);
    }
    return fmt::format("{:#03}:{:#02}:{:#02}.{:#03}",duration.hours,duration.minutes,duration.seconds,duration.milliseconds);
}

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
