/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file serverapp/ipp/localsessioncontroller.ipp
  */

/****************************************************************************/

#ifndef HATNLOCALSESSIONCONTROLLER_IPP
#define HATNLOCALSESSIONCONTROLLER_IPP

#include <hatn/common/meta/chain.h>

#include <hatn/logcontext/context.h>

#include <hatn/db/object.h>
#include <hatn/db/asyncclient.h>
#include <hatn/db/update.h>
#include <hatn/db/indexquery.h>

#include <hatn/api/autherror.h>
#include <hatn/api/makeapierror.h>

#include <hatn/clientserver/clientservererror.h>

#include <hatn/serverapp/sessiondbmodels.h>
#include <hatn/serverapp/sessiontoken.h>
#include <hatn/serverapp/sessioncontroller.h>
#include <hatn/serverapp/localsessioncontroller.h>

HATN_SERVERAPP_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

template <typename ContextTraits>
template <typename CallbackT>
void LocalSessionController<ContextTraits>::createSession(
        common::SharedPtr<Context> ctx,
        CallbackT callback,
        const du::ObjectId& login,
        const du::ObjectId& user,
        db::Topic topic
    ) const
{
    HATN_CTX_SCOPE_WITH_BARRIER("sessioncontroller::createsession")

    const common::pmr::AllocatorFactory* factory=ContextTraits::factory(ctx);
    auto session=factory->template createObject<session::managed>();
    db::initObject(*session);
    session->setFieldValue(session::login,login);
    session->setFieldValue(session::user,user);
    session->setFieldValue(session::ttl,session->fieldValue(db::object::created_at));
    session->field(session::ttl).mutableValue()->addSeconds(config().fieldValue(session_config::session_ttl_secs));

    auto& req=ContextTraits::request(ctx);
    req.sessionId=session->fieldValue(db::object::_id);
    HATN_CTX_PUSH_FIXED_VAR("sess",req.sessionId.toString())

    auto sessToken=tokenHandler().makeToken(session.get(),auth_token::TokenType::Session,config().fieldValue(session_config::session_token_ttl_secs),topic,factory);
    if (sessToken)
    {
        auto ec=sessToken.takeError();
        HATN_CTX_EC_LOG(ec,"failed to make session token")
        callback(std::move(ctx),ec,{});
        return;
    }
    auto refreshToken=tokenHandler().makeToken(session.get(),auth_token::TokenType::Refresh,config().fieldValue(session_config::refresh_token_ttl_secs),topic,factory);
    if (refreshToken)
    {
        auto ec=refreshToken.takeError();
        HATN_CTX_EC_LOG(ec,"failed to make refresh token")
        callback(std::move(ctx),ec,{});
        return;
    }

    auto checkCanLogin=[topic](auto&& createSess, auto ctx, auto callback, auto session)
    {
        HATN_CTX_SCOPE_WITH_BARRIER("[checkcanlogin]")

        auto sessPtr=session.get();
        auto cb=[session=std::move(session),callback=std::move(callback),createSess=std::move(createSess)](auto ctx, common::Error ec) mutable
        {
            if (ec)
            {
                callback(std::move(ctx),std::move(ec),{});
                return;
            }

            HATN_CTX_STACK_BARRIER_OFF("[checkcanlogin]")
            createSess(std::move(ctx),std::move(callback),std::move(session));
        };

        const auto& loginController=ContextTraits::loginController(ctx);
        loginController.checkCanLogin(
            std::move(ctx),
            std::move(cb),
            sessPtr->fieldValue(session::login),
            topic
        );
    };

    auto createSess=[topic,dbModels=sessionDbModels()](auto&& createSessionClient, auto ctx, auto callback, auto session)
    {
        HATN_CTX_SCOPE_WITH_BARRIER("[createsess]")

        auto sessPtr=session.get();
        auto createCb=[session=std::move(session),callback=std::move(callback),createSessionClient=std::move(createSessionClient)](auto ctx, const common::Error& ec) mutable
        {
            if (ec)
            {
                HATN_CTX_EC_LOG(ec,"failed to create session in db")
                callback(std::move(ctx),ec,{});
                return;
            }

            HATN_CTX_STACK_BARRIER_OFF("[createsess]")
            createSessionClient(std::move(ctx),std::move(callback),std::move(session));
        };

        const db::AsyncDb& asyncDb=ContextTraits::contextDb(ctx);
        const auto& dbClient=asyncDb.dbClient(topic);
        dbClient->create(
            std::move(ctx),
            std::move(createCb),
            topic,
            dbModels->sessionModel(),
            sessPtr
        );
    };

    auto createSessionClient=[factory,topic,dbModels=sessionDbModels()](auto&& done, auto ctx, auto callback, auto session)
    {
        HATN_CTX_SCOPE_WITH_BARRIER("[createsessionclient]")

        auto sessionClient=factory->template createObject<session_client::managed>();
        db::initObject(*sessionClient);
        sessionClient->setFieldValue(session_client::login,session->fieldValue(session::login));
        sessionClient->setFieldValue(session_client::session,session->fieldValue(db::object::_id));
        sessionClient->setFieldValue(session_client::ttl,session->fieldValue(session::ttl));

        auto& req=ContextTraits::request(ctx);
        req.sessionClientId=session->fieldValue(db::object::_id);

        //! @todo critical: Implement ClientAgent and ClientIp for server request
#if 0
        const ClientAgent& agent=ContextTraits::clientAgent(ctx);
        sessionClient->setFieldValue(session_client::agent,agent.name);
        sessionClient->setFieldValue(session_client::agent_os,agent.os);
        sessionClient->setFieldValue(session_client::agent_version,agent.version);
        const ClientIp& ip=ContextTraits::clientIp(ctx);
        sessionClient->setFieldValue(session_client::ip_addr,ip.ip_addr);
        sessionClient->setFieldValue(session_client::proxy_ip_addr,ip.proxy_ip_addr);
#endif
        auto sessClientPtr=sessionClient.get();
        auto createCb=[sessionClient=std::move(sessionClient),session=std::move(session),callback=std::move(callback),done=std::move(done)](auto ctx, const common::Error& ec) mutable
        {
            if (ec)
            {
                HATN_CTX_EC_LOG(ec,"failed to create session client in db")
                callback(std::move(ctx),ec,{});
                return;
            }

            HATN_CTX_STACK_BARRIER_OFF("[createsessionclient]")
            done(std::move(ctx),std::move(callback),std::move(session));
        };

        const db::AsyncDb& asyncDb=ContextTraits::contextDb(ctx);
        const auto& dbClient=asyncDb.dbClient(topic);
        dbClient->create(
            std::move(ctx),
            std::move(createCb),
            topic,
            dbModels->sessionClientModel(),
            sessClientPtr
        );
    };

    auto done=[refreshToken=refreshToken.takeValue(),sessToken=sessToken.takeValue(),factory](auto ctx, auto callback, auto session)
    {
        auto response=factory->createObject<HATN_CLIENT_SERVER_NAMESPACE::auth_complete::shared_managed>();
        auto& sessTokenField=response->field(HATN_CLIENT_SERVER_NAMESPACE::auth_complete::session_token);
        sessTokenField.set(std::move(sessToken.clientToken));
        auto& refreshTokenField=response->field(HATN_CLIENT_SERVER_NAMESPACE::auth_complete::refresh_token);
        refreshTokenField.set(std::move(refreshToken.clientToken));

        HATN_CTX_STACK_BARRIER_OFF("sessioncontroller::createsession")
        callback(std::move(ctx),common::Error{},SessionResponse{std::move(response),std::move(session)});
    };

    auto chain=hatn::chain(
        std::move(checkCanLogin),
        std::move(createSess),
        std::move(createSessionClient),
        std::move(done)
    );
    chain(std::move(ctx),std::move(callback),std::move(session));
}

