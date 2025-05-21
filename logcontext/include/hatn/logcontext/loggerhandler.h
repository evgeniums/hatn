/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file logcontext/loggerhandler.h
  *
  */

/****************************************************************************/

#ifndef HATNCONTEXTLOGGERHANDLER_H
#define HATNCONTEXTLOGGERHANDLER_H

#include <hatn/common/error.h>

#include <hatn/base/base.h>
#include <hatn/base/configobject.h>

#include <hatn/logcontext/logcontext.h>
#include <hatn/logcontext/record.h>

HATN_BASE_NAMESPACE_BEGIN

class ConfigTree;

HATN_BASE_NAMESPACE_END

HATN_LOGCONTEXT_NAMESPACE_BEGIN

class AppConfig;

template <typename ContextT>
class LoggerHandlerT
{
    public:

        virtual ~LoggerHandlerT()
        {}

        LoggerHandlerT(std::string name) : m_name(std::move(name))
        {}

        const std::string& name() const noexcept
        {
            return m_name;
        }

        LoggerHandlerT(const LoggerHandlerT&)=default;
        LoggerHandlerT(LoggerHandlerT&&)=default;
        LoggerHandlerT& operator=(const LoggerHandlerT&)=default;
        LoggerHandlerT& operator=(LoggerHandlerT&&)=default;

        virtual void log(
            LogLevel level,
            const ContextT* ctx,
            const char* msg,
            const ImmediateRecords& records,
            lib::string_view module=lib::string_view{}
        )=0;

        virtual void logError(
            LogLevel level,
            const Error& ec,
            const ContextT* ctx,
            const char* msg,
            const ImmediateRecords& records,
            lib::string_view module=lib::string_view{}
        )=0;

        virtual void logClose(
            LogLevel level,
            const Error& ec,
            const ContextT* ctx,
            const char* msg,
            const ImmediateRecords& records,
            lib::string_view module=lib::string_view{}
        )=0;

        virtual void logCloseApi(
            LogLevel level,
            const Error& ec,
            const ContextT* ctx,
            const char* msg,
            const ImmediateRecords& records,
            lib::string_view module=lib::string_view{}
        )=0;

        virtual void log(
            LogLevel level,
            const ContextT* ctx,
            const char* msg,
            lib::string_view module=lib::string_view{}
            )=0;

        virtual void logError(
            LogLevel level,
            const Error& ec,
            const ContextT* ctx,
            const char* msg,
            lib::string_view module=lib::string_view{}
            )=0;

        virtual void logClose(
            LogLevel level,
            const Error& ec,
            const ContextT* ctx,
            const char* msg,
            lib::string_view module=lib::string_view{}
            )=0;

        virtual void logCloseApi(
            LogLevel level,
            const Error& ec,
            const ContextT* ctx,
            const char* msg,
            lib::string_view module=lib::string_view{}
            )=0;

        virtual Error start()
        {
            return OK;
        }

        virtual Error close()
        {
            return OK;
        }

        virtual void setAppConfig(const AppConfig&)
        {}

        virtual Error loadLogConfig(
                const HATN_BASE_NAMESPACE::ConfigTree& configTree,
                const std::string& configPath,
                HATN_BASE_NAMESPACE::config_object::LogRecords& logRecords
            )
        {
            std::ignore=configTree;
            std::ignore=configPath;
            std::ignore=logRecords;
            return OK;
        }

    private:

        std::string m_name;
};

template <typename ContextT>
class LoggerHandlerTraits
{
    public:

        LoggerHandlerTraits(std::shared_ptr<LoggerHandlerT<ContextT>> handler)
            : m_handler(std::move(handler))
        {}

        void log(
            LogLevel level,
            const ContextT* ctx,
            const char* msg,
            const ImmediateRecords& records,
            lib::string_view module=lib::string_view{}
        )
        {
            m_handler->log(level,ctx,msg,records,module);
        }

        void logError(
            LogLevel level,
            const Error& ec,
            const ContextT* ctx,
            const char* msg,
            const ImmediateRecords& records,
            lib::string_view module=lib::string_view{}
        )
        {
            m_handler->logError(level,ec,ctx,msg,records,module);
        }

        void logClose(
            LogLevel level,
            const Error& ec,
            const ContextT* ctx,
            const char* msg,
            const ImmediateRecords& records,
            lib::string_view module=lib::string_view{}
        )
        {
            m_handler->logClose(level,ec,ctx,msg,records,module);
        }

        void logCloseApi(
            LogLevel level,
            const Error& ec,
            const ContextT* ctx,
            const char* msg,
            const ImmediateRecords& records,
            lib::string_view module=lib::string_view{}
        )
        {
            m_handler->logCloseApi(level,ec,ctx,msg,records,module);
        }

        void log(
            LogLevel level,
            const ContextT* ctx,
            const char* msg,
            lib::string_view module=lib::string_view{}
            )
        {
            m_handler->log(level,ctx,msg,module);
        }

        void logError(
            LogLevel level,
            const Error& ec,
            const ContextT* ctx,
            const char* msg,
            lib::string_view module=lib::string_view{}
            )
        {
            m_handler->logError(level,ec,ctx,msg,module);
        }

        void logClose(
            LogLevel level,
            const Error& ec,
            const ContextT* ctx,
            const char* msg,
            lib::string_view module=lib::string_view{}
            )
        {
            m_handler->logClose(level,ec,ctx,msg,module);
        }

        void logCloseApi(
            LogLevel level,
            const Error& ec,
            const ContextT* ctx,
            const char* msg,
            lib::string_view module=lib::string_view{}
            )
        {
            m_handler->logCloseApi(level,ec,ctx,msg,module);
        }

        Error close()
        {
            return m_handler->close();
        }

        Error start()
        {
            return m_handler->start();
        }

        void setAppConfig(const AppConfig& cfg)
        {
            return m_handler->setAppConfig(cfg);
        }

        Error loadLogConfig(
            const HATN_BASE_NAMESPACE::ConfigTree& configTree,
            const std::string& configPath,
            HATN_BASE_NAMESPACE::config_object::LogRecords& logRecords
            )
        {
            return m_handler->loadLogConfig(configTree,configPath,logRecords);
        }

    private:

        std::shared_ptr<LoggerHandlerT<ContextT>> m_handler;
};

HATN_LOGCONTEXT_NAMESPACE_END

#endif // HATNCONTEXTLOGGERHANDLER_H
