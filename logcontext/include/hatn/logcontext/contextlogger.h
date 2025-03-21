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
#include <hatn/common/taskcontext.h>

#include <hatn/logcontext/logcontext.h>
#include <hatn/logcontext/logger.h>
#include <hatn/logcontext/context.h>

HATN_LOGCONTEXT_NAMESPACE_BEGIN

class HATN_LOGCONTEXT_EXPORT ContextLogger : public common::Singleton
{
    HATN_SINGLETON_DECLARE()

    public:

        static Logger& init(std::shared_ptr<LoggerHandler> handler);

        static Logger& instance();

        static bool available() noexcept;

        static void free();

        ~ContextLogger();

    private:

        ContextLogger(std::shared_ptr<LoggerHandler> handler);

        Logger m_logger;

        Logger& logger() noexcept
        {
            return m_logger;
        }
};

template <typename LogContextT>
Logger& contextLogger(const LogContextT* ctx=nullptr)
{
    if (ctx!=nullptr && ctx->logger()!=nullptr)
    {
        return *ctx->logger();
    }
    return ContextLogger::instance();
}

template <typename LogContextT>
bool contextLoggerAvailable(const LogContextT* ctx=nullptr)
{
    if (ctx!=nullptr && ctx->logger()!=nullptr)
    {
        return true;
    }
    return ContextLogger::available();
}

HATN_LOGCONTEXT_NAMESPACE_END

#define HATN_CTX_EXPAND(x) x
#define HATN_CTX_GET_ARG3(arg1, arg2, arg3, ...) arg3
#define HATN_CTX_GET_ARG4(arg1, arg2, arg3, arg4, ...) arg4
#define HATN_CTX_GET_ARG5(arg1, arg2, arg3, arg4, arg5, ...) arg5

#define HATN_LOG_CTX() \
    HATN_LOGCONTEXT_NAMESPACE::contextLogger(HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context))

#define HATN_LOG_CTX_AVAILABLE() \
    HATN_LOGCONTEXT_NAMESPACE::contextLoggerAvailable(HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context))

#define HATN_CTX_LOG_IF_1(Level,Module) \
    if (HATN_LOG_CTX_AVAILABLE() && HATN_LOGCONTEXT_NAMESPACE::Logger::passLog( \
            HATN_LOG_CTX(), \
            Level, \
            HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context), \
            Module ) \
    )

#define HATN_CTX_LOG_1(Level,Msg,Module) \
    HATN_CTX_LOG_IF_1(Level,Module) \
    { \
        HATN_LOG_CTX().log( \
            Level, \
            HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context), \
            Msg, \
            Module \
        ); \
    }

#define HATN_CTX_DEBUG_IF_2(Verbosity,Module) \
    if (HATN_LOG_CTX_AVAILABLE() && HATN_LOGCONTEXT_NAMESPACE::Logger::passDebugLog( \
                                                    HATN_LOG_CTX(), \
                                                    HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context), \
                                                    Verbosity, \
                                                    Module ) \
        )

#define HATN_CTX_DEBUG_IF_1(Verbosity) \
    if (HATN_LOG_CTX_AVAILABLE() && HATN_LOGCONTEXT_NAMESPACE::Logger::passDebugLog( \
                                                    HATN_LOG_CTX(), \
                                                    HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context), \
                                                    Verbosity \
                                                ) \
        )

#define HATN_CTX_DEBUG_IF_0() HATN_CTX_DEBUG_IF_1(0)


#define HATN_CTX_DEBUG_2(Verbosity,Msg,Module) \
    HATN_CTX_DEBUG_IF_2(Verbosity,Module) \
    { \
        HATN_LOG_CTX().log( \
                  HATN_LOGCONTEXT_NAMESPACE::LogLevel::Debug, \
                  HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context), \
                  Msg, \
                  Module \
                ); \
    }

#define HATN_CTX_DEBUG_1(Verbosity,Msg) \
    HATN_CTX_DEBUG_IF_1(Verbosity) \
    { \
        HATN_LOG_CTX().log( \
                  HATN_LOGCONTEXT_NAMESPACE::LogLevel::Debug, \
                  HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context), \
                  Msg \
                ); \
    }