//--------------------------------------------------------------------------

template <typename ContextTraits>
template <typename CallbackT>
void LocalSessionController<ContextTraits>::checkSession(
        common::SharedPtr<Context> ctx,
        CallbackT callback,
        common::SharedPtr<HATN_CLIENT_SERVER_NAMESPACE::auth_with_token::managed> sessionContent,
        bool update
    ) const
{
    HATN_CTX_SCOPE_WITH_BARRIER("sessioncontroller::checksession")

    const common::pmr::AllocatorFactory* factory=ContextTraits::factory(ctx);

    // parse token
    auto token=tokenHandler().parseToken(sessionContent.get(),auth_token::TokenType::Session,factory);
    if (token)
    {
        HATN_CTX_SCOPE_ERROR("failed to parse token")
        auto ec=api::makeApiError(common::chainErrors(token.takeError(),clientServerError(ClientServerError::AUTH_TOKEN_INVALID)),api::ApiAuthError::AUTH_FAILED,api::ApiAuthErrorCategory::getCategory());
        callback(std::move(ctx),ec,{});
        return;
    }

    // check if session exists and active
    auto checkSess=[dbModels=sessionDbModels()](auto&& checkCanLogin, auto reqCtx, auto callback, common::SharedPtr<auth_token::managed> token)
    {
        HATN_CTX_SCOPE_WITH_BARRIER("[checksess]")

        HATN_CTX_PUSH_FIXED_VAR("login",token->fieldValue(auth_token::login).toString())
        HATN_CTX_PUSH_FIXED_VAR("usrtpc",token->fieldValue(auth_token::topic))
        HATN_CTX_PUSH_FIXED_VAR("sess",token->fieldValue(auth_token::session).toString())

        auto tokenPtr=token.get();
        auto topic=tokenPtr->fieldValue(auth_token::topic);
        auto cb=[checkCanLogin=std::move(checkCanLogin),callback=std::move(callback),token=std::move(token),reqCtx](auto, auto foundSessObj) mutable
        {
            if (foundSessObj)
            {
                if (db::objectNotFound(foundSessObj))
                {
                    auto ec=api::makeApiError(clientServerError(ClientServerError::AUTH_SESSION_NOT_FOUND),api::ApiAuthError::AUTH_SESSION_INVALID,api::ApiAuthErrorCategory::getCategory());
                    callback(std::move(reqCtx),ec,{});
                    return;
                }

                HATN_CTX_SCOPE_ERROR("failed to find session in db")
                auto ec=foundSessObj.takeError();
                callback(std::move(reqCtx),ec,{});
                return;
            }

            auto session=foundSessObj->shared();

            if (!session->fieldValue(session::active))
            {
                auto ec=api::makeApiError(clientServerError(ClientServerError::AUTH_SESSION_INACTIVE),api::ApiAuthError::AUTH_SESSION_INVALID,api::ApiAuthErrorCategory::getCategory());
                callback(std::move(reqCtx),ec,{});
                return;
            }

            if (session->fieldValue(session::login)!=token->fieldValue(auth_token::login))
            {
                auto ec=api::makeApiError(clientServerError(ClientServerError::AUTH_SESSION_LOGINS_MISMATCH),api::ApiAuthError::AUTH_SESSION_INVALID,api::ApiAuthErrorCategory::getCategory());
                callback(std::move(reqCtx),ec,{});
                return;
            }

            HATN_CTX_STACK_BARRIER_OFF("[checksess]")
            checkCanLogin(std::move(reqCtx),std::move(callback),std::move(session),std::move(token));
        };

        const db::AsyncDb& asyncDb=ContextTraits::contextDb(reqCtx);
        const auto& dbClient=asyncDb.dbClient(topic);
        dbClient->read(
            std::move(reqCtx),
            std::move(cb),
            topic,
            dbModels->sessionModel(),
            tokenPtr->fieldValue(auth_token::session)
        );
    };

    auto checkCanLogin=[](auto&& updateSessClient, auto ctx, auto callback, common::SharedPtr<session::managed> session, common::SharedPtr<auth_token::managed> token) mutable
    {
        HATN_CTX_SCOPE_WITH_BARRIER("[checkcanlogin]")

        auto tokenPtr=token.get();
        auto topic=tokenPtr->fieldValue(auth_token::topic);
        auto sessPtr=session.get();
        auto cb=[session=std::move(session),callback=std::move(callback),updateSessClient=std::move(updateSessClient),token=std::move(token)](auto ctx, common::Error ec) mutable
        {
            if (ec)
            {                
                callback(std::move(ctx),std::move(ec),{});
                return;
            }

            auto& req=ContextTraits::request(ctx);
            req.sessionId=session->fieldValue(db::object::_id);
            req.login=session->fieldValue(session::login);
            req.userTopic.load(token->fieldValue(auth_token::topic));
            req.user=session->fieldValue(session::user);

            HATN_CTX_STACK_BARRIER_OFF("[checkcanlogin]")
            updateSessClient(std::move(ctx),std::move(callback),std::move(session),std::move(token));
        };

        const auto& loginController=ContextTraits::loginController(ctx);
        loginController.checkCanLogin(
            std::move(ctx),
            std::move(cb),
            sessPtr->fieldValue(session::login),
            topic
        );
    };

    auto updateSessClient=[update,this](auto&& done, auto ctx, auto callback, common::SharedPtr<session::managed> session, common::SharedPtr<auth_token::managed> token)
    {
        if (!update)
        {
            done(std::move(ctx),std::move(callback),std::move(session),std::move(token));
            return;
        }

        //! @todo fix barriers with callbacks in the same call stack
#if 0
        HATN_CTX_SCOPE_WITH_BARRIER("[updatesesscl]")
#endif
        auto tokenPtr=token.get();
        auto topic=tokenPtr->fieldValue(auth_token::topic);
        auto cb=[session,callback=std::move(callback),done=std::move(done),token=std::move(token)](auto ctx, const common::Error& ec) mutable
        {
            if (ec)
            {
                HATN_CTX_ERROR(ec,"faield to update session client in db")
            }
            //! @todo fix barriers with callbacks in the same call stack
#if 0
            HATN_CTX_STACK_BARRIER_OFF("[updatesesscl]")
#endif
            done(std::move(ctx),std::move(callback),std::move(session),std::move(token));
        };

        this->updateSessionClient(
            std::move(ctx),
            std::move(cb),
            session,
            topic
        );
    };

    auto done=[](auto ctx, auto callback, common::SharedPtr<session::managed> session, common::SharedPtr<auth_token::managed> token)
    {
        HATN_CTX_STACK_BARRIER_OFF("sessioncontroller::checksession")
        callback(std::move(ctx),common::Error{},SessionCheckResult{std::move(token),std::move(session)});
    };

    auto chain=hatn::chain(
        std::move(checkSess),
        std::move(checkCanLogin),
        std::move(updateSessClient),
        std::move(done)
    );
    chain(std::move(ctx),std::move(callback),token.takeValue());
}

