/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/server/authdispatcher.h
  *
  */

/****************************************************************************/

#ifndef HATNAPIAUTHDISPATCHER_H
#define HATNAPIAUTHDISPATCHER_H

#include <hatn/api/api.h>
#include <hatn/api/apiliberror.h>
#include <hatn/api/server/servicedispatcher.h>

HATN_API_NAMESPACE_BEGIN

namespace server {

struct AuthDispatcherScopes
{
    constexpr static const char* dispatch="authdispatch";
    constexpr static const char* dispatchCb="authdispatch::cb";
};

class NoAuthProtocol
{
};

template <typename EnvT=BasicEnv, typename RequestT=Request<EnvT>, typename ...AuthProtocols>
class AuthDispatcherT : public common::Env<AuthProtocols...>
{
    public:

        using Request=RequestT;

        using common::Env<AuthProtocols...>::Env;

        void dispatch(
                common::SharedPtr<RequestContext<Request>> reqCtx,
                DispatchCb<Request> callback
            )
        {
            auto& req=request<Request>(reqCtx).request();

            // check if auth can be skipped for service
            auto service=req.serviceNameAndVersion();
            auto it=m_skipAuth.find(service);
            if (it!=m_skipAuth.end())
            {
                callback(std::move(reqCtx));
                return;
            }

            // check if auth field is set
            const auto& authField=req.authField();
            if (!authField.isSet() || req.authMessage().isNull())
            {
                req.response.setStatus(protocol::ResponseStatus::AuthError,apiLibError(ApiLibError::AUTH_MISSING));
                callback(std::move(reqCtx));
                return;
            }

            // invoke auth protocol
            auto selector=[&authField](const auto& authProto)
            {
                return authField.field(auth_protocol::protocol).value()==authProto.name() && authField.field(auth_protocol::version).value()==authProto.version();
            };
            auto visitor=[reqCtx,&req,callback](auto& authProto)
            {
                authProto.invoke(std::move(reqCtx),std::move(callback),req.authMessage());
                return true;
            };
            auto found=this->visitIf(visitor,selector);
            if (!found)
            {
                req.response.setStatus(protocol::ResponseStatus::AuthError,apiLibError(ApiLibError::AUTH_PROTOCOL_UNKNOWN));
                callback(std::move(reqCtx));
            }
        }

        void addSkipAuthService(Service service)
        {
            m_skipAuth.insert(std::move(service));
        }

    private:

        common::FlatSet<Service> m_skipAuth;
};

template <typename EnvT, typename RequestT>
class AuthDispatcherT<EnvT,RequestT,NoAuthProtocol>
{
    public:

        void dispatch(
                common::SharedPtr<RequestContext<RequestT>> reqCtx,
                DispatchCb<RequestT> callback
            )
        {
            callback(std::move(reqCtx));
        }

        void addSkipAuthService(Service)
        {}
};

template <typename EnvT=BasicEnv, typename RequestT=Request<EnvT>, typename ...AuthProtocols>
class AuthDispatcher : public Dispatcher<AuthDispatcherT<EnvT,RequestT,AuthProtocols...>,EnvT,RequestT,AuthDispatcherScopes>
{
    public:

        using Base=Dispatcher<AuthDispatcherT<EnvT,RequestT,AuthProtocols...>,EnvT,RequestT,AuthDispatcherScopes>;

        using Base::Base;

        void addSkipAuthService(Service service)
        {
            this->traits().addSkipAuthService(std::move(service));
        }
};

template <typename EnvT, typename RequestT>
class AuthDispatcher<EnvT,RequestT> : public AuthDispatcher<EnvT,RequestT,NoAuthProtocol>
{
    public:

        using AuthDispatcher<EnvT,RequestT,NoAuthProtocol>::AuthDispatcher;
};

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPIAUTHDISPATCHER_H
