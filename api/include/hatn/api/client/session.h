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
#include <mutex>

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

class Service;
class Method;

namespace client {

/********************** SessionAuth **************************/

class SessionAuth : public Auth
{
    public:

        template <typename UnitT>
        Error serializeAuthHeader(
            lib::string_view protocol,
            uint32_t protocolVersion,
            common::SharedPtr<UnitT> content,
            const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
        );

        common::ByteArrayShared sessionToken() const
        {
            return m_sessionToken;
        }

        const std::string& tokenTag() const
        {
            return m_tokenTag;
        }

        void setSessionToken(common::ByteArrayShared token, std::string tag={})
        {
            m_sessionToken=std::move(token);
            m_tokenTag=std::move(tag);
        }

        void resetSessionToken()
        {
            m_sessionToken.reset();
            m_tokenTag.clear();
        }

        bool isSerializedHeaderNeeded() const
        {
            return m_serializedHeaderNeeded;
        }

        void setSerializedHeaderNeeded(bool enable)
        {
            m_serializedHeaderNeeded=enable;
        }

        bool isAuthHeaderValid() const noexcept
        {
            return Auth::isAuthHeaderValid() || !m_sessionToken.isNull();
        }

        void resetAuthHeader()
        {
            Auth::resetAuthHeader();
            resetSessionToken();
        }

    private:

        common::ByteArrayShared m_sessionToken;
        std::string m_tokenTag;

        bool m_serializedHeaderNeeded=true;
};

/********************** Session **************************/

using SessionId=du::ObjectId::String;

using SessionRefreshCb=std::function<void (common::TaskContextShared,const Error&)>;

//! Invoked once, on the client thread, when a session refresh fails with a terminal ApiError and the
//! session latches that failure (see Session::isAuthFailed()).
using SessionAuthFailedCb=std::function<void (const Error&)>;

template <typename Traits, typename NoAuthT=hana::false_>
class Session : public common::WithTraits<Traits>,
                public common::pmr::WithFactory,
                public SessionAuth,
                public common::TaskSubcontext
{
    public:

        constexpr static const bool NoAuth=NoAuthT::value;
        using RefreshCb=SessionRefreshCb;

        Session();

        template <typename ...TraitsArgs>
        Session(TraitsArgs&& ...traitsArgs);

        template <typename ...TraitsArgs>
        Session(
            lib::string_view id,
            const common::pmr::AllocatorFactory* allocatorFactory,
            TraitsArgs&& ...traitsArgs);

        template <typename ...TraitsArgs>
        Session(
            const std::string& id,
            TraitsArgs&& ...traitsArgs);

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
            return m_valid.load();
        }

        void setValid(bool enable) noexcept
        {
            m_valid.store(enable);
        }

        bool isRefreshing() const noexcept
        {
            return m_refreshing.load();
        }

        void setRefreshing(bool enable) noexcept
        {
            m_refreshing.store(enable);
        }

        /**
         * @brief Check if a refresh failed with a terminal error that retrying cannot fix.
         *
         * A refresh whose error is an ApiError with a Permanent/Unsupported disposition (e.g. the
         * login was revoked on the server) latches here. While latched, refresh() fails at once
         * with authFailure() and sends nothing to the network, so no component can start an
         * endless negotiate loop by issuing requests. Only resetAuthFailure() clears the latch.
         *
         * Safe to call from any thread.
         */
        bool isAuthFailed() const noexcept
        {
            return m_authFailed.load();
        }

        //! The error that caused the latch, empty if not latched. Safe to call from any thread.
        Error authFailure() const
        {
            std::lock_guard<std::mutex> l{m_authFailureMutex};
            return m_authFailure;
        }

        //! Clear the latch so that the next request starts a new auth negotiation. This is meant
        //! for an explicit user action.
        void resetAuthFailure()
        {
            std::lock_guard<std::mutex> l{m_authFailureMutex};
            m_authFailure=Error{};
            m_authFailed.store(false);
        }

        //! Set callback invoked once per latch. Call before the session is used.
        void setAuthFailedCb(SessionAuthFailedCb cb)
        {
            std::lock_guard<std::mutex> l{m_authFailureMutex};
            m_authFailedCb=std::move(cb);
        }

        template <typename ContextT, typename ClientT>
        void refresh(common::SharedPtr<ContextT> ctx, RefreshCb callback, ClientT* client, Response resp={});

        Error loadLogConfig(
            const HATN_BASE_NAMESPACE::ConfigTree& configTree,
            const std::string& configPath,
            HATN_BASE_NAMESPACE::config_object::LogRecords& records,
            const HATN_BASE_NAMESPACE::config_object::LogSettings& settings={}
        )
        {
            return this->traits().loadLogConfig(configTree,configPath,records,settings);
        }

        auto& sessionImpl()
        {
            return this->traits();
        }

        const auto& sessionImpl() const
        {
            return this->traits();
        }

    private:

        //! Latch the failure if it is terminal. Returns the callback to invoke, if any.
        SessionAuthFailedCb latchAuthFailure(const Error& ec);

        SessionId m_id;
        std::atomic<bool> m_valid;
        std::atomic<bool> m_refreshing;
        std::vector<RefreshCb> m_callbacks;

        std::atomic<bool> m_authFailed{false};
        mutable std::mutex m_authFailureMutex;
        Error m_authFailure;
        SessionAuthFailedCb m_authFailedCb;
};

