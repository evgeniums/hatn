/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file serverapp/ipp/logincontroller.ipp
  */

/****************************************************************************/

#ifndef HATNSEVERLOGINCONTROLLER_IPP
#define HATNSEVERLOGINCONTROLLER_IPP

#include <hatn/common/meta/chain.h>

#include <hatn/logcontext/contextlogger.h>
#include <hatn/logcontext/context.h>

#include <hatn/db/dberror.h>

#include <hatn/api/autherror.h>
#include <hatn/api/makeapierror.h>

#include <hatn/clientserver/clientservererror.h>
#include <hatn/clientserver/models/user.h>
#include <hatn/clientserver/models/loginprofile.h>

#include <hatn/serverapp/logincontroller.h>

HATN_SERVERAPP_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

template <typename ContextTraits>
template <typename CallbackT>
void LoginController<ContextTraits>::findLogin(
        common::SharedPtr<Context> ctx,
        CallbackT callback,
        lib::string_view login,
        db::Topic topic
    ) const
{
    auto cb=[callback=std::move(callback)](auto ctx, auto result) mutable
    {
        if (result)
        {
            if (result.error().is(ClientServerError::INVALID_LOGIN_FORMAT,ClientServerErrorCategory::getCategory()))
            {
                auto ec=api::makeApiError(result.takeError(),api::ApiAuthError::INVALID_LOGIN_FORMAT,api::ApiAuthErrorCategory::getCategory());
                callback(std::move(ctx),std::move(ec),common::SharedPtr<login_profile::managed>{});
                return;
            }

            if (db::objectNotFound(result))
            {
                auto ec=api::makeApiError(common::chainErrors(result.takeError(),clientServerError(ClientServerError::LOGIN_NOT_FOUND)),api::ApiAuthError::AUTH_FAILED,api::ApiAuthErrorCategory::getCategory());
                callback(std::move(ctx),std::move(ec),common::SharedPtr<login_profile::managed>{});
                return;
            }

            HATN_CTX_SCOPE("logincontroller::findlogin")
            HATN_CTX_EC_LOG(result.error(),"failed to find login")
            callback(std::move(ctx),result.takeError(),common::SharedPtr<login_profile::managed>{});
            return;
        }

        callback(std::move(ctx),Error{},result->shared());
    };

    const auto& userController=ContextTraits::userController(ctx);
    userController.findLogin(std::move(ctx),std::move(cb),login,topic);
}

//--------------------------------------------------------------------------

template <typename ContextTraits>
template <typename CallbackT>
void LoginController<ContextTraits>::checkCanLogin(
        common::SharedPtr<Context> ctx,
        CallbackT callback,
        lib::string_view login,
        db::Topic topic
    ) const
{
    HATN_CTX_SCOPE_WITH_BARRIER("logincontroller::checkcanlogin")

    auto checkLogin=[this](auto&& checkUser, auto ctx, auto callback, auto login, auto topic)
    {
        HATN_CTX_SCOPE_WITH_BARRIER("[checklogin]")

        auto cb=[checkUser=std::move(checkUser),callback=std::move(callback),topic=db::TopicHolder{topic}](auto ctx, Error ec, common::SharedPtr<login_profile::managed> loginObj) mutable
        {
            if (ec)
            {
                HATN_CTX_SCOPE_LOCK()
                callback(std::move(ctx),std::move(ec));
                return;
            }
            Assert(loginObj,"login can not be null");

            if (loginObj->fieldValue(login_profile::blocked))
            {
                HATN_CTX_SCOPE_LOCK()
                ec=api::makeApiError(clientServerError(ClientServerError::LOGIN_BLOCKED),api::ApiAuthError::ACCESS_DENIED,api::ApiAuthErrorCategory::getCategory());
                callback(std::move(ctx),std::move(ec));
                return;
            }

            HATN_CTX_PUSH_FIXED_VAR("usr",loginObj->fieldValue(with_user::user).toString())

            HATN_CTX_STACK_BARRIER_OFF("[checklogin]")
            checkUser(std::move(ctx),std::move(callback),std::move(loginObj));
        };
        this->findLogin(std::move(ctx),std::move(cb),login,topic);
    };

    auto checkUser=[](auto&& checkACL, auto ctx, auto callback, auto loginObj)
    {
        HATN_CTX_SCOPE_WITH_BARRIER("[checkuser]")

        auto loginPtr=loginObj.get();
        auto cb=[checkACL=std::move(checkACL),callback=std::move(callback),loginObj=std::move(loginObj)](auto ctx, auto result) mutable
        {
            if (result)
            {
                if (db::objectNotFound(result))
                {
                    HATN_CTX_SCOPE_LOCK()
                    auto ec=api::makeApiError(common::chainErrors(result.takeError(),clientServerError(ClientServerError::USER_NOT_FOUND)),api::ApiAuthError::ACCESS_DENIED,api::ApiAuthErrorCategory::getCategory());
                    callback(std::move(ctx),std::move(ec));
                    return;
                }

                HATN_CTX_EC_LOG(result.error(),"failed to find user")
                callback(std::move(ctx),result.takeError());
                return;
            }
            Assert(!result->isNull(),"user can not be null");

            auto user=result->shared();
            if (user->fieldValue(user::blocked))
            {
                HATN_CTX_SCOPE_LOCK()
                auto ec=api::makeApiError(clientServerError(ClientServerError::USER_BLOCKED),api::ApiAuthError::ACCESS_DENIED,api::ApiAuthErrorCategory::getCategory());
                callback(std::move(ctx),std::move(ec));
                return;
            }

            HATN_CTX_STACK_BARRIER_OFF("[checkuser]")
            checkACL(std::move(ctx),std::move(callback),std::move(user),std::move(loginObj));
        };
        const auto& userController=ContextTraits::userController(ctx);
        userController.findUser(std::move(ctx),std::move(cb),loginPtr->fieldValue(with_user::user),loginPtr->fieldValue(with_user::user_topic));
    };

    auto checkACL=[](auto ctx, auto callback, auto userObj, auto loginObj)
    {
        // HATN_CTX_SCOPE_WITH_BARRIER("[checkacl]")

        //! @todo critical: Check user ACL
        std::ignore=userObj;
        std::ignore=loginObj;

        HATN_CTX_STACK_BARRIER_OFF("logincontroller::checkcanlogin")
        callback(std::move(ctx),Error{});
    };

    auto chain=hatn::chain(
        std::move(checkLogin),
        std::move(checkUser),
        std::move(checkACL)
    );
    chain(std::move(ctx),std::move(callback),login,topic);
}

//--------------------------------------------------------------------------

HATN_SERVERAPP_NAMESPACE_END

#endif // HATNSEVERLOGINCONTROLLER_IPP