#define HATN_CTX_DEBUG_0(Msg) HATN_CTX_DEBUG_1(0,Msg)


#define HATN_CTX_LOG_ERR_1(Level,Err,Msg,Module) \
    HATN_CTX_LOG_IF_1(Level,Module) \
    { \
            HATN_LOG_CTX().logError( \
                      Level, \
                      Err, \
                      HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context), \
                      Msg, \
                      Module \
                    ); \
    }

#define HATN_CTX_LOG_IF_0(Level) \
    if (HATN_LOG_CTX_AVAILABLE() && HATN_LOGCONTEXT_NAMESPACE::Logger::passLog( \
                HATN_LOG_CTX(), \
                Level, \
                HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context) \
            ) \
        )

#define HATN_CTX_LOG_0(Level,Msg) \
    HATN_CTX_LOG_IF_0(Level) \
    { \
            HATN_LOG_CTX().log( \
                      Level, \
                      HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context), \
                      Msg \
                    ); \
    }

#define HATN_CTX_CLOSE(Err,Msg) \
    HATN_CTX_LOG_IF_0(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Info) \
    { \
            HATN_LOG_CTX().logClose( \
                      HATN_LOGCONTEXT_NAMESPACE::LogLevel::Info, \
                      Err, \
                      HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context), \
                      Msg \
                    ); \
    }

#define HATN_CTX_CLOSE_API(Err,Msg) \
    HATN_CTX_LOG_IF_0(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Info) \
    { \
            HATN_LOG_CTX().logCloseApi( \
                           HATN_LOGCONTEXT_NAMESPACE::LogLevel::Info, \
                           Err, \
                           HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context), \
                           Msg \
                    ); \
    }

#define HATN_CTX_LOG_ERR_0(Level,Err,Msg) \
    HATN_CTX_LOG_IF_0(Level) \
    { \
            HATN_LOG_CTX().logError( \
                      Level, \
                      Err, \
                      HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context), \
                      Msg \
                    ); \
    }

#define HATN_CTX_LOG_IF(...) \
        HATN_CTX_EXPAND(HATN_CTX_GET_ARG3(__VA_ARGS__, \
                              HATN_CTX_LOG_IF_1, \
                              HATN_CTX_LOG_IF_0 \
                             )(__VA_ARGS__))

#define HATN_CTX_LOG(...) \
        HATN_CTX_EXPAND(HATN_CTX_GET_ARG4(__VA_ARGS__, \
                                 HATN_CTX_LOG_1, \
                                 HATN_CTX_LOG_0 \
                                 )(__VA_ARGS__))

#define HATN_CTX_LOG_ERR(...) \
        HATN_CTX_EXPAND(HATN_CTX_GET_ARG5(__VA_ARGS__, \
                                  HATN_CTX_LOG_ERR_1, \
                                  HATN_CTX_LOG_ERR_0 \
                                  )(__VA_ARGS__))

#define HATN_CTX_LOG_DEBUG(...) \
        HATN_CTX_EXPAND(HATN_CTX_GET_ARG4(__VA_ARGS__, \
                                  HATN_CTX_DEBUG_2, \
                                  HATN_CTX_DEBUG_1, \
                                  HATN_CTX_DEBUG_0 \
                                  )(__VA_ARGS__))

#define HATN_CTX_INFO(...) HATN_CTX_LOG(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Info,__VA_ARGS__)
#define HATN_CTX_WARN(...) HATN_CTX_LOG(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Warn,__VA_ARGS__)
#define HATN_CTX_TRACE(...) HATN_CTX_LOG(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Trace,__VA_ARGS__)

#define HATN_CTX_DEBUG(...) HATN_CTX_LOG_DEBUG(__VA_ARGS__)

#define HATN_CTX_FATAL(...) HATN_CTX_LOG_ERR(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Fatal,__VA_ARGS__)
#define HATN_CTX_ERROR(...) HATN_CTX_LOG_ERR(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Error,__VA_ARGS__)

#define HATN_CTX_LOG_RECORDS_M(Level,Msg,Module,...) \
    HATN_CTX_LOG_IF_1(Level,Module) \
    { \
            HATN_LOG_CTX().log( \
                      Level, \
                      HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context), \
                      Msg, \
                      {__VA_ARGS__}, \
                      Module \
                    ); \
    }

