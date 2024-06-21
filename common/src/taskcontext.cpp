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

#include <hatn/common/datetime.h>
#include <hatn/common/taskcontext.h>

HATN_COMMON_NAMESPACE_BEGIN

//---------------------------------------------------------------

void TaskContext::generateId(DataBuf buf)
{
    static std::atomic<uint64_t> seq{1};

    auto s=seq.fetch_add(1);
    auto dt=common::DateTime::millisecondsSinceEpoch();
    auto rand=common::Random::uniform(1,0xFFFFFF);

    fmt::format_to_n(buf.data(),buf.size(),"{:010x}{:04x}{:06x}",dt,s&0xFFFF,rand;
}

//---------------------------------------------------------------

void TaskContext::beforeThreadProcessing()
{}

//---------------------------------------------------------------

void TaskContext::afterThreadProcessing()
{}

//---------------------------------------------------------------

HATN_COMMON_NAMESPACE_END
