/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file serverapp/auth/authservice.h
  */

/****************************************************************************/

#ifndef HATNSERVERAUTHSERVICE_H
#define HATNSERVERAUTHSERVICE_H

#include <hatn/api/server/serverservice.h>

#include <hatn/clientserver/auth/authprotocol.h>

#include <hatn/serverapp/serverappdefs.h>

HATN_SERVERAPP_NAMESPACE_BEGIN

/**************************** AuthNegotiateMethod ***************************/

template <typename RequestT>
class AuthNegotiateMethodImpl
{
    public:

        using Request=RequestT;
        using Message=auth_negotiate_request::managed;

        void exec(
            common::SharedPtr<serverapi::RequestContext<Request>> request,
            serverapi::RouteCb<Request> callback,
            common::SharedPtr<Message> msg
        ) const;

        HATN_VALIDATOR_NAMESPACE::error_report validate(
            const common::SharedPtr<serverapi::RequestContext<Request>> request,
            const Message& msg
        ) const;
};

template <typename RequestT>
class AuthNegotiateMethod : public serverapi::ServiceMethodV<serverapi::ServiceMethodT<AuthNegotiateMethodImpl<RequestT>>,RequestT>
{
    public:

        using Base=serverapi::ServiceMethodV<serverapi::ServiceMethodT<AuthNegotiateMethodImpl<RequestT>>,RequestT>;

        AuthNegotiateMethod() : Base(AuthNegotiateMethodName)
        {}
};

/***************************** AuthService ***********************************/

template <typename RequestT>
class AuthService : public serverapi::ServerServiceV<serverapi::ServiceMultipleMethods<RequestT>,RequestT>
{
    public:

        using Request=RequestT;
        using Base=serverapi::ServerServiceV<serverapi::ServiceMultipleMethods<Request>,Request>;

        AuthService();
};

HATN_SERVERAPP_NAMESPACE_END

#endif // HATNSERVERAUTHSERVICE_H
