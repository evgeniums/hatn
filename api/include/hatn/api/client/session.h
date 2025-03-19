/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/client/session.h
  *
  */

/****************************************************************************/

#ifndef HATNAPICLIENTSESSION_H
#define HATNAPICLIENTSESSION_H

#include <functional>

#include <hatn/common/objecttraits.h>
#include <hatn/common/sharedptr.h>
#include <hatn/common/error.h>
#include <hatn/common/taskcontext.h>
#include <hatn/common/flatmap.h>
#include <hatn/common/pmr/allocatorfactory.h>

#include <hatn/logcontext/context.h>

#include <hatn/dataunit/objectid.h>

#include <hatn/api/api.h>
#include <hatn/api/auth.h>
#include <hatn/api/responseunit.h>
#include <hatn/api/client/clientresponse.h>

HATN_API_NAMESPACE_BEGIN

namespace client {

/********************** SessionAuth **************************/

class SessionAuth : public Auth
{
public:

    template <typename UnitT>
    Error serializeAuthHeader(lib::string_view protocol, uint32_t protocolVersion, common::SharedPtr<UnitT> content,
                              const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
                              );
};

/********************** Session **************************/

using SessionId=du::ObjectId::String;

template <typename ContextT>
using SessionCb=std::function<void (common::SharedPtr<ContextT> ctx, const common::Error&)>;

template <typename Traits>
class Session : public common::WithTraits<Traits>,
                public common::pmr::WithFactory,
                public SessionAuth,
                public common::TaskSubcontext
{
    public:

        using RefreshCb=std::function<void (const Error& ec, Session* session)>;

        template <typename ...TraitsArgs>
        Session(TraitsArgs&& ...traitsArgs)
            : common::WithTraits<Traits>(this,std::forward<TraitsArgs>(traitsArgs)...),
              m_id(du::ObjectId::generateIdStr()),
              m_valid(false),
              m_refreshing(false)
        {
            init();
        }

        template <typename ...TraitsArgs>
        Session(
            lib::string_view id,
            const common::pmr::AllocatorFactory* allocatorFactory,
            TraitsArgs&& ...traitsArgs)
            : common::pmr::WithFactory(allocatorFactory),
              common::WithTraits<Traits>(this,std::forward<TraitsArgs>(traitsArgs)...),
              m_id(id),
              m_valid(false),
              m_refreshing(false)
        {
            init();
        }

        template <typename ...TraitsArgs>
        Session(
            const std::string& id,
            TraitsArgs&& ...traitsArgs)
            : common::WithTraits<Traits>(this,std::forward<TraitsArgs>(traitsArgs)...),
              m_id(id),
              m_valid(false),
              m_refreshing(false)
        {
            init();
        }

        using common::WithTraits<Traits>::WithTraits;

        void setId(lib::string_view id) noexcept
        {
            m_id=id;
        }

        lib::string_view id() const noexcept
        {
            return m_id;
        }

        bool isValid() const noexcept
        {
            return m_valid;
        }

        void setValid(bool enable) noexcept
        {
            m_valid=enable;
        }

        bool isRefreshing() const noexcept
        {
            return m_refreshing;
        }

        void setRefreshing(bool enable) noexcept
        {
            m_refreshing=enable;
        }

        void refresh(lib::string_view ctxId, RefreshCb callback, Response resp={})
        {
            m_callbacks[ctxId]=std::move(callback);

            if (isRefreshing())
            {
                return;
            }
            setRefreshing(true);

            //! @todo Maybe switch log context to session context
            auto sessionCtx=this->sharedMainCtx();
            this->traits().refresh(
                [sessionCtx{std::move(sessionCtx)},this](const Error& ec)
                {
                    setRefreshing(false);
                    setValid(!ec);
                    if (ec)
                    {
                        resetAuthHeader();
                    }

                    for (auto&& it: m_callbacks)
                    {
                        it.second(ec,this);
                    }
                    m_callbacks.clear();
                },
                std::move(resp)
            );
        }

    private:

        void init()
        {}

        SessionId m_id;
        bool m_valid;
        bool m_refreshing;
        std::map<common::TaskContextId,RefreshCb,std::less<>> m_callbacks;
};

/********************** SessionWrapper **************************/

template <typename SessionContextT, typename SessionT>
class SessionWrapper
{
    public:

