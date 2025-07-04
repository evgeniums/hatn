/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file serverapp/ipp/localusercontroller.ipp
  */

/****************************************************************************/

#ifndef HATNLOCALUSERCONTROLLER_IPP
#define HATNLOCALUSERCONTROLLER_IPP

#include <hatn/common/meta/chain.h>

#include <hatn/db/dberror.h>

#include <hatn/clientserver/clientservererror.h>

#include <hatn/serverapp/userdbmodels.h>
#include <hatn/serverapp/localusercontroller.h>

HATN_SERVERAPP_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

template <typename ContextTraits>
class LocalUserController_p
{
    public:

        using Context=typename ContextTraits::Context;

        LocalUserController_p(
                std::shared_ptr<db::ModelsWrapper> wrp
            )
        {
            dbModelsWrapper=std::dynamic_pointer_cast<UserDbModels>(std::move(wrp));
            Assert(dbModelsWrapper,"Invalid USER database models dbModelsWrapper, must be serverapp::UserDbModels");
        }

        std::shared_ptr<UserDbModels> dbModelsWrapper;
};

//--------------------------------------------------------------------------

template <typename ContextTraits>
LocalUserControllerImpl<ContextTraits>::LocalUserControllerImpl(
        std::shared_ptr<db::ModelsWrapper> modelsWrapper
    ) : d(std::make_shared<LocalUserController_p<ContextTraits>>(std::move(modelsWrapper)))
{}

//--------------------------------------------------------------------------

template <typename ContextTraits>
LocalUserControllerImpl<ContextTraits>::LocalUserControllerImpl()
    : d(std::make_shared<LocalUserController_p<ContextTraits>>(std::make_shared<UserDbModels>()))
{}

//--------------------------------------------------------------------------

template <typename ContextTraits>
LocalUserControllerImpl<ContextTraits>::~LocalUserControllerImpl()
{}

//--------------------------------------------------------------------------

template <typename ContextTraits>
void LocalUserControllerImpl<ContextTraits>::addUser(
        common::SharedPtr<Context> ctx,
        CallbackOid callback,
        common::SharedPtr<user::managed> usr,
        db::Topic topic
    )
{
    db::AsyncModelController<ContextTraits>::create(
        std::move(ctx),
        std::move(callback),
        d->dbModelsWrapper->userModel(),
        std::move(usr),
        topic
    );
}

//--------------------------------------------------------------------------

template <typename ContextTraits>
void LocalUserControllerImpl<ContextTraits>::findUser(
        common::SharedPtr<Context> ctx,
        CallbackObj<user::managed> callback,
        const du::ObjectId& userId,
        db::Topic topic
    ) const
{
    db::AsyncModelController<ContextTraits>::read(
        std::move(ctx),
        std::move(callback),
        d->dbModelsWrapper->userModel(),
        userId,
        topic
    );
}

//--------------------------------------------------------------------------

template <typename ContextTraits>
void LocalUserControllerImpl<ContextTraits>::addLogin(
        common::SharedPtr<Context> ctx,
        CallbackOid callback,
        common::SharedPtr<login_profile::managed> login,
        db::Topic topic
    )
{
    db::AsyncModelController<ContextTraits>::create(
        std::move(ctx),
        std::move(callback),
        d->dbModelsWrapper->loginProfileModel(),
        std::move(login),
        topic
    );
}

//--------------------------------------------------------------------------

template <typename ContextTraits>
void LocalUserControllerImpl<ContextTraits>::findLogin(
        common::SharedPtr<Context> ctx,
        CallbackObj<login_profile::managed> callback,
        lib::string_view login,
        db::Topic topic
    ) const
{
    auto oid=du::ObjectId::fromString(login);
    if (oid)
    {
        callback(std::move(ctx),HATN_CLIENT_SERVER_NAMESPACE::clientServerError(HATN_CLIENT_SERVER_NAMESPACE::ClientServerError::INVALID_LOGIN_FORMAT));
        return;
    }

    db::AsyncModelController<ContextTraits>::read(
        std::move(ctx),
        std::move(callback),
        d->dbModelsWrapper->loginProfileModel(),
        oid.value(),
        topic
    );
}

//--------------------------------------------------------------------------

HATN_SERVERAPP_NAMESPACE_END

#endif // HATNLOCALUSERCONTROLLER_IPP
