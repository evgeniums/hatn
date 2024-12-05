/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file logcontext/contextlogger.сpp
  *
  *      Contains definition of context logger singleton.
  *
  */

#include <hatn/logcontext/contextlogger.h>

HATN_LOGCONTEXT_NAMESPACE_BEGIN

//---------------------------------------------------------------

HATN_SINGLETON_INIT(ContextLogger)

namespace {

std::shared_ptr<ContextLogger>& globalLogger()
{
    static std::shared_ptr<ContextLogger> inst;
    return inst;
}

}

//---------------------------------------------------------------

template class HATN_LOGCONTEXT_EXPORT LoggerHandlerT<Context>;

//---------------------------------------------------------------

ContextLogger::ContextLogger(LoggerHandlerTraits handler) : m_logger(std::move(handler))
{}

//---------------------------------------------------------------

ContextLogger::~ContextLogger()
{}

//---------------------------------------------------------------

Logger& ContextLogger::init(LoggerHandlerTraits handler)
{
    globalLogger().reset(new ContextLogger(std::move(handler)));
    return globalLogger()->logger();
}

//---------------------------------------------------------------

Logger& ContextLogger::instance()
{
    Assert(globalLogger(),"Global logger not initialized, call ContextLogger::init() first");
    return globalLogger()->logger();
}

//---------------------------------------------------------------

bool ContextLogger::available() noexcept
{
    return static_cast<bool>(globalLogger());
}

//---------------------------------------------------------------

void ContextLogger::free()
{
    globalLogger().reset();
}

//---------------------------------------------------------------

HATN_LOGCONTEXT_NAMESPACE_END
