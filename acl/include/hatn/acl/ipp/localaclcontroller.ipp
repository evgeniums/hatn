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

#include <hatn/common/meta/chain.h>

#include <hatn/db/dberror.h>

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
        CallbackOid callback,
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
void LocalAclControllerImpl<ContextTraits>::readRole(
        common::SharedPtr<Context> ctx,
        CallbackObj<acl_role::managed> callback,
        const du::ObjectId& id,
        db::Topic topic
    )
{
    db::AsyncModelController<ContextTraits>::read(
        std::move(ctx),
        std::move(callback),
        d->dbModelsWrapper->aclRoleModel(),
        id,
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
    //! @todo Remove all role operations and relations

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
template <typename QueryBuilderWrapperT>
void LocalAclControllerImpl<ContextTraits>::listRoles(
        common::SharedPtr<Context> ctx,
        CallbackList callback,
        QueryBuilderWrapperT query,
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
        CallbackOid callback,
        common::SharedPtr<acl_role_operation::managed> roleOperation,
        db::Topic topic
    )
{
    auto checkRole=[this,topic](auto&& createRoleOperation,common::SharedPtr<Context> ctx, CallbackOid callback, common::SharedPtr<acl_role_operation::managed> roleOperation) mutable
    {
        auto cb=[callback=std::move(callback),roleOperation,createRoleOperation=std::move(createRoleOperation),d=d]
                (common::SharedPtr<Context> ctx, Result<db::DbObjectT<acl_role::managed>> role) mutable
        {
            // check error
            if (role)
            {
                if (role.error().is(db::DbError::NOT_FOUND,db::DbErrorCategory::getCategory()))
                {
                    callback(std::move(ctx),aclError(AclError::UNKNOWN_ROLE),db::ObjectId{});
                }
                else
                {
                    callback(std::move(ctx),std::move(role),db::ObjectId{});
                }
                return;
            }

            // role found, go to role-operation creation
            createRoleOperation(std::move(ctx),std::move(callback),std::move(roleOperation));
        };
        readRole(std::move(ctx),std::move(cb),roleOperation->fieldValue(acl_role_operation::role),topic);
    };

    auto createRoleOperation=[d=d,topic](common::SharedPtr<Context> ctx, CallbackOid callback, common::SharedPtr<acl_role_operation::managed> roleOperation) mutable
    {
        db::AsyncModelController<ContextTraits>::create(
            std::move(ctx),
            std::move(callback),
            d->dbModelsWrapper->aclRoleOperationModel(),
            std::move(roleOperation),
            topic
        );
    };

    auto chain=hatn::chain(
            std::move(checkRole),
            std::move(createRoleOperation)
        );
    chain(std::move(ctx),std::move(callback),std::move(roleOperation));
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
template <typename QueryBuilderWrapperT>
void LocalAclControllerImpl<ContextTraits>::listRoleOperations(
        common::SharedPtr<Context> ctx,
        CallbackList callback,
        QueryBuilderWrapperT query,
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
void LocalAclControllerImpl<ContextTraits>::addRelation(
        common::SharedPtr<Context> ctx,
        CallbackOid callback,
        common::SharedPtr<acl_relation::managed> subjObjRole,
        db::Topic topic
    )
{
    auto checkRole=[this,topic](auto&& createSubjObjRole,common::SharedPtr<Context> ctx, CallbackOid callback, common::SharedPtr<acl_relation::managed> subjObjRole) mutable
    {
        auto cb=[callback=std::move(callback),subjObjRole,createSubjObjRole=std::move(createSubjObjRole),d=d]
            (common::SharedPtr<Context> ctx, Result<db::DbObjectT<acl_role::managed>> role) mutable
        {
            // check error
            if (role)
            {
                if (role.error().is(db::DbError::NOT_FOUND,db::DbErrorCategory::getCategory()))
                {
                    callback(std::move(ctx),aclError(AclError::UNKNOWN_ROLE),db::ObjectId{});
                }
                else
                {
                    callback(std::move(ctx),std::move(role),db::ObjectId{});
                }
                return;
            }

            // role found, go to subj-obj-role creation
            createSubjObjRole(std::move(ctx),std::move(callback),std::move(subjObjRole));
        };
        readRole(std::move(ctx),std::move(cb),subjObjRole->fieldValue(acl_relation::role),topic);
    };

    auto createSubjObjRole=[d=d,topic](common::SharedPtr<Context> ctx, CallbackOid callback, common::SharedPtr<acl_relation::managed> subjObjRole) mutable
    {
        db::AsyncModelController<ContextTraits>::create(
            std::move(ctx),
            std::move(callback),
            d->dbModelsWrapper->aclRelationModel(),
            std::move(subjObjRole),
            topic
            );
    };

    auto chain=hatn::chain(
        std::move(checkRole),
        std::move(createSubjObjRole)
    );
    chain(std::move(ctx),std::move(callback),std::move(subjObjRole));
}

//--------------------------------------------------------------------------

template <typename ContextTraits>
void LocalAclControllerImpl<ContextTraits>::removeRelation(
        common::SharedPtr<Context> ctx,
        CallbackEc callback,
        const du::ObjectId& id,
        db::Topic topic
    )
{
    db::AsyncModelController<ContextTraits>::remove(
        std::move(ctx),
        std::move(callback),
        d->dbModelsWrapper->aclRelationModel(),
        id,
        topic
    );
}

//--------------------------------------------------------------------------

template <typename ContextTraits>
template <typename QueryBuilderWrapperT>
void LocalAclControllerImpl<ContextTraits>::listRelations(
        common::SharedPtr<Context> ctx,
        CallbackList callback,
        QueryBuilderWrapperT query,
        db::Topic topic
    )
{
    db::AsyncModelController<ContextTraits>::list(
        std::move(ctx),
        std::move(callback),
        d->dbModelsWrapper->aclRelationModel(),
        std::move(query),
        topic
    );
}

//--------------------------------------------------------------------------

HATN_ACL_NAMESPACE_END

#endif // HATNLOCALACLCONTROLLER_IPP
