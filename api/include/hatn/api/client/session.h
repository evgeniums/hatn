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

class Service;
class Method;

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

using SessionRefreshCb=std::function<void (common::TaskContextShared,const Error&)>;

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

        SessionId m_id;
        bool m_valid;
        bool m_refreshing;
        std::vector<RefreshCb> m_callbacks;
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
