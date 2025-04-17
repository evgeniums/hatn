/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file logcontext/logconfigrecords.h
  *
  */

/****************************************************************************/

#ifndef HATNCLOGCONFIGRECORDS_H
#define HATNCLOGCONFIGRECORDS_H

#include <hatn/base/configobject.h>

#include <hatn/logcontext/contextlogger.h>

HATN_LOGCONTEXT_NAMESPACE_BEGIN

struct logConfigRecordsT
{
    void operator() (const std::string& msg, HATN_BASE_NAMESPACE::config_object::LogRecords& records, bool autoClear=true) const
    {
        for (const auto& record : records)
        {
            HATN_CTX_INFO_RECORDS(msg.c_str(),{record.name,record.value});
        }
        if (autoClear)
        {
            records.clear();
        }
    }
};
constexpr logConfigRecordsT logConfigRecords{};

HATN_LOGCONTEXT_NAMESPACE_END

#endif // HATNCLOGCONFIGRECORDS_H
