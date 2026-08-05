/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/
/****************************************************************************/
/** @file common/terminatehandler.h
 *
 *     Terminate handler reporting diagnostic context of uncaught exceptions.
 *
 */
/****************************************************************************/

#ifndef HATNTERMINATEHANDLER_H
#define HATNTERMINATEHANDLER_H

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

/**
 * @brief Terminate handler reporting diagnostic context of uncaught exceptions.
 *
 * Uncaught exceptions must never be intercepted with catch blocks just for the sake of
 * logging. As soon as a matching catch block is found the stack is unwound down to that
 * block and every frame between the throw point and the block is destroyed, so an external
 * crash reporter collects only the few frames that are left instead of the actual call stack
 * of the exception. When no catch block matches, std::terminate() is invoked directly at the
 * throw point with the whole stack still intact, which is exactly what crash reporters need.
 *
 * Thus, instead of catching, the code marks the section currently executed in the thread and
 * this terminate handler reports that section together with the type and the description of
 * the exception being propagated.
 *
 * The handler invokes the terminate handler that was installed before it, so that terminate
 * handlers of crash reporters keep working. Note that the crash report itself is collected in
 * any case because abort() is called in the end of terminate handling chain.
 *
 * Installation is safe to repeat: install() reinstalls the handler if some other terminate
 * handler was set meanwhile, so it can be invoked both as early as possible and once again
 * after initialization of a crash reporter regardless of the actual order of initialization.
 *
 */
class HATN_COMMON_EXPORT TerminateHandler
{
    public:

        //! Install terminate handler chaining it to the currently installed handler if any.
        /**
         * Does nothing if this handler is already the currently installed one.
         */
        static void install() noexcept;

        //! Set name of the section currently executed in the current thread.
        /**
         * @param section Name of the section, must outlive the thread, nullptr to reset.
         *
         * @attention Prefer ThreadSectionGuard to invoking this method directly.
         */
        static void setSection(const char* section) noexcept;

        //! Get name of the section currently executed in the current thread.
        static const char* section() noexcept;
};

//! Scope guard setting name of the section currently executed in the current thread.
/**
 * @note If the section is left via propagation of an uncaught exception then the destructor
 * is not invoked at all because the stack is not unwound in that case, and that is precisely
 * why the terminate handler still sees the name of the failed section.
 */
class ThreadSectionGuard
{
    public:

        explicit ThreadSectionGuard(const char* section) noexcept
            : m_prevSection(TerminateHandler::section())
        {
            TerminateHandler::setSection(section);
        }

        ~ThreadSectionGuard()
        {
            TerminateHandler::setSection(m_prevSection);
        }

        ThreadSectionGuard(const ThreadSectionGuard&)=delete;
        ThreadSectionGuard(ThreadSectionGuard&&)=delete;
        ThreadSectionGuard& operator=(const ThreadSectionGuard&)=delete;
        ThreadSectionGuard& operator=(ThreadSectionGuard&&)=delete;

    private:

        const char* m_prevSection;
};

HATN_COMMON_NAMESPACE_END

#endif // HATNTERMINATEHANDLER_H
