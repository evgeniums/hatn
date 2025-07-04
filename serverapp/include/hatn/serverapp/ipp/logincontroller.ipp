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
    //! @todo Fix findLogin

    // const auto& userController=ContextTraits::userController(ctx);
    // userController.findLogin(std::move(ctx),std::move(callback),login,topic);
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
    auto checkLogin=[this](auto&& checkUser, auto ctx, auto callback, auto login, auto topic)
    {
        auto cb=[checkUser=std::move(checkUser),callback=std::move(callback),topic=db::TopicHolder{topic}](auto ctx, const Error& ec, common::SharedPtr<login_profile::managed> loginObj)
        {
            HATN_CTX_SCOPE("logincontroller::checkcanlogin")
            if (ec)
            {
                HATN_CTX_SCOPE_ERROR("failed to find login")
                callback(std::move(ctx),ec);
                return;
            }

            if (loginObj->fieldValue(login_profile::blocked))
            {
                HATN_CTX_DEBUG(1,"login profile blocked")
                callback(std::move(ctx),clientServerError(ClientServerError::ACCESS_DENIED));
                return;
            }

            checkUser(std::move(ctx),std::move(callback),std::move(loginObj),std::move(topic));
        };
        this->findLogin(std::move(ctx),std::move(callback),login,topic);
    };

    auto checkUser=[](auto&& checkACL, auto ctx, auto callback, auto loginObj)
    {
        auto cb=[checkACL=std::move(checkACL),callback=std::move(callback),loginObj=std::move(loginObj)](auto ctx, const Error& ec, common::SharedPtr<user::managed> user)
        {
            HATN_CTX_SCOPE("logincontroller::checkcanlogin")
            if (ec)
            {
                HATN_CTX_SCOPE_ERROR("failed to find user")
                callback(std::move(ctx),ec);
                return;
            }

            if (user->fieldValue(user::blocked))
            {
                HATN_CTX_DEBUG(1,"user blocked")
                callback(std::move(ctx),clientServerError(ClientServerError::ACCESS_DENIED));
                return;
            }

            checkACL(std::move(ctx),std::move(callback),std::move(user),std::move(loginObj));
        };
        const auto& userController=ContextTraits::userController(ctx);
        userController.findUser(std::move(ctx),std::move(callback),loginObj->fieldValue(with_user::user),loginObj->fieldValue(with_user::user_topic));
    };

    auto checkACL=[](auto ctx, auto callback, auto userObj, auto loginObj)
    {
        //! @todo critical: Check user ACL
        std::ignore=userObj;
        std::ignore=loginObj;
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
