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

#include <hatn/db/object.h>
#include <hatn/db/asyncclient.h>
#include <hatn/db/update.h>
#include <hatn/db/indexquery.h>

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
        lib::string_view username,
        db::Topic topic
    ) const
{
    const common::pmr::AllocatorFactory* factory=ContextTraits::factory(ctx);
    auto session=factory->template createObject<session::managed>();
    db::initObject(*session);
    session->setFieldValue(session::login,login);
    session->setFieldValue(session::username,username);
    session->setFieldValue(session::ttl,session->fieldValue(db::object::created_at));
    session->field(session::ttl).mutableValue()->addDays(config().fieldValue(session_config::session_ttl_days));

    auto sessToken=tokenHandler()->makeToken(session.get(),auth_token::TokenType::Session,config().fieldValue(session_config::session_token_ttl_secs),topic,factory);
    if (sessToken)
    {
        //! @todo critical: Log and chain errors
        auto ec=sessToken.takeError();
        callback(std::move(ctx),ec,{});
        return;
    }
    auto refreshToken=tokenHandler()->makeToken(session.get(),auth_token::TokenType::Refresh,config().fieldValue(session_config::refresh_token_ttl_secs),topic,factory);
    if (refreshToken)
    {
        //! @todo critical: Log and chain errors
        auto ec=refreshToken.takeError();
        callback(std::move(ctx),ec,{});
        return;
    }

    auto checkCanLogin=[topic](auto&& createSess, auto ctx, auto callback, auto session)
    {
        auto sessPtr=session.get();
        auto cb=[session=std::move(session),callback=std::move(callback),createSess=std::move(createSess)](auto ctx, const common::Error& ec)
        {
            if (ec)
            {
                //! @todo critical: Log and chain errors
                callback(std::move(ctx),ec,{});
                return;
            }

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
        auto sessPtr=session.get();
        auto createCb=[session=std::move(session()),callback=std::move(callback),createSessionClient=std::move(createSessionClient)](auto ctx, const common::Error& ec)
        {
            if (ec)
            {
                //! @todo critical: Log and chain errors
                callback(std::move(ctx),ec,{});
                return;
            }

            createSessionClient(std::move(ctx),std::move(callback),std::move(session));
        };

        const db::AsyncDb& asyncDb=ContextTraits::db(ctx);
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
        auto sessionClient=factory->template createObject<session_client::managed>();
        db::initObject(*sessionClient);
        sessionClient->setFieldValue(session_client::login,session->fieldValue(session::login));
        sessionClient->setFieldValue(session_client::session,session->fieldValue(db::object::_id));
        sessionClient->setFieldValue(session_client::ttl,session->fieldValue(session::ttl));
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
        auto createCb=[sessionClient=std::move(sessionClient),session=std::move(session()),callback=std::move(callback),done=std::move(done)](auto ctx, const common::Error& ec)
        {
            if (ec)
            {
                //! @todo critical: Log and chain errors
                callback(std::move(ctx),ec,{});
                return;
            }

            done(std::move(ctx),std::move(callback),std::move(session));
        };

        const db::AsyncDb& asyncDb=ContextTraits::db(ctx);
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

        callback(std::move(ctx),common::Error{},SessionResponse{std::move(response,std::move(session))});
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
    const common::pmr::AllocatorFactory* factory=ContextTraits::factory(ctx);

    // parse token
    auto token=tokenHandler()->parseToken(sessionContent.get(),auth_token::TokenType::Session,factory);
    if (token)
    {
        //! @todo critical: Log and chain errors
        auto ec=token.takeError();
        callback(std::move(ctx),ec,{});
        return;
    }

    // check if session exists and active
    auto checkSess=[dbModels=sessionDbModels()](auto&& checkCanLogin, auto ctx, auto callback, common::SharedPtr<auth_token::managed> token)
    {
        auto tokenPtr=token.get();
        auto topic=tokenPtr->fieldValue(auth_token::topic);
        auto cb=[checkCanLogin=std::move(checkCanLogin),callback=std::move(callback),token=std::move(token)](auto ctx, auto foundSessObj) mutable
        {
            if (foundSessObj)
            {
                //! @todo critical: Log and chain errors
                auto ec=foundSessObj.takeError();
                callback(std::move(ctx),ec,{});
                return;
            }

            if (!foundSessObj.value()->fieldValue(session::active))
            {
                auto ec=HATN_CLIENT_SERVER_NAMESPACE::clientServerError(HATN_CLIENT_SERVER_NAMESPACE::ClientServerError::AUTH_SESSION_INVALID);
                callback(std::move(ctx),ec,{});
                return;
            }

            if (foundSessObj.value()->fieldValue(session::login)!=token->fieldValue(auth_token::login))
            {
                //! @todo critical: Log and chain errors
                auto ec=HATN_CLIENT_SERVER_NAMESPACE::clientServerError(HATN_CLIENT_SERVER_NAMESPACE::ClientServerError::AUTH_SESSION_INVALID);
                callback(std::move(ctx),ec,{});
                return;
            }

            checkCanLogin(std::move(ctx),std::move(callback),foundSessObj.takeValue(),std::move(token));
        };

        const db::AsyncDb& asyncDb=ContextTraits::db(ctx);
        const auto& dbClient=asyncDb.dbClient(topic);
        dbClient->read(
            std::move(ctx),
            std::move(cb),
            topic,
            dbModels->sessionModel(),
            tokenPtr->fieldValue(auth_token::session)
        );
    };

    auto checkCanLogin=[](auto&& updateSessClient, auto ctx, auto callback, auto session, common::SharedPtr<auth_token::managed> token)
    {
        auto tokenPtr=token.get();
        auto topic=tokenPtr->fieldValue(auth_token::topic);
        auto sessPtr=session.get();
        auto cb=[session=std::move(session),callback=std::move(callback),updateSessClient=std::move(updateSessClient),token=std::move(token)](auto ctx, const common::Error& ec)
        {
            if (ec)
            {
                //! @todo critical: Log and chain errors
                callback(std::move(ctx),ec,{});
                return;
            }

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

    auto updateSessClient=[update,this](auto&& done, auto ctx, auto callback, auto session, common::SharedPtr<auth_token::managed> token)
    {
        if (!update)
        {
            done(std::move(ctx),std::move(callback),std::move(session),std::move(token));
            return;
        }

        auto tokenPtr=token.get();
        auto topic=tokenPtr->fieldValue(auth_token::topic);
        auto sessPtr=session.get();
        auto cb=[session=std::move(session),callback=std::move(callback),done=std::move(done),token=std::move(token)](auto ctx, const common::Error& ec)
        {
            if (ec)
            {
                //! @todo critical: Log error
            }

            done(std::move(ctx),std::move(callback),std::move(session),std::move(token));
        };

        this->updateSessionClient(
            std::move(ctx),
            std::move(cb),
            sessPtr->fieldValue(db::object::_id),
            topic
        );
    };

    auto done=[](auto ctx, auto callback, auto session, common::SharedPtr<auth_token::managed> token)
    {
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
        const common::SharedPtr<session::managed>& session,
        db::Topic topic
    ) const
{
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
        [session,ctx,topic]()
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
        if (res)
        {
            callback(std::move(ctx),res.error());
        }
        else
        {
            callback(std::move(ctx),Error{});
        }
    };

    const db::AsyncDb& asyncDb=ContextTraits::db(ctx);
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
}

//--------------------------------------------------------------------------

HATN_SERVERAPP_NAMESPACE_END

#endif // HATNLOCALSESSIONCONTROLLER_IPP