#define HATN_CTX_DEBUG_RECORDS_M(Verbosity,Msg,Module,...) \
    HATN_CTX_DEBUG_IF_2(Verbosity,Module) \
    { \
            HATN_LOG_CTX().log( \
                      HATN_LOGCONTEXT_NAMESPACE::LogLevel::Debug, \
                      HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context), \
                      Msg, \
                      {__VA_ARGS__}, \
                      Module \
                    ); \
    }

#define HATN_CTX_LOG_RECORDS_ERR_M(Level,Err,Msg,Module,...) \
    HATN_CTX_LOG_IF_1(Level,Module) \
    { \
            HATN_LOG_CTX().logError( \
                      Level, \
                      Err, \
                      HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context), \
                      Msg, \
                      {__VA_ARGS__}, \
                      Module \
                    ); \
    }

#define HATN_CTX_LOG_RECORDS(Level,Msg,...) \
    HATN_CTX_LOG_IF_0(Level) \
    { \
            HATN_LOG_CTX().log( \
              Level, \
              HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context), \
              Msg, \
              {__VA_ARGS__}, \
              HATN_COMMON_NAMESPACE::lib::string_view{} \
              ); \
    }

#define HATN_CTX_DEBUG_RECORDS(Verbosity,Msg,...) \
    HATN_CTX_DEBUG_IF_1(Verbosity) \
    { \
            HATN_LOG_CTX().log( \
                      HATN_LOGCONTEXT_NAMESPACE::LogLevel::Debug, \
                      HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context), \
                      Msg, \
                      {__VA_ARGS__}, \
                      HATN_COMMON_NAMESPACE::lib::string_view{} \
                    ); \
    }

#define HATN_CTX_LOG_RECORDS_ERR(Level,Err,Msg,...) \
    HATN_CTX_LOG_IF_0(Level) \
    { \
            HATN_LOG_CTX().logError( \
                      Level, \
                      Err, \
                      HATN_THREAD_SUBCONTEXT(HATN_LOGCONTEXT_NAMESPACE::Context), \
                      Msg, \
                      {__VA_ARGS__}, \
                      HATN_COMMON_NAMESPACE::lib::string_view{} \
                    ); \
    }

#define HATN_CTX_INFO_RECORDS(Msg,...) HATN_CTX_LOG_RECORDS(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Info,Msg,__VA_ARGS__)
#define HATN_CTX_WARN_RECORDS(Msg,...) HATN_CTX_LOG_RECORDS(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Warn,Msg,__VA_ARGS__)
#define HATN_CTX_TRACE_RECORDS(Msg,...) HATN_CTX_LOG_RECORDS(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Trace,Msg,__VA_ARGS__)

#define HATN_CTX_ERROR_RECORDS(Err,Msg,...) HATN_CTX_LOG_RECORDS_ERR(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Error,Err,Msg,__VA_ARGS__)
#define HATN_CTX_FATAL_RECORDS(Err,Msg,...) HATN_CTX_LOG_RECORDS_ERR(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Fatal,Err,Msg,__VA_ARGS__)

#define HATN_CTX_INFO_RECORDS_M(Msg,Module,...) HATN_CTX_LOG_RECORDS_M(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Info,Msg,Module,__VA_ARGS__)
#define HATN_CTX_WARN_RECORDS_M(Msg,Module,...) HATN_CTX_LOG_RECORDS_M(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Warn,Msg,Module,__VA_ARGS__)
#define HATN_CTX_TRACE_RECORDS_M(Msg,Module,...) HATN_CTX_LOG_RECORDS_M(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Trace,Msg,Module,__VA_ARGS__)

#define HATN_CTX_ERROR_RECORDS_M(Err,Msg,Module,...) HATN_CTX_LOG_RECORDS_ERR_M(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Error,Err,Msg,Module,__VA_ARGS__)
#define HATN_CTX_FATAL_RECORDS_M(Err,Msg,Module,...) HATN_CTX_LOG_RECORDS_ERR_M(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Fatal,Err,Msg,Module,__VA_ARGS__)

#endif // HATNCONTEXTCONTEXTLOGGER_H