        SessionWrapper(common::SharedPtr<SessionContextT> sessionCtx):m_sessionCtx(std::move(sessionCtx))
        {}

        const SessionT& session() const
        {
            return m_sessionCtx-> template get<SessionT>();
        }

        SessionT& session()
        {
            return m_sessionCtx-> template get<SessionT>();
        }

        void setId(lib::string_view id) noexcept
        {
            session().setId(id);
        }

        lib::string_view id() const noexcept
        {
            return session().id();
        }

        bool isValid() const noexcept
        {
            return session().isValid();
        }

        void setValid(bool enable) noexcept
        {
            session().setValid(enable);
        }

        bool isRefreshing() const noexcept
        {
            return session().isRefreshing();
        }

        void setRefreshing(bool enable) noexcept
        {
            session().setRefreshing(enable);
        }

        template <typename CallbackT>
        void refresh(lib::string_view ctxId, CallbackT callback, Response resp={})
        {
            session().refresh(ctxId,std::move(callback),std::move(resp));
        }

    private:

        common::SharedPtr<SessionContextT> m_sessionCtx;
};

/********************** SessionNoAuth **************************/

class SessionNoAuthTraits
{
    public:

        using SessionType=Session<SessionNoAuthTraits>;

        SessionNoAuthTraits(SessionType* session) : m_session(session)
        {
            session->setValid(true);
        }

        template <typename CallbackT>
        void refresh(lib::string_view, CallbackT callback, Response ={})
        {
            m_session->resetAuthHeader();
            callback(Error{});
            return;
        }

    private:

        SessionType* m_session;
};

using SessionNoAuth=Session<SessionNoAuthTraits>;

using SessionNoAuthContext=common::TaskContextType<SessionNoAuth,HATN_LOGCONTEXT_NAMESPACE::Context>;

struct allocateSessionNoAuthContextT
{
    template <typename ...Args>
    auto operator () (
        const HATN_COMMON_NAMESPACE::pmr::polymorphic_allocator<SessionNoAuthContext>& allocator,
        Args&&... args
        ) const
    {
        return SessionWrapper<SessionNoAuthContext,SessionNoAuth>{
                HATN_COMMON_NAMESPACE::allocateTaskContextType<SessionNoAuthContext>(
                    allocator,
                    HATN_COMMON_NAMESPACE::subcontexts(
                        HATN_COMMON_NAMESPACE::subcontext(std::forward<Args>(args)...),
                        HATN_COMMON_NAMESPACE::subcontext()
                        )
                )
            };
    }
};
constexpr allocateSessionNoAuthContextT allocateSessionNoAuthContext{};

struct makeSessionNoAuthContextT
{
    template <typename ...Args>
    auto operator () (
        Args&&... args
        ) const
    {
        return SessionWrapper<SessionNoAuthContext,SessionNoAuth>{
            HATN_COMMON_NAMESPACE::makeTaskContextType<SessionNoAuthContext>(
                HATN_COMMON_NAMESPACE::subcontexts(
                    HATN_COMMON_NAMESPACE::subcontext(std::forward<Args>(args)...),
                    HATN_COMMON_NAMESPACE::subcontext()
                    )
            )
        };
    }
};
constexpr makeSessionNoAuthContextT makeSessionNoAuthContext{};

} // namespace client

HATN_API_NAMESPACE_END

HATN_TASK_CONTEXT_DECLARE(HATN_API_NAMESPACE::client::SessionNoAuth)

#endif // HATNAPICLIENTSESSION_H
