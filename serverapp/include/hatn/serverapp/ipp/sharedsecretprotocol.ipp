/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file serverapp/ipp/sharedsecretprotocol.ipp
  */

/****************************************************************************/

#ifndef HATNSERVERSHAREDSECRETPROTOCOL_IPP
#define HATNSERVERSHAREDSECRETPROTOCOL_IPP

#include <hatn/common/meta/chain.h>

#include <hatn/crypt/cryptcontainer.h>

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/wirebufsolid.h>

#include <hatn/clientserver/clientservererror.h>
#include <hatn/clientserver/models/loginprofile.h>

#include <hatn/serverapp/auth/authtokens.h>
#include <hatn/serverapp/auth/sharedsecretprotocol.h>
#include <hatn/serverapp/sessioncontroller.h>

#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/objectid.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>

#include <hatn/serverapp/ipp/encryptedtoken.ipp>

HATN_SERVERAPP_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

template <typename ContextT, typename CallbackT>
void SharedSecretAuthProtocol::prepare(
        common::SharedPtr<ContextT> ctx,
        CallbackT callback,
        common::SharedPtr<auth_negotiate_request::managed> message,
        const common::pmr::AllocatorFactory* factory
    ) const
{
    auto msg=prepareChallengeToken(std::move(message),factory);
    if (msg)
    {
        //! @todo critical: Log error
        callback(std::move(ctx),msg.error(),{});
        return;
    }

    callback(std::move(ctx),{},msg.takeValue());
}

//--------------------------------------------------------------------------

template <typename ContextT, typename CallbackT, typename LoginControllerT>
void SharedSecretAuthProtocol::check(
        common::SharedPtr<ContextT> ctx,
        CallbackT callback,
        common::SharedPtr<auth_hss_check::managed> message,
        const LoginControllerT& loginController,
        const common::pmr::AllocatorFactory* factory
    ) const
{
    // deserialize token
    const auto& tokenField=message->field(auth_hss_check::token);
    const auto* tokenBuf=tokenField.buf();
    auto tokenR=parseToken<auth_challenge_token::managed>(*tokenBuf,factory);
    if (tokenR)
    {
        //! @todo critical: chain and log error
        auto ec=tokenR.takeError();
        callback(std::move(ctx),ec,{},{});
    }
    auto token=tokenR.takeValue();

    // check if token expired
    auto now=common::DateTime::currentUtc();
    if (now.after(token->fieldValue(auth_challenge_token::expire)))
    {
        callback(std::move(ctx),clientServerError(ClientServerError::AUTH_TOKEN_EXPIRED),std::move(token),{});
        return;
    }

    // find login
    auto findLogin=[loginController](auto&& checkMAC, auto ctx, auto callback, auto token)
    {
        auto tokenPtr=token.get();

        auto findCb=[token=std::move(token),callback=std::move(callback),checkMAC=std::move(checkMAC)](auto ctx, const Error& ec,
                                                                                                                 common::SharedPtr<login_profile::managed> login)
        {
            if (ec)
            {
                //! @todo critical: chain and log error
                callback(std::move(ctx),ec,std::move(token),{});
                return;
            }

            checkMAC(std::move(ctx),std::move(callback),std::move(token),std::move(login));
        };

        loginController.findLogin(
            std::move(ctx),
            std::move(findCb),
            tokenPtr->fieldValue(auth_challenge_token::login),
            tokenPtr->fieldValue(auth_challenge_token::topic)
        );
    };

    // check MAC
    auto checkMAC=[this,message=std::move(message)](auto ctx, auto callback, common::SharedPtr<auth_challenge_token::managed> token,
                           common::SharedPtr<login_profile::managed> login)
    {
        Error ec;

        // prepare MAC algorithm
        crypt::CryptAlgorithmConstP macAlg;
        ec=m_suite->macAlgorithm(macAlg);
        if (ec)
        {
            //! @todo critical: chain and log error
            callback(std::move(ctx),ec,std::move(token),{});
            return;
        }

        // prepare MAC key
        auto key=macAlg->createMACKey();
        ec=key->importFromBuf(login->fieldValue(login_profile::secret1),crypt::ContainerFormat::RAW_PLAIN);
        if (ec)
        {
            //! @todo critical: chain and log error
            callback(std::move(ctx),ec,std::move(token),{});
            return;
        }

        // check MAC
        auto mac=m_suite->createMAC(ec);
        if (ec)
        {
            //! @todo critical: chain and log error
            callback(std::move(ctx),ec,std::move(token),{});
            return;
        }
        ec=mac->check(macAlg,token->fieldValue(auth_challenge_token::challenge),message->fieldValue(auth_hss_check::mac));
        if (ec)
        {
            //! @todo critical: log error
            callback(std::move(ctx),clientServerError(ClientServerError::AUTH_FAILED),std::move(token),{});
            return;
        }

        // MAC is ok
        // note that blocking and ACL must be checked somewhere else
        callback(std::move(ctx),{},std::move(token),std::move(login));
    };

    auto chain=hatn::chain(
        std::move(findLogin),
        std::move(checkMAC)
    );
    chain(std::move(ctx),std::move(callback),std::move(token));
}

