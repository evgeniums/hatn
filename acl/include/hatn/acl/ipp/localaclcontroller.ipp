/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file acl/ipp/localaclcontroller.ipp
  */

/****************************************************************************/

#ifndef HATNLOCALACLCONTROLLER_IPP
#define HATNLOCALACLCONTROLLER_IPP

#include <hatn/acl/aclerror.h>
#include <hatn/acl/acldbmodels.h>
#include <hatn/acl/localaclcontroller.h>

HATN_ACL_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

template <typename ContextTraits>
class LocalAclController_p
{
    public:

        using Context=typename ContextTraits::Context;

        LocalAclController_p(
                LocalAclControllerImpl<ContextTraits>* ctrl,
                std::shared_ptr<db::ModelsWrapper> wrp
            ) : ctrl(ctrl)
        {
            dbModelsWrapper=std::dynamic_pointer_cast<AclDbModels>(std::move(wrp));
            Assert(dbModelsWrapper,"Invalid ACL database models dbModelsWrapper, must be acl::AclDbModels");
        }

        LocalAclControllerImpl<ContextTraits>* ctrl;
        std::shared_ptr<AclDbModels> dbModelsWrapper;

};

//--------------------------------------------------------------------------

template <typename ContextTraits>
LocalAclControllerImpl<ContextTraits>::LocalAclControllerImpl(
        std::shared_ptr<db::ModelsWrapper> modelsWrapper
    ) : d(std::make_shared<LocalAclController_p<ContextTraits>>(this,std::move(modelsWrapper)))
{}

//--------------------------------------------------------------------------

template <typename ContextTraits>
LocalAclControllerImpl<ContextTraits>::LocalAclControllerImpl()
    : d(std::make_shared<LocalAclController_p<ContextTraits>>(std::make_shared<AclDbModels>()))
{}

//--------------------------------------------------------------------------

template <typename ContextTraits>
LocalAclControllerImpl<ContextTraits>::~LocalAclControllerImpl()
{}

//--------------------------------------------------------------------------

template <typename ContextTraits>
void LocalAclControllerImpl<ContextTraits>::addRole(
        common::SharedPtr<Context> ctx,
        CallbackEc callback,
        common::SharedPtr<acl_role::managed> role,
        db::Topic topic
    )
{
    db::AsyncModelController<ContextTraits>::create(
        std::move(ctx),
        std::move(callback),
        d->dbModelsWrapper->aclRoleModel(),
        std::move(role),
        topic
    );
}

//--------------------------------------------------------------------------

template <typename ContextTraits>
void LocalAclControllerImpl<ContextTraits>::removeRole(
        common::SharedPtr<Context> ctx,
        CallbackEc callback,
        const du::ObjectId& id,
        db::Topic topic
    )
{
    db::AsyncModelController<ContextTraits>::remove(
        std::move(ctx),
        std::move(callback),
        d->dbModelsWrapper->aclRoleModel(),
        id,
        topic
    );
}

//--------------------------------------------------------------------------

template <typename ContextTraits>
void LocalAclControllerImpl<ContextTraits>::listRoles(
        common::SharedPtr<Context> ctx,
        CallbackList callback,
        db::AsyncQueryBuilder query,
        db::Topic topic
    )
{
    db::AsyncModelController<ContextTraits>::list(
        std::move(ctx),
        std::move(callback),
        d->dbModelsWrapper->aclRoleModel(),
        std::move(query),
        topic
    );
}

//--------------------------------------------------------------------------

template <typename ContextTraits>
void LocalAclControllerImpl<ContextTraits>::updateRole(
        common::SharedPtr<Context> ctx,
        CallbackEc callback,
        const du::ObjectId& id,
        common::SharedPtr<db::update::Request> request,
        db::Topic topic
    )
{
    db::AsyncModelController<ContextTraits>::update(
        std::move(ctx),
        std::move(callback),
        d->dbModelsWrapper->aclRoleModel(),
        id,
        std::move(request),
        topic
    );
}

//--------------------------------------------------------------------------

template <typename ContextTraits>
void LocalAclControllerImpl<ContextTraits>::addRoleOperation(
        common::SharedPtr<Context> ctx,
        CallbackEc callback,
        common::SharedPtr<acl_role_operation::managed> role,
        db::Topic topic
    )
{
    db::AsyncModelController<ContextTraits>::create(
        std::move(ctx),
        std::move(callback),
        d->dbModelsWrapper->aclRoleOperationModel(),
        std::move(role),
        topic
        );
}

//--------------------------------------------------------------------------

template <typename ContextTraits>
void LocalAclControllerImpl<ContextTraits>::removeRoleOperation(
        common::SharedPtr<Context> ctx,
        CallbackEc callback,
        const du::ObjectId& id,
        db::Topic topic
    )
{
    db::AsyncModelController<ContextTraits>::remove(
        std::move(ctx),
        std::move(callback),
        d->dbModelsWrapper->aclRoleOperationModel(),
        id,
        topic
    );
}

//--------------------------------------------------------------------------

template <typename ContextTraits>
void LocalAclControllerImpl<ContextTraits>::listRoleOperations(
        common::SharedPtr<Context> ctx,
        CallbackList callback,
        db::AsyncQueryBuilder query,
        db::Topic topic
    )
{
    db::AsyncModelController<ContextTraits>::list(
        std::move(ctx),
        std::move(callback),
        d->dbModelsWrapper->aclRoleOperationModel(),
        std::move(query),
        topic
    );
}

//--------------------------------------------------------------------------

template <typename ContextTraits>
void LocalAclControllerImpl<ContextTraits>::addSubjectObjectRole(
        common::SharedPtr<Context> ctx,
        CallbackEc callback,
        common::SharedPtr<acl_subject_role::managed> role,
        db::Topic topic
    )
{
    db::AsyncModelController<ContextTraits>::create(
        std::move(ctx),
        std::move(callback),
        d->dbModelsWrapper->aclSubjRoleModel(),
        std::move(role),
        topic
    );
}

//--------------------------------------------------------------------------

template <typename ContextTraits>
void LocalAclControllerImpl<ContextTraits>::removeSubjectObjectRole(
        common::SharedPtr<Context> ctx,
        CallbackEc callback,
        const du::ObjectId& id,
        db::Topic topic
    )
{
    db::AsyncModelController<ContextTraits>::remove(
        std::move(ctx),
        std::move(callback),
        d->dbModelsWrapper->aclSubjRoleModel(),
        id,
        topic
    );
}

//--------------------------------------------------------------------------

template <typename ContextTraits>
void LocalAclControllerImpl<ContextTraits>::listSubjectObjectRoles(
        common::SharedPtr<Context> ctx,
        CallbackList callback,
        db::AsyncQueryBuilder query,
        db::Topic topic
    )
{
    db::AsyncModelController<ContextTraits>::list(
        std::move(ctx),
        std::move(callback),
        d->dbModelsWrapper->aclSubjRoleModel(),
        std::move(query),
        topic
    );
}

//--------------------------------------------------------------------------

HATN_ACL_NAMESPACE_END

#endif // HATNLOCALACLCONTROLLER_IPP
