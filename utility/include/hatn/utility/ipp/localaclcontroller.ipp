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
#include <hatn/utility/localaclcontroller.h>

HATN_UTILITY_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

template <typename ContextTraits, typename AccessCheckerT, typename JournalNotifyT>
class LocalAclController_p
{
    public:

        using Context=typename ContextTraits::Context;

        LocalAclController_p(
                LocalAclControllerImpl<ContextTraits,AccessCheckerT,JournalNotifyT>* ctrl,
                std::shared_ptr<db::ModelsWrapper> wrp,
                std::shared_ptr<AccessCheckerT> accessChecker,
                std::shared_ptr<JournalNotifyT> journalNotify
            ) : ctrl(ctrl),
                accessChecker(std::move(accessChecker)),
                journalNotify(std::move(journalNotify))

        {
            dbModelsWrapper=std::dynamic_pointer_cast<AclDbModels>(std::move(wrp));
            Assert(dbModelsWrapper,"Invalid ACL database models dbModelsWrapper, must be acl::AclDbModels");
        }

        LocalAclControllerImpl<ContextTraits,AccessCheckerT,JournalNotifyT>* ctrl;
        std::shared_ptr<AclDbModels> dbModelsWrapper;
        std::shared_ptr<AccessCheckerT> accessChecker;
        std::shared_ptr<JournalNotifyT> journalNotify;
};

//--------------------------------------------------------------------------

template <typename ContextTraits, typename AccessCheckerT, typename JournalNotifyT>
LocalAclControllerImpl<ContextTraits,AccessCheckerT,JournalNotifyT>::LocalAclControllerImpl(
        std::shared_ptr<db::ModelsWrapper> modelsWrapper,
        std::shared_ptr<AccessCheckerT> accessChecker,
        std::shared_ptr<JournalNotifyT> journalNotify
    ) : d(std::make_shared<LocalAclController_p<ContextTraits,AccessCheckerT,JournalNotifyT>>(this,
                                                                                              std::move(modelsWrapper),
                                                                                              std::move(accessChecker),
                                                                                              std::move(journalNotify)))
{}

//--------------------------------------------------------------------------

template <typename ContextTraits, typename AccessCheckerT, typename JournalNotifyT>
LocalAclControllerImpl<ContextTraits,AccessCheckerT,JournalNotifyT>::LocalAclControllerImpl()
    : d(std::make_shared<LocalAclController_p<ContextTraits,AccessCheckerT,JournalNotifyT>>(std::make_shared<AclDbModels>()))
{}

//--------------------------------------------------------------------------

template <typename ContextTraits, typename AccessCheckerT, typename JournalNotifyT>
LocalAclControllerImpl<ContextTraits,AccessCheckerT,JournalNotifyT>::~LocalAclControllerImpl()
{}

//--------------------------------------------------------------------------

template <typename ImplT>
struct checkTopicAccessT
{
    template <typename OpHandlerT, typename ContextT, typename CallbackT>
    void operator () (OpHandlerT&& doOp, common::SharedPtr<ContextT> ctx, CallbackT callback)
    {
        auto cb=[d=d,doOp=std::move(doOp),callback=std::move(callback),this](auto ctx,AclStatus status, Error ec) mutable
        {
            if (ec || status!=AclStatus::Grant)
            {
                if (!ec)
                {
                    ec=utilityError(UtilityError::OPERATION_FORBIDDEN);
                }
                auto cb1=[callback=std::move(callback),ec,topic=TopicType{topic}](auto ctx)
                {
                    callback(std::move(ctx),ec,du::ObjectId{});
                };
                d->journalNotify->invoke(std::move(ctx),
                                         cb1,
                                         ec,
                                         operation,
                                         du::ObjectId{},
                                         topic,
                                         model
                                         );
                return;
            }
            doOp(std::move(ctx),std::move(callback));
        };
        d->accessChecker->checkAccess(std::move(ctx),std::move(cb),operation,topic);
    };

    std::shared_ptr<ImplT> d;
    const std::string& model;
    const Operation* operation;
    TopicType topic;
};

template <typename ImplT>
auto checkTopicAccess(std::shared_ptr<ImplT> d, const std::string& model, const Operation* operation,lib::string_view topic)
{
    return checkTopicAccessT<ImplT>{std::move(d),model,operation,topic};
}

template <typename ImplT>
struct journalNotifyT
{
    template <typename ContextT, typename CallbackT>
    void operator () (
                    common::SharedPtr<ContextT> ctx,
                    CallbackT callback,
                    const du::ObjectId& oid,
                    common::pmr::vector<Parameter> params={}
                    )
    {
        if (d->journalNotify)
        {
            auto cb1=[callback=std::move(callback),oid](auto ctx)
            {
                callback(std::move(ctx),Error{},oid);
            };
            d->journalNotify->invoke(std::move(ctx),
                                     cb1,
                                     Error{},
                                     operation,
                                     oid,
                                     topic,
                                     model,
                                     std::move(params)
                                     );
            return;
        }
        callback(std::move(ctx),Error{},oid);
    };

