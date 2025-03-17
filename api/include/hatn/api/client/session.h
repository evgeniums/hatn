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

#include <hatn/dataunit/objectid.h>

#include <hatn/api/api.h>
#include <hatn/api/auth.h>
#include <hatn/api/responseunit.h>
#include <hatn/api/client/clientresponse.h>

HATN_API_NAMESPACE_BEGIN

namespace client {

class SessionAuth : public Auth
{
public:

    template <typename UnitT>
    Error serializeAuthHeader(lib::string_view protocol, uint32_t protocolVersion, common::SharedPtr<UnitT> content,
                              const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
                              );
};

using SessionId=du::ObjectId::String;

template <typename ContextT>
using SessionCb=std::function<void (common::SharedPtr<ContextT> ctx, const common::Error&)>;

template <typename Traits>
class Session : public common::WithTraits<Traits>,
                public common::TaskSubcontext,
                public SessionAuth
{
    public:

        using RefreshCb=std::function<void (const Error& ec, Session* session)>;

        template <typename ...TraitsArgs>
        Session(TraitsArgs&& ...traitsArgs)
            : common::WithTraits<Traits>(this,std::forward<TraitsArgs>(traitsArgs)...),
              m_allocatorFactory(common::pmr::AllocatorFactory::getDefault()),
              m_id(du::ObjectId::generateIdStr()),
              m_valid(false)
        {
            init();
        }

        template <typename ...TraitsArgs>
        Session(
            lib::string_view id,
            TraitsArgs&& ...traitsArgs)
            : common::WithTraits<Traits>(this,std::forward<TraitsArgs>(traitsArgs)...),
              m_allocatorFactory(common::pmr::AllocatorFactory::getDefault()),
              m_id(id),
              m_valid(false)
        {
            init();
        }

        template <typename ...TraitsArgs>
        Session(
            const std::string& id,
            TraitsArgs&& ...traitsArgs)
            : common::WithTraits<Traits>(this,std::forward<TraitsArgs>(traitsArgs)...),
              m_allocatorFactory(common::pmr::AllocatorFactory::getDefault()),
              m_id(id),
              m_valid(false)
        {
            init();
        }

        using common::WithTraits<Traits>::WithTraits;

        void setAllocatroFactory(const common::pmr::AllocatorFactory* factory) noexcept
        {
            m_allocatorFactory=factory;
        }

        const common::pmr::AllocatorFactory* allocatorFactory() const noexcept
        {
            return m_allocatorFactory;
        }

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
        {
            m_callbacks.reserve(DefaultSessionCallbacksCapacity);
        }

        const common::pmr::AllocatorFactory* m_allocatorFactory;

        SessionId m_id;
        bool m_valid;
        bool m_refreshing;
        std::map<common::TaskContextId,RefreshCb,std::less<>> m_callbacks;        
};

class SessionNoAuthTraits
{
    public:

        using SessionType=Session<SessionNoAuthTraits>;
        using RefreshCb=std::function<void (const Error& ec, SessionType* session)>;

        SessionNoAuthTraits(SessionType* session) : m_session(session)
        {}

        void refresh(lib::string_view, RefreshCb callback, Response ={})
        {
            m_session->resetAuthHeader();
            callback(Error{},m_session);
            return;
        }

    private:

        SessionType* m_session;
};

using SessionNoAuth=Session<SessionNoAuthTraits>;

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPICLIENTSESSION_H
