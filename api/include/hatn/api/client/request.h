/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/client/request.h
  *
  */

/****************************************************************************/

#ifndef HATNAPICLIENTREQUEST_H
#define HATNAPICLIENTREQUEST_H

#include <hatn/dataunit/unitwrapper.h>

#include <hatn/api/api.h>
#include <hatn/api/priority.h>
#include <hatn/api/service.h>
#include <hatn/api/method.h>
#include <hatn/api/requestunit.h>
#include <hatn/api/client/session.h>

HATN_API_NAMESPACE_BEGIN

namespace client {

template <typename ContextT>
using RequestCb=std::function<void (common::SharedPtr<ContextT> ctx,const Error& ec,du::UnitWrapper result)>;

template <typename SessionTraits, typename ContextT, typename RequestUnitT=request::shared_managed>
struct Request
{
    public:

        Request(
                common::SharedPtr<Session<SessionTraits>> session,
                Priority priority=Priority::Normal,
                uint32_t timeoutMs=0
            ) : m_session(std::move(session)),
                m_priority(priority),
                m_timeoutMs(timeoutMs),
                m_pending(true),
                m_cancelled(false)
        {
        }

        Request(
                Priority priority=Priority::Normal,
                uint32_t timeoutMs=0
            ) : m_priority(priority),
                m_timeoutMs(timeoutMs),
                m_pending(true)
        {
        }

        void setPriority(Priority priority) noexcept
        {
            m_priority=priority;
        }

        Priority priority() const noexcept
        {
            return m_priority;
        }

        void setTimeout(uint32_t timeoutMs) noexcept
        {
            m_timeoutMs=timeoutMs;
        }

        uint32_t timeoutMs() const noexcept
        {
            return m_timeoutMs;
        }

        bool pending() const noexcept
        {
            return m_pending;
        }

        bool cancel() noexcept
        {
            if (m_pending)
            {
                return false;
            }
            m_cancelled=true;
            return true;
        }

        bool cancelled() const noexcept
        {
            return m_cancelled;
        }

    private:

        void setCtx(common::SharedPtr<ContextT> ctx)
        {
            m_ctx=std::move(ctx);
        }

        template <typename UnitT>
        Error makeUnit(
                const Service& service,
                const Method& method,
                const UnitT& content,
                lib::string_view topic={}
            );

        void updateSession(std::function<void (const Error&)> cb);

        void regenId();

        bool setNotPending() noexcept
        {
            m_pending=false;
        }

        lib::string_view id() const noexcept
        {
            return m_unit->fieldValue(request::id);
        }

        common::SharedPtr<ContextT> m_ctx;
        common::SharedPtr<Session<SessionTraits>> m_session;
        common::SharedPtr<RequestUnitT> m_unit;

        Priority m_priority;
        uint32_t m_timeoutMs;

        bool m_pending;
        bool m_cancelled;

        template <typename ...T>
        friend class Client;
};

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPICLIENTREQUEST_H