//--------------------------------------------------------------------------

template <typename ContextTraits>
template <typename CallbackT>
void LocalSessionController<ContextTraits>::updateSessionClient(
        common::SharedPtr<Context> ctx,
        CallbackT callback,
        common::SharedPtr<session::managed> session,
        db::Topic topic
    ) const
{
//! @todo Implement ClientAgent and ClientIp
#if 0

    const common::pmr::AllocatorFactory* factory=ContextTraits::factory(ctx);

    auto sessionClient=factory->template createObject<session_client::managed>();
    db::initObject(*sessionClient);
    sessionClient->setFieldValue(session_client::login,session->fieldValue(session::login));
    sessionClient->setFieldValue(session_client::session,session->fieldValue(db::object::_id));
    sessionClient->setFieldValue(session_client::ttl,session->fieldValue(session::ttl));
    const ClientAgent& agent=ContextTraits::clientAgent(ctx);
    sessionClient->setFieldValue(session_client::agent,agent.name);
    sessionClient->setFieldValue(session_client::agent_os,agent.os);
    sessionClient->setFieldValue(session_client::agent_version,agent.version);
    const ClientIp& ip=ContextTraits::clientIp(ctx);
    sessionClient->setFieldValue(session_client::ip_addr,ip.ip_addr);
    sessionClient->setFieldValue(session_client::proxy_ip_addr,ip.proxy_ip_addr);

    auto q=db::wrapQueryBuilder(
        [session=std::move(session),ctx,topic]()
        {
            const ClientIp& ip=ContextTraits::clientIp(ctx);
            return db::makeQuery(sessionClientSessIdx(),
                                 db::where(session_client::session,db::query::eq,session->fieldValue(db::object::_id)).
                                 and_(session_client::ip_addr,db::query::eq,ip.ip_addr).
                                 and_(session_client::proxy_ip_addr,db::query::eq,ip.proxy_ip_addr),
                                 topic
                                );
        },
        topic
    );

    auto r=db::update::sharedRequest(
        db::update::field(session_client::agent_version,db::update::set,agent.version)
    );

    auto cb=[callback=std::move(callback),sessionClient](auto ctx, auto res)
    {
        auto& req=ContextTraits::request(ctx);
        req.sessionClientId=session->fieldValue(db::object::_id);

        if (res)
        {
            callback(std::move(ctx),res.error());
        }
        else
        {
            callback(std::move(ctx),Error{});
        }
    };

    const db::AsyncDb& asyncDb=ContextTraits::contextDb(ctx);
    const auto& dbClient=asyncDb.dbClient(topic);
    dbClient->findUpdateCreate(
        std::move(ctx),
        std::move(cb),
        sessionDbModels()->sessionClientModel(),
        std::move(q),
        std::move(r),
        sessionClient,
        db::update::ModifyReturn::After,
        nullptr,
        topic
    );
#else
    auto& req=ContextTraits::request(ctx);
    req.sessionClientId=du::ObjectId::generateId();
    HATN_CTX_PUSH_FIXED_VAR("sesscl",req.sessionClientId.toString())
    callback(std::move(ctx),Error{});
#endif
}

//--------------------------------------------------------------------------

HATN_SERVERAPP_NAMESPACE_END

#endif // HATNLOCALSESSIONCONTROLLER_IPP
