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

        static Logger& init(LoggerHandlerTraits handler);

        static Logger& instance();

        static void free();

        ~ContextLogger();

    private:

        ContextLogger(LoggerHandlerTraits handler);

        Logger m_logger;

        Logger& logger() noexcept
        {
            return m_logger;
        }
};

HATN_LOGCONTEXT_NAMESPACE_END

#define HATN_CTX_EXPAND(x) x
#define HATN_CTX_GET_ARG3(arg1, arg2, arg3, ...) arg3
#define HATN_CTX_GET_ARG4(arg1, arg2, arg3, arg4, ...) arg4
#define HATN_CTX_GET_ARG5(arg1, arg2, arg3, arg4, arg5, ...) arg5

#define HATN_CTX_LOG_IF_1(Level,Module) \
    {HATN_LOG_MODULE(Module)* _{nullptr}; std::ignore=_;} \
    if (HATN_LOGCONTEXT_NAMESPACE::Logger::passLog( \
            HATN_LOGCONTEXT_NAMESPACE::ContextLogger::instance(), \
            Level, \
            HATN_COMMON_NAMESPACE::ThreadLocalContext<HATN_LOGCONTEXT_NAMESPACE::Context>::value(), \
            #Module ) \
    )

#define HATN_CTX_LOG_1(Level,Msg,Module) \
    HATN_CTX_LOG_IF_1(Level,Module) \
    { \
        HATN_LOGCONTEXT_NAMESPACE::ContextLogger::instance().log( \
            Level, \
            HATN_COMMON_NAMESPACE::ThreadLocalContext<HATN_LOGCONTEXT_NAMESPACE::Context>::value(), \
            Msg, \
            #Module \
        ); \
    }

#define HATN_CTX_LOG_ERR_1(Level,Err,Msg,Module) \
    HATN_CTX_LOG_IF_1(Level,Module) \
    { \
            HATN_LOGCONTEXT_NAMESPACE::ContextLogger::instance().logError( \
                      Level, \
                      Err, \
                      HATN_COMMON_NAMESPACE::ThreadLocalContext<HATN_LOGCONTEXT_NAMESPACE::Context>::value(), \
                      Msg, \
                      #Module \
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

#define HATN_CTX_CLOSE(Err,Msg) \
    HATN_CTX_LOG_IF_0(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Info) \
    { \
            HATN_LOGCONTEXT_NAMESPACE::ContextLogger::instance().logClose( \
                      HATN_LOGCONTEXT_NAMESPACE::LogLevel::Info, \
                      Err, \
                      HATN_COMMON_NAMESPACE::ThreadLocalContext<HATN_LOGCONTEXT_NAMESPACE::Context>::value(), \
                      Msg \
                    ); \
    }

#define HATN_CTX_CLOSE_API(Err,Msg) \
    HATN_CTX_LOG_IF_0(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Info) \
    { \
            HATN_LOGCONTEXT_NAMESPACE::ContextLogger::instance().logCloseApi( \
                           HATN_LOGCONTEXT_NAMESPACE::LogLevel::Info, \
                           Err, \
                           HATN_COMMON_NAMESPACE::ThreadLocalContext<HATN_LOGCONTEXT_NAMESPACE::Context>::value(), \
                           Msg \
                    ); \
    }

#define HATN_CTX_LOG_ERR_0(Level,Err,Msg) \
    HATN_CTX_LOG_IF_0(Level) \
    { \
            HATN_LOGCONTEXT_NAMESPACE::ContextLogger::instance().logError( \
                      Level, \
                      Err, \
                      HATN_COMMON_NAMESPACE::ThreadLocalContext<HATN_LOGCONTEXT_NAMESPACE::Context>::value(), \
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

#define HATN_CTX_INFO(...) HATN_CTX_LOG(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Info,__VA_ARGS__)
#define HATN_CTX_WARN(...) HATN_CTX_LOG(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Warn,__VA_ARGS__)
#define HATN_CTX_DEBUG(...) HATN_CTX_LOG(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Debug,__VA_ARGS__)
#define HATN_CTX_TRACE(...) HATN_CTX_LOG(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Trace,__VA_ARGS__)

#define HATN_CTX_FATAL(...) HATN_CTX_LOG_ERR(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Fatal,__VA_ARGS__)
#define HATN_CTX_ERROR(...) HATN_CTX_LOG_ERR(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Error,__VA_ARGS__)

#define HATN_CTX_INFO_IF(...) HATN_CTX_LOG_IF(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Info,__VA_ARGS__)
#define HATN_CTX_ERROR_IF(...) HATN_CTX_LOG_IF(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Error,__VA_ARGS__)
#define HATN_CTX_WARN_IF(...) HATN_CTX_LOG_IF(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Warn,__VA_ARGS__)
#define HATN_CTX_DEBUG_IF(...) HATN_CTX_LOG_IF(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Debug,__VA_ARGS__)
#define HATN_CTX_FATAL_IF(...) HATN_CTX_LOG_IF(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Fatal,__VA_ARGS__)
#define HATN_CTX_TRACE_IF(...) HATN_CTX_LOG_IF(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Trace,__VA_ARGS__)

#define HATN_CTX_LOG_RECORDS_M(Level,Msg,Module,...) \
    HATN_CTX_LOG_IF_1(Level,Module) \
    { \
            HATN_COMMON_NAMESPACE::pmr::vector<HATN_LOGCONTEXT_NAMESPACE::Record> recs{{__VA_ARGS__},\
                                    HATN_LOGCONTEXT_NAMESPACE::ContextAllocatorFactory::defaultFactory()->dataAllocator<HATN_LOGCONTEXT_NAMESPACE::Record>()}; \
            HATN_LOGCONTEXT_NAMESPACE::ContextLogger::instance().log( \
                      Level, \
                      HATN_COMMON_NAMESPACE::ThreadLocalContext<HATN_LOGCONTEXT_NAMESPACE::Context>::value(), \
                      Msg, \
                      #Module, \
                      std::move(recs) \
                    ); \
    }

#define HATN_CTX_LOG_RECORDS_ERR_M(Level,Err,Msg,Module,...) \
    HATN_CTX_LOG_IF_1(Level,Module) \
    { \
            HATN_COMMON_NAMESPACE::pmr::vector<HATN_LOGCONTEXT_NAMESPACE::Record> recs{{__VA_ARGS__},\
                                    HATN_LOGCONTEXT_NAMESPACE::ContextAllocatorFactory::defaultFactory()->dataAllocator<HATN_LOGCONTEXT_NAMESPACE::Record>()}; \
            HATN_LOGCONTEXT_NAMESPACE::ContextLogger::instance().logError( \
                      Level, \
                      Err, \
                      HATN_COMMON_NAMESPACE::ThreadLocalContext<HATN_LOGCONTEXT_NAMESPACE::Context>::value(), \
                      Msg, \
                      #Module, \
                      std::move(recs) \
                    ); \
    }

#define HATN_CTX_LOG_RECORDS(Level,Msg,...) \
    HATN_CTX_LOG_IF_0(Level) \
    { \
            HATN_COMMON_NAMESPACE::pmr::vector<HATN_LOGCONTEXT_NAMESPACE::Record> recs{{__VA_ARGS__},\
                                    HATN_LOGCONTEXT_NAMESPACE::ContextAllocatorFactory::defaultFactory()->dataAllocator<HATN_LOGCONTEXT_NAMESPACE::Record>()}; \
            HATN_LOGCONTEXT_NAMESPACE::ContextLogger::instance().log( \
              Level, \
              HATN_COMMON_NAMESPACE::ThreadLocalContext<HATN_LOGCONTEXT_NAMESPACE::Context>::value(), \
              Msg, \
              HATN_COMMON_NAMESPACE::lib::string_view{}, \
              std::move(recs) \
              ); \
    }

#define HATN_CTX_LOG_RECORDS_ERR(Level,Err,Msg,...) \
    HATN_CTX_LOG_IF_0(Level) \
    { \
            HATN_COMMON_NAMESPACE::pmr::vector<HATN_LOGCONTEXT_NAMESPACE::Record> recs{{__VA_ARGS__},\
                                    HATN_LOGCONTEXT_NAMESPACE::ContextAllocatorFactory::defaultFactory()->dataAllocator<HATN_LOGCONTEXT_NAMESPACE::Record>()}; \
            HATN_LOGCONTEXT_NAMESPACE::ContextLogger::instance().logError( \
                      Level, \
                      Err, \
                      HATN_COMMON_NAMESPACE::ThreadLocalContext<HATN_LOGCONTEXT_NAMESPACE::Context>::value(), \
                      Msg, \
                      HATN_COMMON_NAMESPACE::lib::string_view{}, \
                      std::move(recs) \
                    ); \
    }

#define HATN_CTX_INFO_RECORDS(Msg,...) HATN_CTX_LOG_RECORDS(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Info,Msg,__VA_ARGS__)
#define HATN_CTX_WARN_RECORDS(Msg,...) HATN_CTX_LOG_RECORDS(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Warn,Msg,__VA_ARGS__)
#define HATN_CTX_DEBUG_RECORDS(Msg,...) HATN_CTX_LOG_RECORDS(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Debug,Msg,__VA_ARGS__)
#define HATN_CTX_TRACE_RECORDS(Msg,...) HATN_CTX_LOG_RECORDS(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Trace,Msg,__VA_ARGS__)

#define HATN_CTX_ERROR_RECORDS(Err,Msg,...) HATN_CTX_LOG_RECORDS_ERR(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Error,Err,Msg,__VA_ARGS__)
#define HATN_CTX_FATAL_RECORDS(Err,Msg,...) HATN_CTX_LOG_RECORDS_ERR(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Fatal,Err,Msg,__VA_ARGS__)

#define HATN_CTX_INFO_RECORDS_M(Msg,Module,...) HATN_CTX_LOG_RECORDS_M(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Info,Msg,Module,__VA_ARGS__)
#define HATN_CTX_WARN_RECORDS_M(Msg,Module,...) HATN_CTX_LOG_RECORDS_M(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Warn,Msg,Module,__VA_ARGS__)
#define HATN_CTX_DEBUG_RECORDS_M(Msg,Module,...) HATN_CTX_LOG_RECORDS_M(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Debug,Msg,Module,__VA_ARGS__)
#define HATN_CTX_TRACE_RECORDS_M(Msg,Module,...) HATN_CTX_LOG_RECORDS_M(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Trace,Msg,Module,__VA_ARGS__)

#define HATN_CTX_ERROR_RECORDS_M(Err,Msg,Module,...) HATN_CTX_LOG_RECORDS_ERR_M(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Error,Err,Msg,Module,__VA_ARGS__)
#define HATN_CTX_FATAL_RECORDS_M(Err,Msg,Module,...) HATN_CTX_LOG_RECORDS_ERR_M(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Fatal,Err,Msg,Module,__VA_ARGS__)

#endif // HATNCONTEXTCONTEXTLOGGER_H
