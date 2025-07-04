/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file serverapp/localusercontroller.h
  */

/****************************************************************************/

#ifndef HATNLOCALUSERCONTROLLER_H
#define HATNLOCALUSERCONTROLLER_H

#include <hatn/db/modelsprovider.h>

#include <hatn/clientserver/usercontroller.h>

#include <hatn/serverapp/serverappdefs.h>

HATN_SERVERAPP_NAMESPACE_BEGIN

template <typename ContextTraits>
class LocalUserController_p;

template <typename ContextTraits>
class LocalUserControllerImpl
{
    public:

        using Context=typename ContextTraits::Context;
        using CallbackEc=db::AsyncCallbackEc<Context>;
        using CallbackList=db::AsyncCallbackList<Context>;
        using CallbackOid=db::AsyncCallbackOid<Context>;
        template <typename ModelT>
        using CallbackObj=db::AsyncCallbackObj<Context,ModelT>;

        LocalUserControllerImpl(
            std::shared_ptr<db::ModelsWrapper> modelsWrapper
        );

        LocalUserControllerImpl();

        ~LocalUserControllerImpl();
        LocalUserControllerImpl(const LocalUserControllerImpl&)=delete;
        LocalUserControllerImpl(LocalUserControllerImpl&&)=default;
        LocalUserControllerImpl& operator=(const LocalUserControllerImpl&)=delete;
        LocalUserControllerImpl& operator=(LocalUserControllerImpl&&)=default;

        void addUser(
            common::SharedPtr<Context> ctx,
            CallbackOid callback,
            common::SharedPtr<user::managed> usr,
            db::Topic topic
        );

        void findUser(
            common::SharedPtr<Context> ctx,
            CallbackObj<user::managed> callback,
            const du::ObjectId& userId,
            db::Topic topic
        ) const;

        void addLogin(
            common::SharedPtr<Context> ctx,
            CallbackOid callback,
            common::SharedPtr<login_profile::managed> login,
            db::Topic topic
        );

        void findLogin(
            common::SharedPtr<Context> ctx,
            CallbackObj<login_profile::managed> callback,
            lib::string_view login,
            db::Topic topic
        ) const;

    private:

        std::shared_ptr<LocalUserController_p<ContextTraits>> d;

        template <typename ContextTraits1>
        friend class LocalUserController_p;
};

template <typename ContextTraits>
using LocalUserController=UserController<ContextTraits,LocalUserControllerImpl<ContextTraits>>;

HATN_SERVERAPP_NAMESPACE_END

#endif // HATNLOCALUSERCONTROLLER_H
