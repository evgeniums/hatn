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
#include <hatn/common/taskcontext.h>

HATN_COMMON_NAMESPACE_BEGIN

//---------------------------------------------------------------

DateTime TaskContext::generateId(TaskContextId& id)
{
    static std::atomic<uint64_t> seq{1};

    auto s=seq.fetch_add(1);
    auto dt=common::DateTime::millisecondsSinceEpoch();
    auto rand=common::Random::uniform(1,0xFFFFFF);

    fmt::format_to_n(id.data(),id.size(),"{:010x}{:04x}{:06x}",dt,s&0xFFFF,rand);
    return DateTime::fromEpochMs(dt);
}

//---------------------------------------------------------------

void TaskContext::beforeThreadProcessing()
{
    //! @todo Keep in local thread
}

//---------------------------------------------------------------

void TaskContext::afterThreadProcessing()
{
    //! @todo Move from local thread
}

//---------------------------------------------------------------

Result<DateTime> TaskContext::extractDateTime(const TaskContextId& id)
{
    if (id.size()<id.capacity())
    {
        return commonError(CommonError::INVALID_DATETIME_FORMAT);
    }

    uint64_t dtNum{0};

#if __cplusplus < 201703L
    try
    {
        std::string dtStr{id.data(), id.size()};
        dtNum=std::stoll(dtStr,nullptr,16);
    }
    catch(...)
    {
        return commonError(CommonError::INVALID_DATETIME_FORMAT);
    }
#else
    auto r = std::from_chars(id.data(), id.data() + DateTimeLength, dtNum, 16);
    if (r.ec != std::errc())
    {
        return CommonError::INVALID_DATETIME_FORMAT;
    }
#endif
    return DateTime::fromNumber(dtNum);
}

//---------------------------------------------------------------

HATN_COMMON_NAMESPACE_END
