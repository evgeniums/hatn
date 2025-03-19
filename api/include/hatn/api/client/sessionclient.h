/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/client/sessionclient.h
  *
  */

/****************************************************************************/

#ifndef HATNAPISESSIONCLIENT_H
#define HATNAPISESSIONCLIENT_H

#include <hatn/common/simplequeue.h>
#include <hatn/common/flatmap.h>
#include <hatn/common/pmr/allocatorfactory.h>
#include <hatn/common/taskcontext.h>

#include <hatn/base/configobject.h>

#include <hatn/logcontext/context.h>

#include <hatn/dataunit/unitwrapper.h>
#include <hatn/dataunit/syntax.h>

#include <hatn/api/api.h>
#include <hatn/api/connectionpool.h>
#include <hatn/api/method.h>
#include <hatn/api/service.h>
#include <hatn/api/priority.h>
#include <hatn/api/client/session.h>
#include <hatn/api/client/clientrequest.h>
#include <hatn/api/client/session.h>

HATN_API_NAMESPACE_BEGIN

namespace client {

template <typename SessionWrapperT, typename ClientContextT, typename ClientT>
class SessionClient : public common::TaskSubcontext
{
    public:

        using SessionWrapper=SessionWrapperT;

        using Client=ClientT;
        using ClientContext=ClientContextT;
        using Context=typename Client::Context;
        using MessageType=typename Client::MessageType;
        using ReqCtx=typename Client::ReqCtx;

        SessionClient(
                SessionWrapperT session,
                common::SharedPtr<ClientContext> clientCtx
            ) : m_session(std::move(session)),
                m_client(nullptr)
        {
            setClient(clientCtx);
        }

        SessionClient(
                SessionWrapper session
            ) : m_session(std::move(session)),
                m_client(nullptr)
        {}

        SessionClient() : m_client(nullptr)
        {}

        Error exec(
            common::SharedPtr<Context> ctx,
            const Service& service,
            const Method& method,
            MessageType message,
            RequestCb<Context> callback,
            lib::string_view topic={},
            Priority priority=Priority::Normal,
            uint32_t timeoutMs=0,
            MethodAuth methodAuth={}
        )
        {
            return m_client->exec(std::move(ctx),m_session,service,method,std::move(message),std::move(callback),topic,priority,timeoutMs,std::move(methodAuth));
        }

        common::Result<common::SharedPtr<ReqCtx>> prepare(
            const common::SharedPtr<Context>& ctx,
            const Service& service,
            const Method& method,
            MessageType message,
            lib::string_view topic={},
            MethodAuth methodAuth={}
        )
        {
            return m_client->prepare(std::move(ctx),m_session,service,method,std::move(message),topic,std::move(methodAuth));
        }

        void exec(
            common::SharedPtr<Context> ctx,
            common::SharedPtr<ReqCtx> req,
            RequestCb<Context> callback
        )
        {
            return m_client->exec(std::move(ctx),std::move(req),std::move(callback));
        }

        Error cancel(
            common::SharedPtr<ReqCtx>& req
        )
        {
            return m_client->cancel(req);
        }

        void setClient(common::SharedPtr<ClientContext> clientCtx)
        {
            m_clientCtx=std::move(clientCtx);
            if (m_clientCtx)
            {
                m_client=&m_clientCtx->template get<Client>();
            }
            else
            {
                m_client=nullptr;
            }
        }

        auto clientCtx() const
        {
            return m_clientCtx;
        }

        void setSession(SessionWrapper session)
        {
            m_session=std::move(session);
        }

        auto session() const
        {
            return m_session;
        }

    private:

        SessionWrapper m_session;
        common::SharedPtr<ClientContext> m_clientCtx;
        Client* m_client;
};

template <typename SessionT, typename ClientContextT, typename ClientT>
using SessionClientContext=common::TaskContextType<SessionClient<SessionT,ClientContextT,ClientT>,HATN_LOGCONTEXT_NAMESPACE::Context>;

template <typename T>
struct allocateSessionClientContextT
{
    template <typename ...Args>
    auto operator () (
            const HATN_COMMON_NAMESPACE::pmr::polymorphic_allocator<T>& allocator,
            Args&&... args
        ) const
    {
        return HATN_COMMON_NAMESPACE::allocateTaskContextType<T>(
            allocator,
            HATN_COMMON_NAMESPACE::subcontexts(
                    HATN_COMMON_NAMESPACE::subcontext(std::forward<Args>(args)...),
                    HATN_COMMON_NAMESPACE::subcontext()
                )
            );
    }
};
template <typename T>
constexpr allocateSessionClientContextT<T> allocateSessionClientContext{};

template <typename T>
struct makeSessionClientContextT
{
    template <typename ...Args>
    auto operator () (
            Args&&... args
        ) const
    {
        return HATN_COMMON_NAMESPACE::makeTaskContextType<T>(
            HATN_COMMON_NAMESPACE::subcontexts(
                HATN_COMMON_NAMESPACE::subcontext(std::forward<Args>(args)...),
                HATN_COMMON_NAMESPACE::subcontext()
                )
            );
    }
};
template <typename T>
constexpr makeSessionClientContextT<T> makeSessionClientContext{};

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPISESSIONCLIENT_H
