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
    //! @todo Create user, default user character and temporary login

    db::AsyncModelController<ContextTraits>::create(
        std::move(ctx),
        std::move(callback),
        d->dbModelsWrapper->userModel(),
        std::move(usr),
        topic
    );
}

//--------------------------------------------------------------------------

HATN_SERVERAPP_NAMESPACE_END

#endif // HATNLOCALUSERCONTROLLER_IPP
