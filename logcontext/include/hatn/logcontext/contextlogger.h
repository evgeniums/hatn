/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file logcontext/logger.h
  *
  *  Defines context logger.
  *
  */

/****************************************************************************/

#ifndef HATNCONTEXTCONTEXTLOGGER_H
#define HATNCONTEXTCONTEXTLOGGER_H

#include <hatn/common/singleton.h>

#include <hatn/logcontext/logcontext.h>
#include <hatn/logcontext/logger.h>

HATN_LOGCONTEXT_NAMESPACE_BEGIN

class HATN_LOGCONTEXT_EXPORT ContextLogger : public common::Singleton
{
    HATN_SINGLETON_DECLARE()

    public:

        static Logger& init(LogHandler handler);

        static Logger& instance();

        static void free();

        ~ContextLogger();

    private:

        ContextLogger(LogHandler handler);

        Logger m_logger;

        Logger& logger() noexcept
        {
            return m_logger;
        }
};

HATN_LOGCONTEXT_NAMESPACE_END

#define HATN_CTX_MSG_FORMAT(Format,...) fmt::format(Format,__VA_ARGS__)

#define HATN_CTX_EXPAND(x) x
#define HDU_CTX_GET_ARG3(arg1, arg2, arg3, ...) arg3
#define HDU_CTX_GET_ARG4(arg1, arg2, arg3, arg4, ...) arg4

#define HATN_CTX_LOG_IF_1(Level,Module) \
    if (HATN_LOGCONTEXT_NAMESPACE::Logger::passLog( \
            HATN_LOGCONTEXT_NAMESPACE::ContextLogger::instance(), \
            Level, \
            HATN_COMMON_NAMESPACE::ThreadLocalContext<HATN_LOGCONTEXT_NAMESPACE::Context>::value(), \
            Module) \
    )

#define HATN_CTX_LOG_1(Level,Msg,Module) \
    HATN_CTX_LOG_IF_1(Level,Module) \
    { \
        HATN_LOGCONTEXT_NAMESPACE::ContextLogger::instance().log( \
            Level, \
            HATN_COMMON_NAMESPACE::ThreadLocalContext<HATN_LOGCONTEXT_NAMESPACE::Context>::value(), \
            Msg, \
            Module \
        ); \
    }

#define HATN_CTX_LOG_IF_0(Level) \
if (HATN_LOGCONTEXT_NAMESPACE::Logger::passLog( \
            HATN_LOGCONTEXT_NAMESPACE::ContextLogger::instance(), \
            Level, \
            HATN_COMMON_NAMESPACE::ThreadLocalContext<HATN_LOGCONTEXT_NAMESPACE::Context>::value() \
        ) \
    )

#define HATN_CTX_LOG_0(Level,Msg) \
    HATN_CTX_LOG_IF_0(Level) \
    { \
            HATN_LOGCONTEXT_NAMESPACE::ContextLogger::instance().log( \
                      Level, \
                      HATN_COMMON_NAMESPACE::ThreadLocalContext<HATN_LOGCONTEXT_NAMESPACE::Context>::value(), \
                      Msg \
                    ); \
    }

#define HATN_CTX_LOG_IF(...) \
        HATN_CTX_EXPAND(HDU_CTX_GET_ARG3(__VA_ARGS__, \
                              HATN_CTX_LOG_IF_1(__VA_ARGS__), \
                              HATN_CTX_LOG_IF_0(__VA_ARGS__) \
                             ))

#define HATN_CTX_LOG(...) \
        HATN_CTX_EXPAND(HDU_CTX_GET_ARG4(__VA_ARGS__, \
                                 HATN_CTX_LOG_1(__VA_ARGS__), \
                                 HATN_CTX_LOG_0(__VA_ARGS__) \
                                 ))

#define HATN_CTX_INFO(...) HATN_CTX_LOG(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Info,__VA_ARGS__)
#define HATN_CTX_ERROR(...) HATN_CTX_LOG(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Error,__VA_ARGS__)
#define HATN_CTX_WARN(...) HATN_CTX_LOG(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Warn,__VA_ARGS__)
#define HATN_CTX_DEBUG(...) HATN_CTX_LOG(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Debug,__VA_ARGS__)
#define HATN_CTX_FATAL(...) HATN_CTX_LOG(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Fatal,__VA_ARGS__)
#define HATN_CTX_TRACE(...) HATN_CTX_LOG(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Trace,__VA_ARGS__)

