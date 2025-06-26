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
        common::SharedPtr<HATN_CLIENT_SERVER_NAMESPACE::auth_hss_check::managed> message,
        const LoginControllerT* loginController,
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
        callback(std::move(ctx),ec,{});
    }
    auto token=tokenR.takeValue();

    // check if token expired
    auto now=common::DateTime::currentUtc();
    if (now.after(token->fieldValue(auth_challenge_token::expire)))
    {
        callback(std::move(ctx),HATN_CLIENT_SERVER_NAMESPACE::clientServerError(HATN_CLIENT_SERVER_NAMESPACE::ClientServerError::AUTH_TOKEN_EXPIRED),{});
        return;
    }

    // find login
    auto findLogin=[loginController](auto&& checkMAC, auto ctx, auto callback, auto token)
    {
        auto tokenPtr=token.get();

        auto findCb=[token=std::move(token),callback=std::move(callback),checkMAC=std::move(checkMAC)](auto ctx, const Error& ec,
                                                                                                                 common::SharedPtr<HATN_CLIENT_SERVER_NAMESPACE::login_profile::managed> login)
        {
            if (ec)
            {
                //! @todo critical: chain and log error
                callback(std::move(ctx),ec,{});
                return;
            }

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
                           common::SharedPtr<HATN_CLIENT_SERVER_NAMESPACE::login_profile::managed> login)
    {
        Error ec;

        // prepare MAC algorithm
        crypt::CryptAlgorithmConstP macAlg;
        ec=m_suite->macAlgorithm(macAlg);
        if (ec)
        {
            //! @todo critical: chain and log error
            callback(std::move(ctx),ec,{});
            return;
        }

        // prepare MAC key
        auto key=macAlg->createMACKey();
        ec=key->importFromBuf(login->fieldValue(HATN_CLIENT_SERVER_NAMESPACE::login_profile::secret1),crypt::ContainerFormat::RAW_PLAIN);
        if (ec)
        {
            //! @todo critical: chain and log error
            callback(std::move(ctx),ec,{});
            return;
        }

        // check MAC
        auto mac=m_suite->createMAC(ec);
        if (ec)
        {
            //! @todo critical: chain and log error
            callback(std::move(ctx),ec,{});
            return;
        }
        ec=mac->check(macAlg,token->fieldValue(auth_challenge_token::challenge),message->fieldValue(HATN_CLIENT_SERVER_NAMESPACE::auth_hss_check::mac));
        if (ec)
        {
            //! @todo critical: log error
            callback(std::move(ctx),HATN_CLIENT_SERVER_NAMESPACE::clientServerError(HATN_CLIENT_SERVER_NAMESPACE::ClientServerError::AUTH_FAILED),{});
            return;
        }

        // MAC is ok
        // note that blocking and ACL must be checked somewhere else
        callback(std::move(ctx),{},std::move(login));
    };

    auto chain=hatn::chain(
        std::move(findLogin),
        std::move(checkMAC)
    );
    chain(std::move(ctx),std::move(callback),std::move(token));
}

//--------------------------------------------------------------------------

HATN_SERVERAPP_NAMESPACE_END

#endif // HATNSERVERSHAREDSECRETPROTOCOL_IPP
