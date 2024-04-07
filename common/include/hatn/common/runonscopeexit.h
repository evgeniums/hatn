/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/runonscopeexit.h
  *
  *     Guard to run handler on exit from a scope.
  *
  */

/****************************************************************************/

#ifndef HATNRUNONSCOPEEXIT_H
#define HATNRUNONSCOPEEXIT_H

#include <functional>

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

//! Guard running the handler on scope leaving.
template <typename Fn>
class RunOnScopeExitT
{
    public:

        /**
         * @brief Constructor.
         * @param handler handler to run when object is destroyed.
         */
        RunOnScopeExitT(Fn&& handler, bool enable=true) noexcept
            : m_handler(std::move(handler)),
              m_enable(enable)
        {}

        //! Dtor
        ~RunOnScopeExitT()
        {
            if (m_enable)
            {
                m_handler();
            }
        }

        RunOnScopeExitT(const RunOnScopeExitT&)=delete;
        RunOnScopeExitT(RunOnScopeExitT&&) =delete;
        RunOnScopeExitT& operator=(const RunOnScopeExitT&)=delete;
        RunOnScopeExitT& operator=(RunOnScopeExitT&&) =delete;

        //! Enable handler
        inline void setEnable(bool enable) noexcept
        {
            m_enable=enable;
        }

        //! Check if handler is enabled
        inline bool isEnabled() const noexcept
        {
            return m_enable;
        }

    private:

        Fn m_handler;
        bool m_enable;
};

template <typename Fn>
auto makeScopeGuard(Fn&& fn, bool enable=true)
{
    return RunOnScopeExitT<std::decay_t<Fn>>(std::forward<Fn>(fn),enable);
}

using RunOnScopeExit=RunOnScopeExitT<std::function<void ()>>;

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END

#define HATN_SCOPE_GUARD(fn) \
    auto _onExit=fn;\
    decltype(_onExit) _scopeGuard{HATN_COMMON_NAMESPACE::makeScopeGuard(std::move(_onExit))};

#endif // HATNRUNONSCOPEEXIT_H
