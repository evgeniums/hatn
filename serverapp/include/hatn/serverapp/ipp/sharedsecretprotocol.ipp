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
#include <hatn/common/apierror.h>

#include "hatn/logcontext/context.h"

#include <hatn/crypt/cryptcontainer.h>

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/wirebufsolid.h>

#include <hatn/api/autherror.h>
#include <hatn/api/makeapierror.h>

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
    HATN_CTX_SCOPE_WITH_BARRIER("authhss::check")

    // deserialize token
    const auto& tokenField=message->field(auth_hss_check::token);
    const auto* tokenBuf=tokenField.buf();
    auto tokenR=parseToken<auth_challenge_token::managed>(*tokenBuf,factory);
    if (tokenR)
    {
        auto ec=api::makeApiError(ClientServerError::AUTH_TOKEN_INVALID,ClientServerErrorCategory::getCategory(),api::ApiAuthError::AUTH_FAILED,api::ApiAuthErrorCategory::getCategory());
        HATN_CTX_SCOPE_ERROR("failed to parse token")
        callback(std::move(ctx),common::chainErrors(tokenR.takeError(),std::move(ec)),{},{});
        return;
    }
    auto token=tokenR.takeValue();

    HATN_CTX_PUSH_FIXED_VAR("login",token->fieldValue(auth_challenge_token::login))
    if (!token->fieldValue(auth_challenge_token::topic).empty())
    {
        HATN_CTX_PUSH_FIXED_VAR("login_topic",token->fieldValue(auth_challenge_token::topic))
    }

    // check if token expired
    auto now=common::DateTime::currentUtc();
    if (now.after(token->fieldValue(auth_challenge_token::expire)))
    {
        auto ec=api::makeApiError(ClientServerError::AUTH_TOKEN_EXPIRED,ClientServerErrorCategory::getCategory(),api::ApiAuthError::AUTH_FAILED,api::ApiAuthErrorCategory::getCategory());
        callback(std::move(ctx),std::move(ec),std::move(token),{});
        return;
    }

    // find login
    auto findLogin=[loginController=&loginController](auto&& checkMAC, auto ctx, auto callback, auto token)
    {
        HATN_CTX_SCOPE_WITH_BARRIER("[findlogin]")

        auto tokenPtr=token.get();

        auto findCb=[token=std::move(token),callback=std::move(callback),checkMAC=std::move(checkMAC)](auto ctx, Error ec,
                                                                                                                 common::SharedPtr<login_profile::managed> login) mutable
        {
            if (ec)
            {
                callback(std::move(ctx),std::move(ec),std::move(token),{});
                return;
            }

            HATN_CTX_STACK_BARRIER_OFF("[findlogin]")
            checkMAC(std::move(ctx),std::move(callback),std::move(token),std::move(login));
        };

        loginController->findLogin(
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
        HATN_CTX_SCOPE_WITH_BARRIER("[checkmac]")

        Error ec;

        // prepare MAC algorithm
        crypt::CryptAlgorithmConstP macAlg;
        ec=m_suite->macAlgorithm(macAlg);
        if (ec)
        {
            HATN_CTX_EC_LOG(ec,"failed to find mac algorithm")
            callback(std::move(ctx),std::move(ec),std::move(token),{});
            return;
        }

        // prepare MAC key
        auto key=macAlg->createMACKey();
        ec=key->importFromBuf(login->fieldValue(login_profile::secret1),crypt::ContainerFormat::RAW_PLAIN);
        if (ec)
        {
            HATN_CTX_EC_LOG(ec,"failed to import login's secret")
            callback(std::move(ctx),std::move(ec),std::move(token),{});
            return;
        }

        // check MAC
        auto mac=m_suite->createMAC(ec);
        if (ec)
        {
            HATN_CTX_EC_LOG(ec,"failed to create MAC processor")
            callback(std::move(ctx),std::move(ec),std::move(token),{});
            return;
        }
        mac->setKey(key.get());
        ec=mac->runVerify(token->fieldValue(auth_challenge_token::challenge),message->fieldValue(auth_hss_check::mac));
        if (ec)
        {
            if (ec.is(crypt::CryptError::DIGEST_MISMATCH))
            {
                ec=api::makeApiError(std::move(ec),api::ApiAuthError::AUTH_FAILED,api::ApiAuthErrorCategory::getCategory());
            }
            else
            {
                HATN_CTX_EC_LOG(ec,"failed to verify MAC")
            }
            callback(std::move(ctx),std::move(ec),std::move(token),{});
            return;
        }

        // MAC is ok
        // note that blocking and ACL must be checked somewhere else
        HATN_CTX_STACK_BARRIER_OFF("authhss::check")
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
    HATN_CTX_SCOPE_WITH_BARRIER("authhss")

    auto checkSharedSecret=[](auto&& createSession, auto request, auto callback, auto msg) mutable
    {
        HATN_CTX_SCOPE_WITH_BARRIER("[checksharedsecret]")

        auto cb=[createSession=std::move(createSession),callback=std::move(callback)](auto ctx, Error ec, common::SharedPtr<auth_challenge_token::managed> token, common::SharedPtr<login_profile::managed> login) mutable
        {
            auto& req=serverapi::request<Request>(ctx).request();
            if (ec)
            {
                if (ec.apiError()!=nullptr && ec.apiError()->isFamily(api::ApiAuthErrorCategory::getCategory()))
                {
                    req.setResponseError(std::move(ec),api::protocol::ResponseStatus::AuthError);
                }
                else
                {
                    req.setResponseError(std::move(ec));
                }

                callback(std::move(ctx));
                return;
            }
            else
            {
                HATN_CTX_STACK_BARRIER_OFF("[checksharedsecret]")
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
        HATN_CTX_SCOPE_WITH_BARRIER("[createsession]")

        auto tokenPtr=token.get();
        auto loginPtr=login.get();
        auto cb=[callback=std::move(callback),token=std::move(token),login=std::move(login)](auto request, common::Error ec, SessionResponse response)
        {
            auto& req=serverapi::request<Request>(request).request();
            if (ec)
            {
                if (ec.apiError()!=nullptr && ec.apiError()->isFamily(api::ApiAuthErrorCategory::getCategory()))
                {
                    req.setResponseError(std::move(ec),api::protocol::ResponseStatus::Forbidden);
                }
                else
                {
                    req.setResponseError(std::move(ec));
                }
                callback(std::move(request));
                return;
            }
            req.response.setSuccessMessage(std::move(response.response));

            HATN_CTX_STACK_BARRIER_OFF("authhss")
            callback(std::move(request));
        };

        auto& reqEnv=serverapi::requestEnv<Request>(request);
        const auto& sessionController=reqEnv->sessionController();
        sessionController.createSession(
            std::move(request),
            std::move(cb),
            loginPtr->fieldValue(db::object::_id),
            loginPtr->fieldValue(with_user::user),
            tokenPtr->fieldValue(auth_challenge_token::topic)
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
