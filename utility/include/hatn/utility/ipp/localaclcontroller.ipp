/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file utility/ipp/localaclcontroller.ipp
  */

/****************************************************************************/

#ifndef HATNLOCALACLCONTROLLER_IPP
#define HATNLOCALACLCONTROLLER_IPP

#include <hatn/common/meta/chain.h>

#include <hatn/db/dberror.h>

#include <hatn/utility/utilityerror.h>
#include <hatn/utility/acldbmodels.h>
#include <hatn/utility/operationchain.h>
#include <hatn/utility/localaclcontroller.h>

HATN_UTILITY_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

template <typename ContextTraitsT>
class LocalAclController_p
{
    public:

        using ContextTraits=ContextTraitsT;
        using Context=typename ContextTraits::Context;

        LocalAclController_p(
                std::shared_ptr<db::ModelsWrapper> wrp
            )

        {
            dbModelsWrapper=std::dynamic_pointer_cast<AclDbModels>(std::move(wrp));
            Assert(dbModelsWrapper,"Invalid ACL database models dbModelsWrapper, must be utility::AclDbModels");
        }

        std::shared_ptr<AclDbModels> dbModelsWrapper;
};

//--------------------------------------------------------------------------

template <typename ContextTraits>
LocalAclControllerImpl<ContextTraits>::LocalAclControllerImpl(
        std::shared_ptr<db::ModelsWrapper> modelsWrapper
    ) : d(std::make_shared<LocalAclController_p<ContextTraits>>(std::move(modelsWrapper)))
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
    auto create=[topic=TopicType{topic},role=std::move(role),d=d](auto&& journalNotify, auto ctx, auto callback)
    {
        auto cb=[journalNotify=std::move(journalNotify),callback=std::move(callback)]
            (auto ctx, const Error& ec, const du::ObjectId& oid) mutable
        {
            if (ec)
            {
                callback(std::move(ctx),ec,du::ObjectId{});
                return;
            }
            auto cb1=[callback=std::move(callback),oid](auto ctx)
            {
                callback(std::move(ctx),common::Error{},oid);
            };
            journalNotify(std::move(ctx),std::move(cb1),oid);
        };
        db::AsyncModelController<ContextTraits>::create(
            std::move(ctx),
            std::move(cb),
            d->dbModelsWrapper->aclRoleModel(),
            std::move(role),
            topic
        );
    };

    auto cbEc=[callback](auto ctx, const Error& ec)
    {
        callback(std::move(ctx),ec,du::ObjectId{});
    };

    auto chain=chainAclOpJournalNotify(
        d,        
        &AclOperations::addRole(),
        topic,
        d->dbModelsWrapper->aclRoleModel()->info->modelIdStr(),
        std::move(create)
    );
    chain(std::move(ctx),std::move(cbEc),std::move(callback));
}

//--------------------------------------------------------------------------

template <typename ContextTraits>
void LocalAclControllerImpl<ContextTraits>::readRole(
        common::SharedPtr<Context> ctx,
        CallbackObj<acl_role::managed> callback,
        const du::ObjectId& id,
        db::Topic topic,
        bool noJournal
    )
{
    auto read=[d=d,noJournal,topic=TopicType{topic},id](auto&& journal, auto ctx, auto callback) mutable
    {
        auto cb=[noJournal,journal=std::move(journal),callback=std::move(callback)]
            (auto ctx, auto result) mutable
        {
            if (result || noJournal)
            {
                callback(std::move(ctx),std::move(result));
                return;
            }

            auto oid=result.value()->fieldValue(db::Oid);
            auto cb1=[callback=std::move(callback),result=std::move(result)](auto ctx) mutable
            {
                callback(std::move(ctx),std::move(result));
            };
            journal(std::move(ctx),std::move(cb1),oid);
        };

        db::AsyncModelController<ContextTraits>::read(
            std::move(ctx),
            std::move(callback),
            d->dbModelsWrapper->aclRoleModel(),
            id,
            topic
        );
    };

    auto cbEc=[callback](auto ctx, const Error& ec)
    {
        callback(std::move(ctx),Result<db::DbObjectT<acl_role::managed>>{ec});
    };

    auto chain=chainAclOpJournal(
        d,
        &AclOperations::readRole(),
        id,
        topic,
        d->dbModelsWrapper->aclRoleModel()->info->modelIdStr(),
        std::move(read)
    );
    chain(std::move(ctx),std::move(cbEc),std::move(callback));
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
    //! @todo critical: pack into chain
    //! @todo critical: Remove all role operations and relations

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
    auto list=[d=d,query=std::move(query),topic=TopicType{topic}](auto&& journalOnlyList, auto ctx, auto callback) mutable
    {
        auto cb=[d=d,journalOnlyList=std::move(journalOnlyList),callback=std::move(callback)]
            (auto ctx, auto result) mutable
        {
            if (result)
            {
                callback(std::move(ctx),std::move(result));
                return;
            }

            auto oids=ContextTraits::contextFactory(ctx)->template createObjectVector<std::reference_wrapper<const du::ObjectId>>();
            oids.reserve(result->size());
            for (size_t i=0; i<result->size(); i++)
            {
                const auto& obj=result->at(i);
                oids.emplace_back(std::cref(obj.template as<acl_role::type>()->fieldValue(db::Oid)));
            }
            auto cb1=[callback=std::move(callback),result=std::move(result)](auto ctx) mutable
            {
                callback(std::move(ctx),std::move(result));
            };
            journalOnlyList(std::move(ctx),std::move(cb1),oids);
        };

        db::AsyncModelController<ContextTraits>::list(
            std::move(ctx),
            std::move(cb),
            d->dbModelsWrapper->aclRoleModel(),
            std::move(query),
            topic
        );
    };

    auto cbEc=[callback](auto ctx, const Error& ec)
    {
        callback(std::move(ctx),Result<common::pmr::vector<db::DbObject>>{ec});
    };

    auto chain=chainAclOpJournal(
        d,
        &AclOperations::listRoles(),
        topic,
        d->dbModelsWrapper->aclRoleModel()->info->modelIdStr(),
        std::move(list)
    );
    chain(std::move(ctx),std::move(cbEc),std::move(callback));
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
                    callback(std::move(ctx),utilityError(UtilityError::UNKNOWN_ROLE),db::ObjectId{});
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
        readRole(std::move(ctx),std::move(cb),roleOperation->fieldValue(acl_role_operation::role),topic,true);
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
                    callback(std::move(ctx),utilityError(UtilityError::UNKNOWN_ROLE),db::ObjectId{});
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
        readRole(std::move(ctx),std::move(cb),subjObjRole->fieldValue(acl_relation::role),subjObjRole->fieldValue(acl_relation::role_topic),true);
    };

    //! @todo If subj and obj topic mismatch then create relation in both topics
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
    //! @todo remove relation in subj topic as well

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

HATN_UTILITY_NAMESPACE_END

#endif // HATNLOCALACLCONTROLLER_IPP
