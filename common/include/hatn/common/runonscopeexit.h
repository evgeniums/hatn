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
struct RunOnScopeExit final
{
    /**
     * @brief Constructor.
     * @param handler handler to run when object is destroyed.
     */
    RunOnScopeExit(std::function<void ()> handler, bool enable=true) noexcept
        : m_handler(std::move(handler)),
          m_enable(enable)
    {}

    //! Dtor
    ~RunOnScopeExit()
    {
        if (m_enable && m_handler)
        {
            m_handler();
        }
    }

    RunOnScopeExit(const RunOnScopeExit&)=delete;
    RunOnScopeExit(RunOnScopeExit&&) =delete;
    RunOnScopeExit& operator=(const RunOnScopeExit&)=delete;
    RunOnScopeExit& operator=(RunOnScopeExit&&) =delete;

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

        std::function<void ()> m_handler;
        bool m_enable;
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
#endif // HATNRUNONSCOPEEXIT_H
