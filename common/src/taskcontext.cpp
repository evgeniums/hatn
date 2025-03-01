/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/taskcontext.сpp
  *
  *   COntains definition of TaskContext.
  */

#include <atomic>

#include <hatn/common/error.h>
#include <hatn/common/datetime.h>
#include <hatn/common/random.h>
#include <hatn/common/taskcontext.h>

HATN_COMMON_NAMESPACE_BEGIN

//---------------------------------------------------------------

std::chrono::time_point<TaskContext::Clock> TaskContext::generateId(TaskContextId& id)
{
    static std::atomic<uint64_t> seq{1};

    auto s=seq.fetch_add(1);
    auto started=nowUtc();
    auto millisSinceEpoch=std::chrono::duration_cast<std::chrono::milliseconds>(started.time_since_epoch()).count();
    auto rand=common::Random::uniform(1,0xFFFFFF);

    id.resize(id.capacity());
    fmt::format_to_n(id.data(),id.size(),"{:010x}{:04x}{:06x}",millisSinceEpoch,s&0xFFFF,rand);
    return started;
}

TaskContextId TaskContext::generateId()
{
    TaskContextId id;
    generateId(id);
    return id;
}

//---------------------------------------------------------------

Result<std::chrono::time_point<TaskContext::Clock>> TaskContext::extractStarted(const lib::string_view& id)
{
    if (id.size()<TaskContextId::capacity())
    {
        return commonError(CommonError::INVALID_DATETIME_FORMAT);
    }

    uint64_t millisSinceEpoch{0};

#if __cplusplus < 201703L
    try
    {
        std::string dtStr{id.data(), id.size()};
        millisSinceEpoch=std::stoll(dtStr,nullptr,16);
    }
    catch(...)
    {
        return commonError(CommonError::INVALID_DATETIME_FORMAT);
    }
#else
    auto r = std::from_chars(id.data(), id.data() + id.size(), millisSinceEpoch, 16);
    if (r.ec != std::errc())
    {
        return commonError(CommonError::INVALID_DATETIME_FORMAT);
    }
#endif
    auto dSinceEpoch=std::chrono::milliseconds(millisSinceEpoch);
    return std::chrono::time_point<TaskContext::Clock>(dSinceEpoch);
}

//---------------------------------------------------------------

HATN_COMMON_NAMESPACE_END
