/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file serverapp/logincontroller.h
  */

/****************************************************************************/

#ifndef HATNLOGINCONTROLLER_H
#define HATNLOGINCONTROLLER_H

#include <hatn/dataunit/objectid.h>
#include <hatn/db/topic.h>

#include <hatn/serverapp/serverappdefs.h>

HATN_SERVERAPP_NAMESPACE_BEGIN

template <typename ContextTraits>
class LoginController
{
    public:

        using Context=typename ContextTraits::Context;

        // CallbackT= void (auto ctx, const Error& ec, common::SharedPtr<HATN_CLIENT_SERVER_NAMESPACE::login_profile::managed> login)
        template <typename CallbackT>
        void findLogin(
            common::SharedPtr<Context> ctx,
            CallbackT callback,
            lib::string_view login,
            db::Topic topic
        ) const;

        // CallbackT = void (auto ctx, const common::Error& ec)
        template <typename CallbackT>
        void checkCanLogin(
            common::SharedPtr<Context> ctx,
            CallbackT callback,
            lib::string_view login,
            db::Topic topic
        ) const;

        template <typename CallbackT>
        void checkCanLogin(
            common::SharedPtr<Context> ctx,
            CallbackT callback,
            const du::ObjectId& login,
            db::Topic topic
        ) const
        {
            checkCanLogin(std::move(ctx),std::move(callback),login.string(),topic);
        }
};

HATN_SERVERAPP_NAMESPACE_END

#endif // HATNLOGINCONTROLLER_H
