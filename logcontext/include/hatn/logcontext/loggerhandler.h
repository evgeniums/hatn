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

#include <hatn/logcontext/logcontext.h>
#include <hatn/logcontext/record.h>

HATN_LOGCONTEXT_NAMESPACE_BEGIN

template <typename ContextT>
class LoggerHandlerT
{
    public:

        virtual ~LoggerHandlerT()
        {}

        LoggerHandlerT()=default;
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

    private:

        std::shared_ptr<LoggerHandlerT<ContextT>> m_handler;
};

HATN_LOGCONTEXT_NAMESPACE_END

#endif // HATNCONTEXTLOGGERHANDLER_H