#define HATN_CTX_INFO_IF(...) HATN_CTX_LOG_IF(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Info,__VA_ARGS__)
#define HATN_CTX_ERROR_IF(...) HATN_CTX_LOG_IF(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Error,__VA_ARGS__)
#define HATN_CTX_WARN_IF(...) HATN_CTX_LOG_IF(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Warn,__VA_ARGS__)
#define HATN_CTX_DEBUG_IF(...) HATN_CTX_LOG_IF(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Debug,__VA_ARGS__)
#define HATN_CTX_FATAL_IF(...) HATN_CTX_LOG_IF(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Fatal,__VA_ARGS__)
#define HATN_CTX_TRACE_IF(...) HATN_CTX_LOG_IF(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Trace,__VA_ARGS__)

#define HATN_CTX_LOG_RECORDS_M(Level,Msg,Module,...) \
    HATN_CTX_LOG_IF_1(Level,Module) \
    { \
            std::vector<HATN_LOGCONTEXT_NAMESPACE::Record> recs{__VA_ARGS__}; \
            HATN_LOGCONTEXT_NAMESPACE::ContextLogger::instance().log( \
                      Level, \
                      HATN_COMMON_NAMESPACE::ThreadLocalContext<HATN_LOGCONTEXT_NAMESPACE::Context>::value(), \
                      Msg, \
                      Module, \
                      std::move(recs) \
                    ); \
    }

#define HATN_CTX_LOG_RECORDS(Level,Msg,...) \
    HATN_CTX_LOG_IF_0(Level) \
    { \
            std::vector<HATN_LOGCONTEXT_NAMESPACE::Record> recs{__VA_ARGS__}; \
            HATN_LOGCONTEXT_NAMESPACE::ContextLogger::instance().log( \
              Level, \
              HATN_COMMON_NAMESPACE::ThreadLocalContext<HATN_LOGCONTEXT_NAMESPACE::Context>::value(), \
              Msg, \
              HATN_COMMON_NAMESPACE::lib::string_view{}, \
              std::move(recs) \
              ); \
    }

#define HATN_CTX_INFO_RECORDS(...) HATN_CTX_LOG_RECORDS(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Info,__VA_ARGS__)
#define HATN_CTX_ERROR_RECORDS(...) HATN_CTX_LOG_RECORDS(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Error,__VA_ARGS__)
#define HATN_CTX_WARN_RECORDS(...) HATN_CTX_LOG_RECORDS(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Warn,__VA_ARGS__)
#define HATN_CTX_DEBUG_RECORDS(...) HATN_CTX_LOG_RECORDS(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Debug,__VA_ARGS__)
#define HATN_CTX_FATAL_RECORDS(...) HATN_CTX_LOG_RECORDS(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Fatal,__VA_ARGS__)
#define HATN_CTX_TRACE_RECORDS(...) HATN_CTX_LOG_RECORDS(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Trace,__VA_ARGS__)

#define HATN_CTX_INFO_RECORDS_M(...) HATN_CTX_LOG_RECORDS_M(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Info,__VA_ARGS__)
#define HATN_CTX_ERROR_RECORDS_M(...) HATN_CTX_LOG_RECORDS_M(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Error,__VA_ARGS__)
#define HATN_CTX_WARN_RECORDS_M(...) HATN_CTX_LOG_RECORDS_M(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Warn,__VA_ARGS__)
#define HATN_CTX_DEBUG_RECORDS_M(...) HATN_CTX_LOG_RECORDS_M(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Debug,__VA_ARGS__)
#define HATN_CTX_FATAL_RECORDS_M(...) HATN_CTX_LOG_RECORDS_M(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Fatal,__VA_ARGS__)
#define HATN_CTX_TRACE_RECORDS_M(...) HATN_CTX_LOG_RECORDS_M(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Trace,__VA_ARGS__)

#endif // HATNCONTEXTCONTEXTLOGGER_H
