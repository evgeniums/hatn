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

        Logger& logger() noexcept
        {
            return m_logger;
        }

        ~ContextLogger();

    private:

        ContextLogger(LogHandler handler);

        Logger m_logger;
};

HATN_LOGCONTEXT_NAMESPACE_END

#if 0
#define HATN_CTX_EXPAND(x) x
#define HDU_CTX_GET_ARG4(arg1, arg2, arg3, arg4, ...) arg4

#define HATN_CTX_LOG_MODULE(Level,Ctx,Msg,Module) \


#define HATN_CTX_LOG(...) \
        HATN_CTX_EXPAND(HDU_CTX_GET_ARG4(__VA_ARGS__, \
                              HDU_V2_FIELD_DEF_DEFAULT, \
                              HDU_V2_FIELD_DEF_REQUIRED, \
                              HDU_V2_FIELD_DEF_OPTIONAL \
                              ))

#define HDU_V2_FIELD(...) HDU_V2_EXPAND(HDU_V2_VARG_SELECT_FIELD(__VA_ARGS__)(__VA_ARGS__,Auto))


#define HATN_CTX_LOG(Level,Ctx,Msg,Module)
#endif

#endif // HATNCONTEXTCONTEXTLOGGER_H