/**************************** AuthNegotiateMethod ***************************/

//--------------------------------------------------------------------------

template <typename RequestT>
void AuthHssCheckMethodImpl<RequestT>::exec(
    common::SharedPtr<serverapi::RequestContext<Request>> request,
    serverapi::RouteCb<Request> callback,
    common::SharedPtr<Message> msg
    ) const
{
    auto checkSharedSecret=[](auto&& createSession, auto request, auto callback, auto msg) mutable
    {
        auto cb=[createSession=std::move(createSession),callback=std::move(callback)](auto ctx, const Error& ec, common::SharedPtr<auth_challenge_token::managed> token, common::SharedPtr<login_profile::managed> login) mutable
        {
            auto& req=serverapi::request<Request>(ctx).request();
            //! @todo critical: chain and log error
            //! @todo check if it is internal error or invalid login data
            if (ec)
            {
                req.response.setStatus(api::protocol::ResponseStatus::AuthError,ec);
                callback(std::move(ctx));
                return;
            }
            else
            {
                createSession(std::move(ctx),std::move(callback),std::move(token),std::move(login));
            }
        };

        const auto& req=serverapi::request<Request>(request).request();
        const auto& loginController=req.env->loginController();
        const auto& hssProtocol=req.template envContext<WithSharedSecretAuthProtocol>();        

        hssProtocol->check(
            std::move(request),
            std::move(cb),
            std::move(msg),
            loginController,
            req.factory()
        );
    };

    auto createSession=[](auto request, auto callback, common::SharedPtr<auth_challenge_token::managed> token, common::SharedPtr<login_profile::managed> login)
    {
        auto cb=[callback=std::move(callback)](auto request, const common::Error& ec, SessionResponse response)
        {
            auto& req=serverapi::request<Request>(request).request();
            //! @todo critical: chain and log error
            //! @todo check if it is internal error or invalid login data
            if (ec)
            {
                req.response.setStatus(api::protocol::ResponseStatus::AuthError,ec);
                callback(std::move(request));
                return;
            }
            req.response.setSuccessMessage(std::move(response.response));
            callback(std::move(request));
        };

        //! @todo critical: keep username in login profile

        auto& reqEnv=serverapi::requestEnv<Request>(request);
        const auto& sessionController=reqEnv->sessionController();
        sessionController.createSession(
            std::move(request),
            std::move(cb),
            login->fieldValue(db::object::_id),
            login->fieldValue(login_profile::name),
            token->fieldValue(auth_challenge_token::topic)
        );
    };

    auto chain=hatn::chain(
        std::move(checkSharedSecret),
        std::move(createSession)
    );
    chain(std::move(request),std::move(callback),std::move(msg));
}

//--------------------------------------------------------------------------

template <typename RequestT>
HATN_VALIDATOR_NAMESPACE::error_report AuthHssCheckMethodImpl<RequestT>::validate(
    const common::SharedPtr<serverapi::RequestContext<Request>> /*request*/,
    const Message& /*msg*/
    ) const
{
    //! @todo critical: Implement validation of AuthHssCheckMethod
    return HATN_VALIDATOR_NAMESPACE::error_report{};
}

//--------------------------------------------------------------------------

HATN_SERVERAPP_NAMESPACE_END

#endif // HATNSERVERSHAREDSECRETPROTOCOL_IPP
