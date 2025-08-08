/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file clientserver/usercontroller.h
  */

/****************************************************************************/

#ifndef HATNUSERCONTROLLER_H
#define HATNUSERCONTROLLER_H

#include <hatn/common/objecttraits.h>

#include <hatn/db/update.h>
#include <hatn/db/indexquery.h>
#include <hatn/db/asyncmodelcontroller.h>

#include <hatn/clientserver/clientserver.h>
#include <hatn/clientserver/models/user.h>
#include <hatn/clientserver/models/loginprofile.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

template <typename ContextTraits, typename Traits>
class UserController : public common::WithTraits<Traits>
{
    public:

        using common::WithTraits<Traits>::WithTraits;

        using Context=typename ContextTraits::Context;
        using CallbackEc=db::AsyncCallbackEc<Context>;
        using CallbackList=db::AsyncCallbackList<Context>;
        using CallbackOid=db::AsyncCallbackOid<Context>;
        template <typename ModelT>
        using CallbackObj=db::AsyncCallbackObj<Context,ModelT>;

        void addUser(
            common::SharedPtr<Context> ctx,
            CallbackOid callback,
            common::SharedPtr<user::managed> usr,
            db::Topic topic
        )
        {
            this->traits().addUser(std::move(ctx),std::move(callback),std::move(usr),topic);
        }

        void findUser(
            common::SharedPtr<Context> ctx,
            CallbackObj<user::managed> callback,
            const du::ObjectId& userId,
            db::Topic topic
        ) const
        {
            this->traits().findUser(std::move(ctx),std::move(callback),userId,topic);
        }

        void addLogin(
            common::SharedPtr<Context> ctx,
            CallbackOid callback,
            common::SharedPtr<user_login::managed> login,
            db::Topic topic
            )
        {
            this->traits().addLogin(std::move(ctx),std::move(callback),std::move(login),topic);
        }

        void findLogin(
            common::SharedPtr<Context> ctx,
            CallbackObj<user_login::managed> callback,
            lib::string_view login,
            db::Topic topic
        ) const
        {
            this->traits().findLogin(std::move(ctx),std::move(callback),login,topic);
        }

        //! @todo Implement the rest user operations
};

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNUSERCONTROLLER_H
