/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file acl/ipp/aclcontroller.ipp
  */

/****************************************************************************/

#ifndef HATNACLCONTROLLER_IPP
#define HATNACLCONTROLLER_IPP

#include <hatn/logcontext/contextlogger.h>

#include <hatn/db/indexquery.h>

#include <hatn/acl/aclerror.h>
#include <hatn/acl/acldbmodels.h>
#include <hatn/acl/aclcontroller.h>

HATN_ACL_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

template <typename ContextTraits, typename SubjectHierarchyT, typename ObjectHierarchyT, typename CacheT>
class AclController_p : std::enable_shared_from_this<AclController_p<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>>
{
    using Context=typename ContextTraits::Context;
    using Callback=std::function<void (common::SharedPtr<Context>, const Error&)>;

    AclController_p(
            const AclController<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>* ctrl,
            std::shared_ptr<db::ModelsWrapper> wrp
        ) : ctrl(ctrl)
    {
        dbModelsWrapper=std::dynamic_pointer_cast<AclDbModels>(std::move(wrp));
        Assert(dbModelsWrapper,"Invalid ACL database models dbModelsWrapper, must be acl::AclDbModels");
    }

    const AclController<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>* ctrl;
    std::shared_ptr<AclDbModels> dbModelsWrapper;

    mutable std::shared_ptr<typename AclController<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>::Cache> cache;
    std::shared_ptr<typename AclController<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>::SubjectHierarchy> subjHierarchy;
    std::shared_ptr<typename AclController<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>::ObjectHierarchy> objHierarchy;

    void find(
        common::SharedPtr<Context> ctx,
        Callback cb,
        common::SharedPtr<AclControllerArgs> args,
        common::SharedPtr<AclControllerArgs> initialArgs
    ) const;
};

//--------------------------------------------------------------------------

template <typename ContextTraits, typename SubjectHierarchyT, typename ObjectHierarchyT, typename CacheT>
AclController<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>::AclController(
        std::shared_ptr<db::ModelsWrapper> dbModelsWrapper,
        std::shared_ptr<SubjectHierarchy> subjHierarchy,
        std::shared_ptr<ObjectHierarchy> objHierarchy
    ) : d(std::move(dbModelsWrapper))
{
    d->subjHierarchy=std::move(subjHierarchy);
    d->objHierarchy=std::move(objHierarchy);
}

//--------------------------------------------------------------------------

template <typename ContextTraits, typename SubjectHierarchyT, typename ObjectHierarchyT, typename CacheT>
AclController<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>::AclController(
        std::shared_ptr<SubjectHierarchy> subjHierarchy,
        std::shared_ptr<ObjectHierarchy> objHierarchy
    ) : AclController<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>(std::make_shared<AclDbModels>(),std::move(subjHierarchy),std::move(objHierarchy))
{}

//--------------------------------------------------------------------------

template <typename ContextTraits, typename SubjectHierarchyT, typename ObjectHierarchyT, typename CacheT>
AclController<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>::AclController(
    std::shared_ptr<db::ModelsWrapper> dbModelsWrapper,
    std::shared_ptr<ObjectHierarchy> objHierarchy
    ) : AclController<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>(std::move(dbModelsWrapper),{},std::move(objHierarchy))
{}

//--------------------------------------------------------------------------

template <typename ContextTraits, typename SubjectHierarchyT, typename ObjectHierarchyT, typename CacheT>
AclController<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>::AclController(
    std::shared_ptr<ObjectHierarchy> objHierarchy
    ) : AclController<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>(std::make_shared<AclDbModels>(),{},std::move(objHierarchy))
{}

//--------------------------------------------------------------------------

template <typename ContextTraits, typename SubjectHierarchyT, typename ObjectHierarchyT, typename CacheT>
AclController<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>::~AclController()
{}

//--------------------------------------------------------------------------

template <typename ContextTraits, typename SubjectHierarchyT, typename ObjectHierarchyT, typename CacheT>
void AclController<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>::setCache(std::shared_ptr<Cache> cache)
{
    d->cache=std::move(cache);
}

//--------------------------------------------------------------------------

template <typename ContextTraits, typename SubjectHierarchyT, typename ObjectHierarchyT, typename CacheT>
void AclController<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>::checkAccess(
        common::SharedPtr<Context> ctx,
        Callback callback,
        lib::string_view object,
        lib::string_view subject,
        lib::string_view operation,
        lib::string_view topic
    ) const
{
    auto args=contextFactory(ctx)->template createObject<AclControllerArgs>(
        object,subject,operation,topic
    );
    checkAccess(std::move(ctx),std::move(callback),args,args);
}

