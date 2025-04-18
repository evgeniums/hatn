/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file logcontext/streamlogger.h
  *
  *  Defines context logger to char stream.
  *
  */

/****************************************************************************/

#ifndef HATNFILELOGGER_H
#define HATNFILELOGGER_H

#include <hatn/common/pmr/allocatorfactory.h>

#include <hatn/logcontext/context.h>
#include <hatn/logcontext/fileloggertraits.h>

HATN_LOGCONTEXT_NAMESPACE_BEGIN

constexpr const char* FileLoggerName="filelogger";

template <typename ContextT=Subcontext>
class FileLoggerT : public BufLoggerT<FileLoggerTraits,ContextT>
{
    public:

        template <typename ...TraitsArgs>
        FileLoggerT(TraitsArgs&& ...traitsArgs) :
            BufLoggerT<FileLoggerTraits,ContextT>(FileLoggerName,std::forward<TraitsArgs>(traitsArgs)...)
        {}

        virtual Error loadLogConfig(
                const HATN_BASE_NAMESPACE::ConfigTree& configTree,
                const std::string& configPath,
                HATN_BASE_NAMESPACE::config_object::LogRecords& records
            ) override
        {
            return this->traits().loadLogConfig(configTree,configPath,records);
        }

        virtual Error start() override
        {
            return this->traits().start();
        }

        virtual Error close() override
        {
            return this->traits().close();
        }

        virtual void setAppConfig(const AppConfig& cfg) override
        {
            this->traits().setAllocatorFactory(cfg.allocatorFactory());
        }
};

using FileLogger=FileLoggerT<>;
using FileLogHandler=FileLogger;

HATN_LOGCONTEXT_NAMESPACE_END

#endif // HATNFILELOGGER_H