/********************** SessionWrapper **************************/

template <typename SessionWrapperT>
class SessionWrapperImpl : public SessionWrapperT
{
    public:

        using SessionWrapperT::SessionWrapperT;

        void setId(lib::string_view id) noexcept
        {
            this->session().setId(id);
        }

        lib::string_view id() const noexcept
        {
            return this->session().id();
        }

        bool isValid() const noexcept
        {
            return this->session().isValid();
        }

        void setValid(bool enable) noexcept
        {
            this->session().setValid(enable);
        }

        bool isRefreshing() const noexcept
        {
            return this->session().isRefreshing();
        }

        void setRefreshing(bool enable) noexcept
        {
            this->session().setRefreshing(enable);
        }

        bool isAuthFailed() const noexcept
        {
            return this->session().isAuthFailed();
        }

        Error authFailure() const
        {
            return this->session().authFailure();
        }

        void resetAuthFailure()
        {
            this->session().resetAuthFailure();
        }

        void setAuthFailedCb(SessionAuthFailedCb cb)
        {
            this->session().setAuthFailedCb(std::move(cb));
        }

        template <typename ContextT, typename CallbackT, typename ClientT>
        void refresh(common::SharedPtr<ContextT> ctx, CallbackT callback, ClientT* client, Response resp={})
        {
            this->session().refresh(std::move(ctx),std::move(callback),client,std::move(resp));
        }

        template <typename UnitT>
        Error serializeAuthHeader(lib::string_view protocol, uint32_t protocolVersion, common::SharedPtr<UnitT> content,
                                  const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
                                  )
        {
            return this->session().serializeAuthHeader(protocol,protocolVersion,std::move(content),factory);
        }

        auto authHeader() const
        {
            return this->session().authHeader();
        }

        void resetAuthHeader()
        {
            this->session().resetAuthHeader();
        }

        common::ByteArrayShared sessionToken() const
        {
            return this->session().sessionToken();
        }

        const std::string& tokenTag() const
        {
            return this->session().tokenTag();
        }

        bool isSerializedHeaderNeeded() const
        {
            return this->session().isSerializedHeaderNeeded();
        }

        void setSerializedHeaderNeeded(bool enable)
        {
            this->session().setSerializedHeaderNeeded(enable);
        }
};

template <typename SessionT,typename SessionContextT=void>
class SessionWrapperT
{
    public:

        SessionWrapperT(common::SharedPtr<SessionContextT> sessionCtx={}):m_sessionCtx(std::move(sessionCtx))
        {}

        void setSession(common::SharedPtr<SessionContextT> sessionCtx) noexcept
        {
            m_sessionCtx=std::move(sessionCtx);
        }

        const SessionT& session() const
        {
            return m_sessionCtx->template get<SessionT>();
        }

        SessionT& session()
        {
            return m_sessionCtx->template get<SessionT>();
        }

        bool isNull() const noexcept
        {
            return m_sessionCtx.isNull();
        }

    private:

        common::SharedPtr<SessionContextT> m_sessionCtx;
};

template <typename SessionT>
class SessionWrapperT<SessionT>
{
    public:

        SessionWrapperT(SessionT* session=nullptr):m_session(session)
        {}

        void setSession(SessionT* session) noexcept
        {
            m_session=session;
        }

        const SessionT& session() const
        {
            return *m_session;
        }

        SessionT& session()
        {
            return *m_session;
        }

        bool isNull() const noexcept
        {
            return m_session==nullptr;
        }

    private:

        SessionT* m_session;
};

template <typename SessionT,typename SessionContextT=void>
class SessionWrapper : public SessionWrapperImpl<SessionWrapperT<SessionT,SessionContextT>>
{
    public:

        using SessionWrapperImpl<SessionWrapperT<SessionT,SessionContextT>>::SessionWrapperImpl;
};

template <typename SessionT>
class SessionWrapper<SessionT> : public SessionWrapperImpl<SessionWrapperT<SessionT>>
{
    public:

        using SessionWrapperImpl<SessionWrapperT<SessionT>>::SessionWrapperImpl;
};

/********************** SessionNoAuth **************************/

class SessionNoAuthTraits
{
    public:

        using SessionType=Session<SessionNoAuthTraits,hana::true_>;

        SessionNoAuthTraits(SessionType* session) : m_session(session)
        {
            session->setValid(true);
        }

        template <typename ContextT, typename CallbackT, typename ClientT>
        void refresh(common::SharedPtr<ContextT> ctx, CallbackT callback, ClientT*, Response ={})
        {
            m_session->resetAuthHeader();
            callback(std::move(ctx),Error{});
            return;
        }

    private:

        SessionType* m_session;
};

using SessionNoAuth=Session<SessionNoAuthTraits,hana::true_>;

using SessionNoAuthContext=common::TaskContextType<SessionNoAuth,HATN_LOGCONTEXT_NAMESPACE::Context>;

struct allocateSessionNoAuthContextT
{
    template <typename ...Args>
    auto operator () (
        const HATN_COMMON_NAMESPACE::pmr::polymorphic_allocator<SessionNoAuthContext>& allocator,
        Args&&... args
        ) const
    {
        return SessionWrapper<SessionNoAuth,SessionNoAuthContext>{
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
        return SessionWrapper<SessionNoAuth,SessionNoAuthContext>{
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