//--------------------------------------------------------------------------

template <typename ContextTraits, typename SubjectHierarchyT, typename ObjectHierarchyT, typename CacheT>
void AclController<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>::checkAccess(
    common::SharedPtr<Context> ctx,
    Callback callback,
    common::SharedPtr<AclControllerArgs> args,
    common::SharedPtr<AclControllerArgs> initialArgs
    ) const
{
    // try to find in cache
    if (d->cache)
    {
        auto cacheCb=[pimpl{common::toWeakPtr(d)},callback,args,initialArgs](auto ctx, AclStatus status, const Error& ec)
        {
            if (!ec && status!=AclStatus::Unknown)
            {
                callback(std::move(ctx),status,ec);
                return;
            }

            auto d=pimpl.lock();
            if (d)
            {
                d->find(std::move(ctx),std::move(callback),args,args,initialArgs);
            }
        };
        d->cache->find(std::move(ctx),std::move(callback),std::move(args));
        return;
    }

    // invoke lookup
    d->find(std::move(ctx),std::move(callback),args,args,initialArgs);
}

//--------------------------------------------------------------------------

template <typename ContextTraits, typename SubjectHierarchyT, typename ObjectHierarchyT, typename CacheT>
void AclController_p<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>::find(
        common::SharedPtr<Context> ctx,
        Callback callback,
        common::SharedPtr<AclControllerArgs> args,
        common::SharedPtr<AclControllerArgs> initialArgs
    ) const
{
    auto iterateObjHierarchy=[this,self{this->shared_from_this()},initialArgs,args,callback](auto ctx, AclStatus prevStatus)
    {
        // break object iteration if hierarchy not set or access was denied at previous steps
        if (!objHierarchy || prevStatus==AclStatus::Deny)
        {
            //! @todo Keep in cache denied status

            // nothing found, operation is forbidden
            callback(std::move(ctx),AclStatus::Deny,Error{});
            return;
        }

        auto parentCb=[this,self{this->shared_from_this()},initialArgs,args,callback](auto ctx,
                                                                                     lib::optional<lib::string_view> parent,
                                                                                     const Error& ec,
                                                                                     auto nextCb
                                                                                     )
        {
            // break object iteration in case of error
            if (ec)
            {
                HATN_CTX_SCOPE_ERROR("failed to find object parent for ACL")
                callback(std::move(ctx),AclStatus::Deny,ec);
                nextCb(false);
                return;
            }

            if (!parent)
            {
                //! @todo Keep in cache denied status

                // no more parents, operation forbidden
                callback(std::move(ctx),AclStatus::Deny,Error{});
                nextCb(false);
                return;
            }

            auto nextArgs=ctrl->contextFactory(ctx)->template createObject<AclControllerArgs>(
                parent.value(),
                args->subject,
                args->operation,
                args->topic
            );
            auto cb=[callback,nextCb](auto ctx, AclStatus status, const Error& ec)
            {
                // break object iteration if access is either granted or denied or in case of error
                if (ec || status==AclStatus::Grant || status==AclStatus::Deny)
                {
                    callback(std::move(ctx),status,ec);
                    nextCb(false);
                    return;
                }
                nextCb(true);
            };
            ctrl->checkAccess(std::move(ctx),std::move(cb),std::move(nextArgs),std::move(initialArgs));
        };
        objHierarchy->eachParent(
            ctx,
            parentCb,
            args->object
        );
    };

    auto iterateSubjHierarchy=[this,self{this->shared_from_this()},initialArgs,args,callback,iterateObjHierarchy](auto ctx, AclStatus prevStatus)
    {
        if (!subjHierarchy)
        {
            iterateObjHierarchy(std::move(ctx),std::move(args),prevStatus);
            return;
        }

        auto parentCb=[this,self{this->shared_from_this()},initialArgs,args,callback,iterateObjHierarchy,prevStatus](auto ctx,
                                                                                                        lib::optional<lib::string_view> parent,
                                                                                                        const Error& ec,
                                                                                                        auto nextCb
                                                                                                        )
        {
            if (ec)
            {
                HATN_CTX_SCOPE_ERROR("failed to find subject parent for ACL")
                callback(std::move(ctx),AclStatus::Deny,ec);
                nextCb(false);
                return;
            }

            if (!parent)
            {
                // no more parents, iterate objects
                iterateObjHierarchy(std::move(ctx),prevStatus);
                return;
            }

            auto nextArgs=ctrl->contextFactory(ctx)->template createObject<AclControllerArgs>(
                args->object,
                parent.value(),
                args->operation,
                args->topic
            );
            auto cb=[nextCb,callback](auto ctx, AclStatus status, const Error& ec)
            {
                // break subject iteration if access is granted or in case of error
                if (ec || status==AclStatus::Grant)
                {
                   nextCb(false);
                   callback(std::move(ctx),status,ec);
                   return;
                }
                nextCb(true);
            };
            ctrl->checkAccess(std::move(ctx),std::move(cb),std::move(nextArgs),std::move(initialArgs));
        };
        subjHierarchy->eachParent(
            ctx,
            parentCb,
            args->subject
        );
    };

    auto listRolesCb=[callback,iterateSubjHierarchy,self{this->shared_from_this()},args,initialArgs,this](auto ctx, Result<common::pmr::vector<db::DbObject>> roles)
    {
        if (roles)
        {
            HATN_CTX_SCOPE_ERROR("failed to list ACL roles for object-subject pair")
            callback(std::move(ctx),AclStatus::Deny,roles.error());
            return;
        }

        if (roles->empty())
        {
            iterateSubjHierarchy(std::move(ctx));
            return;
        }
        auto list=roles.takeValue();

        // look if operation is defined for roles
        auto findOperationCb=[callback,iterateSubjHierarchy,self,this,args,initialArgs](auto ctx, Result<common::pmr::vector<db::DbObject>> dbObjResult)
        {
            // db error, break iteration
            if (dbObjResult)
            {
                HATN_CTX_SCOPE_ERROR("failed to find role-operation in ACL")
                callback(std::move(ctx),AclStatus::Deny,dbObjResult.error());
                return;
            }
            AclStatus status=AclStatus::Unknown;

            // evaluate access, access is granted if at least one role grants it
            if (!dbObjResult->empty())
            {
                for (auto&& item: dbObjResult.value())
                {
                    const auto* roleOp=item.as<acl_role_operation::type>();
                    if (roleOp->fieldValue(acl_role_operation::grant_ndeny))
                    {
                        status=AclStatus::Grant;
                        break;
                    }
                    else
                    {
                        status=AclStatus::Deny;
                    }
                }
            }

            // if not granted then iterate next subject
            if (status==AclStatus::Unknown || status==AclStatus::Deny)
            {
                iterateSubjHierarchy(std::move(ctx),status);
                return;
            }

            // iteration complete
            if (cache)
            {
                // keep in cache
                auto cacheCb=[status,callback](auto ctx, const Error&)
                {
                    // done
                    callback(std::move(ctx),status,Error{});
                };
                cache->set(std::move(ctx),std::move(cacheCb),std::move(args),std::move(initialArgs),status);
            }
            else
            {
                // done
                callback(std::move(ctx),status,Error{});
            }
        };
        auto roleOperationsQuery=db::wrapQueryBuilder(
            [args,self,list{list}]()
            {
                std::vector<du::ObjectId> roles;
                roles.reserve(list.size());
                for (auto&& item: list)
                {
                    const auto* rule=item.as<acl_subject_role::type>();
                    roles.push_back(rule->fieldValue(acl_subject_role::role));
                }

                return db::makeQuery(aclRoleOperationIdx(),
                                     db::where(acl_role_operation::role,db::query::in,roles).
                                     and_(acl_role_operation::operation,db::query::eq,args->operation),
                                     args->topic
                                     );
            }
        );
        auto& db=ctrl->contextDb(ctx);
        db.find(
            std::move(ctx),
            std::move(findOperationCb),
            dbModelsWrapper->aclRoleOperationModel(),
            roleOperationsQuery,
            args->topic
        );
    };

    // list roles defined for object-subject
    auto listRolesQuery=db::wrapQueryBuilder(
        [args,self{this->shared_from_this()}]()
        {
            return db::makeQuery(aclSubjRoleObjSubjIdx(),
                                 db::where(acl_subject_role::object,db::query::eq,args->object).
                                      and_(acl_subject_role::subject,db::query::eq,args->subject),
                                 args->topic
                                 );
        }
    );
    auto& db=ctrl->contextDb(ctx);
    db.find(
        std::move(ctx),
        std::move(listRolesCb),
        dbModelsWrapper->aclRoleModel(),
        listRolesQuery,
        args->topic
    );
}

//--------------------------------------------------------------------------

HATN_ACL_NAMESPACE_END

#endif // HATNACLCONTROLLER_IPP
