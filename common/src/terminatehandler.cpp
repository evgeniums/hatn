/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/
/****************************************************************************/
/** @file common/terminatehandler.cpp
  *
  *     Terminate handler reporting diagnostic context of uncaught exceptions.
  *
  */
/****************************************************************************/

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <typeinfo>

#include <hatn/common/logger.h>
#include <hatn/common/thread.h>
#include <hatn/common/terminatehandler.h>

#include <hatn/common/loggermoduleimp.h>

DECLARE_LOG_MODULE(terminate)

HATN_COMMON_NAMESPACE_BEGIN

namespace {

thread_local const char* CurrentSection=nullptr;

std::atomic<bool> InstallLock{false};
std::atomic<bool> Handling{false};
std::terminate_handler PrevHandler=nullptr;

void reportException()
{
    const char* section=(CurrentSection!=nullptr)?CurrentSection:"unknown section";
    const char* threadId=Thread::currentThreadID();

    auto ex=std::current_exception();
    if (!ex)
    {
        HATN_FATAL(terminate,"Terminate called without active exception in thread " << threadId
                             << " in " << section);
        return;
    }

    // rethrow only to figure out type and description of the exception, the stack below the
    // terminate handler is not affected because it was not unwound
    try
    {
        std::rethrow_exception(ex);
    }
    catch (const std::exception& e)
    {
        HATN_FATAL(terminate,"Uncaught exception in thread " << threadId << " in " << section
                             << " [" << typeid(e).name() << "]: " << e.what());
    }
    catch (...)
    {
        HATN_FATAL(terminate,"Uncaught non-standard exception in thread " << threadId
                             << " in " << section);
    }
}

void handleTerminate()
{
    bool expected=false;
    if (!Handling.compare_exchange_strong(expected,true))
    {
        // recursive or concurrent terminate, do not risk hanging in the logger
        std::abort();
    }

    try
    {
        reportException();
    }
    catch (...)
    {
        std::fputs("Failed to report uncaught exception\n",stderr);
    }

    // hand over to the terminate handler of a crash reporter if any, the stack of the original
    // exception is still intact at this point
    if (PrevHandler!=nullptr && PrevHandler!=&handleTerminate)
    {
        PrevHandler();
    }

    std::abort();
}

} // anonymous namespace

//---------------------------------------------------------------
void TerminateHandler::install() noexcept
{
    // spinlock, this method is invoked only from initialization paths
    bool expected=false;
    while (!InstallLock.compare_exchange_weak(expected,true))
    {
        expected=false;
    }

    // Reinstall if some other terminate handler was set after the previous installation, e.g.
    // by a crash reporter initialized later. This keeps handlers chained in the order of
    // installation no matter when the crash reporter is initialized.
    if (std::get_terminate()!=&handleTerminate)
    {
        PrevHandler=std::set_terminate(&handleTerminate);
    }

    InstallLock.store(false);
}

//---------------------------------------------------------------
void TerminateHandler::setSection(const char* section) noexcept
{
    CurrentSection=section;
}

//---------------------------------------------------------------
const char* TerminateHandler::section() noexcept
{
    return CurrentSection;
}

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
