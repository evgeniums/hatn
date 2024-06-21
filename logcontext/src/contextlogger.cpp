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

ContextLogger::ContextLogger(LogHandler handler) : m_logger(std::move(handler))
{}

//---------------------------------------------------------------

ContextLogger::~ContextLogger()
{}

//---------------------------------------------------------------

Logger& ContextLogger::init(LogHandler handler)
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

void ContextLogger::free()
{
    globalLogger().reset();
}

//---------------------------------------------------------------

HATN_LOGCONTEXT_NAMESPACE_END