    std::shared_ptr<ImplT> d;
    const std::string& model;
    const Operation* operation;
    TopicType topic;
};
template <typename ImplT>
auto journalNotify(std::shared_ptr<ImplT> d, const std::string& model, const Operation* operation,lib::string_view topic)
{
    return journalNotifyT<ImplT>{std::move(d),model,operation,topic};
}

struct operationChainT
{
    template <typename ImplT, typename ...Handlers>
    auto operator() (std::shared_ptr<ImplT> d, const std::string& model, const Operation* operation, db::Topic topic, Handlers&& ...handlers) const
    {
        auto chain=hatn::chain(
            checkTopicAccess(d,model,operation,topic),
            std::forward<Handlers>(handlers)...,
            journalNotify(d,model,operation,topic)
            );
        return chain;
    }
};
constexpr operationChainT operationChain{};

template <typename ContextTraits, typename AccessCheckerT, typename JournalNotifyT>
void LocalAclControllerImpl<ContextTraits,AccessCheckerT,JournalNotifyT>::addRole(
        common::SharedPtr<Context> ctx,
        CallbackOid callback,
        common::SharedPtr<acl_role::managed> role,
        db::Topic topic
    )
{
    auto create=[topic=TopicType{topic},role=std::move(role),d=d](auto&& journalNotify, auto ctx, auto callback)
    {
        auto cb=[journalNotify=std::move(journalNotify),callback=std::move(callback),topic]
            (auto ctx, const Error& ec, const du::ObjectId& oid) mutable
        {
            if (ec)
            {
                callback(std::move(ctx),ec,du::ObjectId{});
                return;
            }
            journalNotify(std::move(ctx),std::move(callback),oid);
        };
        db::AsyncModelController<ContextTraits>::create(
            std::move(ctx),
            std::move(cb),
            d->dbModelsWrapper->aclRoleModel(),
            std::move(role),
            topic
        );
    };

    auto chain=operationChain(
        d,
        d->dbModelsWrapper->aclRoleModel()->info->modelIdStr(),
        &AclOperations::addRole(),
        topic,
        std::move(create)
    );
    chain(std::move(ctx),std::move(callback));
}

//--------------------------------------------------------------------------

template <typename ContextTraits, typename AccessCheckerT, typename JournalNotifyT>
void LocalAclControllerImpl<ContextTraits,AccessCheckerT,JournalNotifyT>::readRole(
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

template <typename ContextTraits, typename AccessCheckerT, typename JournalNotifyT>
void LocalAclControllerImpl<ContextTraits,AccessCheckerT,JournalNotifyT>::removeRole(
        common::SharedPtr<Context> ctx,
        CallbackEc callback,
        const du::ObjectId& id,
        db::Topic topic
    )
{
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

template <typename ContextTraits, typename AccessCheckerT, typename JournalNotifyT>
template <typename QueryBuilderWrapperT>
void LocalAclControllerImpl<ContextTraits,AccessCheckerT,JournalNotifyT>::listRoles(
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

template <typename ContextTraits, typename AccessCheckerT, typename JournalNotifyT>
void LocalAclControllerImpl<ContextTraits,AccessCheckerT,JournalNotifyT>::updateRole(
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

template <typename ContextTraits, typename AccessCheckerT, typename JournalNotifyT>
void LocalAclControllerImpl<ContextTraits,AccessCheckerT,JournalNotifyT>::addRoleOperation(
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

template <typename ContextTraits, typename AccessCheckerT, typename JournalNotifyT>
void LocalAclControllerImpl<ContextTraits,AccessCheckerT,JournalNotifyT>::removeRoleOperation(
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

template <typename ContextTraits, typename AccessCheckerT, typename JournalNotifyT>
template <typename QueryBuilderWrapperT>
void LocalAclControllerImpl<ContextTraits,AccessCheckerT,JournalNotifyT>::listRoleOperations(
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

template <typename ContextTraits, typename AccessCheckerT, typename JournalNotifyT>
void LocalAclControllerImpl<ContextTraits,AccessCheckerT,JournalNotifyT>::addRelation(
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

template <typename ContextTraits, typename AccessCheckerT, typename JournalNotifyT>
void LocalAclControllerImpl<ContextTraits,AccessCheckerT,JournalNotifyT>::removeRelation(
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

template <typename ContextTraits, typename AccessCheckerT, typename JournalNotifyT>
template <typename QueryBuilderWrapperT>
void LocalAclControllerImpl<ContextTraits,AccessCheckerT,JournalNotifyT>::listRelations(
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
