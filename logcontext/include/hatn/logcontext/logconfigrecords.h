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

HATN_NAMESPACE_BEGIN

struct logConfigRecordsT
{
    void operator() (const std::string& msg,
                    HATN_BASE_NAMESPACE::config_object::LogRecords& records,
                    bool autoClear=true) const
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

    void operator() (const std::string& msg,
                    const std::string& logModule,
                    HATN_BASE_NAMESPACE::config_object::LogRecords& records,
                    bool autoClear=true) const
    {
        for (const auto& record : records)
        {
            HATN_CTX_INFO_RECORDS_M(msg.c_str(),logModule,{record.name,record.value});
        }
        if (autoClear)
        {
            records.clear();
        }
    }
};
constexpr logConfigRecordsT logConfigRecords{};

struct loadLogConfigT
{
    template <typename T>
    Error operator() (
                     const std::string& logMsg,
                     T& obj,
                     const HATN_BASE_NAMESPACE::ConfigTree& tree,
                     const HATN_BASE_NAMESPACE::ConfigTreePath& path,
                     const HATN_BASE_NAMESPACE::config_object::LogSettings& logSettings={}
                     ) const
    {
        HATN_BASE_NAMESPACE::config_object::LogRecords records;
        auto ec=obj.loadLogConfig(tree,path,records,logSettings);
        logConfigRecords(logMsg,records,false);
        return ec;
    }

    template <typename T>
    Error operator() (
        const std::string& logMsg,
        const std::string& logModule,
        T& obj,
        const HATN_BASE_NAMESPACE::ConfigTree& tree,
        const HATN_BASE_NAMESPACE::ConfigTreePath& path,
        const HATN_BASE_NAMESPACE::config_object::LogSettings& logSettings={}
        ) const
    {
        HATN_BASE_NAMESPACE::config_object::LogRecords records;
        auto ec=obj.loadLogConfig(tree,path,records,logSettings);
        logConfigRecords(logMsg,logModule,records,false);
        return ec;
    }

    template <typename T, typename ValidatorT>
    Error operator() (
        const std::string& logMsg,
        T& obj,
        const HATN_BASE_NAMESPACE::ConfigTree& tree,
        const HATN_BASE_NAMESPACE::ConfigTreePath& path,
        const ValidatorT& validator,
        const HATN_BASE_NAMESPACE::config_object::LogSettings& logSettings={}
        ) const
    {
        HATN_BASE_NAMESPACE::config_object::LogRecords records;
        auto ec=obj.loadLogConfig(tree,path,validator,records,logSettings);
        logConfigRecords(logMsg,records,false);
        return ec;
    }

    template <typename T, typename ValidatorT>
    Error operator() (
        const std::string& logMsg,
        const std::string& logModule,
        T& obj,
        const HATN_BASE_NAMESPACE::ConfigTree& tree,
        const HATN_BASE_NAMESPACE::ConfigTreePath& path,
        const ValidatorT& validator,
        const HATN_BASE_NAMESPACE::config_object::LogSettings& logSettings={}
        ) const
    {
        HATN_BASE_NAMESPACE::config_object::LogRecords records;
        auto ec=obj.loadLogConfig(tree,path,validator,records,logSettings);
        logConfigRecords(logMsg,logModule,records,false);
        return ec;
    }
};
constexpr loadLogConfigT loadLogConfig{};

HATN_NAMESPACE_END

#endif // HATNCLOGCONFIGRECORDS_H
