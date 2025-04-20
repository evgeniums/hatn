/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file acl/ipp/accesschecker.ipp
  */

/****************************************************************************/

#ifndef HATNACLACCESSCHECKER_IPP
#define HATNACLACCESSCHECKER_IPP

#include <hatn/common/meta/chain.h>

#include <hatn/logcontext/contextlogger.h>

#include <hatn/db/indexquery.h>

#include <hatn/acl/aclerror.h>
#include <hatn/acl/acldbmodels.h>
#include <hatn/acl/accesschecker.h>

HATN_ACL_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

template <typename ContextTraits, typename SubjectHierarchyT, typename ObjectHierarchyT, typename CacheT>
class AccessChecker_p : public std::enable_shared_from_this<AccessChecker_p<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>>
{
    public:

        using Context=typename ContextTraits::Context;
        using Callback=std::function<void (common::SharedPtr<Context>, AclStatus, const Error&)>;

        AccessChecker_p(
                const AccessChecker<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>* ctrl,
                std::shared_ptr<db::ModelsWrapper> wrp
            ) : ctrl(ctrl)
        {
            dbModelsWrapper=std::dynamic_pointer_cast<AclDbModels>(std::move(wrp));
            Assert(dbModelsWrapper,"Invalid ACL database models dbModelsWrapper, must be acl::AclDbModels");
        }

        const AccessChecker<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>* ctrl;
        std::shared_ptr<AclDbModels> dbModelsWrapper;

        mutable std::shared_ptr<typename AccessChecker<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>::Cache> cache;
        std::shared_ptr<typename AccessChecker<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>::SubjectHierarchy> subjHierarchy;
        std::shared_ptr<typename AccessChecker<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>::ObjectHierarchy> objHierarchy;

        void find(
            common::SharedPtr<Context> ctx,
            Callback cb,
            common::SharedPtr<AccessCheckerArgs> args,
            common::SharedPtr<AccessCheckerArgs> initialArgs
        ) const;

        void iterateSubjHierarchy(
            common::SharedPtr<Context> ctx,
            Callback callback,
            AclStatus prevStatus,
            common::SharedPtr<AccessCheckerArgs> args,
            common::SharedPtr<AccessCheckerArgs> initialArgs
        ) const;

        void iterateObjHierarchy(
            common::SharedPtr<Context> ctx,
            Callback callback,
            AclStatus prevStatus,
            common::SharedPtr<AccessCheckerArgs> args,
            common::SharedPtr<AccessCheckerArgs> initialArgs
        ) const;
};

//--------------------------------------------------------------------------

template <typename ContextTraits, typename SubjectHierarchyT, typename ObjectHierarchyT, typename CacheT>
AccessChecker<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>::AccessChecker(
        std::shared_ptr<db::ModelsWrapper> dbModelsWrapper,
        std::shared_ptr<SubjectHierarchy> subjHierarchy,
        std::shared_ptr<ObjectHierarchy> objHierarchy
    ) : d(std::make_shared<AccessChecker_p<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>>(this,std::move(dbModelsWrapper)))
{
    d->subjHierarchy=std::move(subjHierarchy);
    d->objHierarchy=std::move(objHierarchy);
}

//--------------------------------------------------------------------------

template <typename ContextTraits, typename SubjectHierarchyT, typename ObjectHierarchyT, typename CacheT>
AccessChecker<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>::AccessChecker(
        std::shared_ptr<SubjectHierarchy> subjHierarchy,
        std::shared_ptr<ObjectHierarchy> objHierarchy
    ) : AccessChecker<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>(std::make_shared<AclDbModels>(),std::move(subjHierarchy),std::move(objHierarchy))
{}

//--------------------------------------------------------------------------

template <typename ContextTraits, typename SubjectHierarchyT, typename ObjectHierarchyT, typename CacheT>
AccessChecker<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>::AccessChecker(
    std::shared_ptr<db::ModelsWrapper> dbModelsWrapper,
    std::shared_ptr<ObjectHierarchy> objHierarchy
    ) : AccessChecker<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>(std::move(dbModelsWrapper),{},std::move(objHierarchy))
{}

//--------------------------------------------------------------------------

template <typename ContextTraits, typename SubjectHierarchyT, typename ObjectHierarchyT, typename CacheT>
AccessChecker<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>::AccessChecker(
    std::shared_ptr<ObjectHierarchy> objHierarchy
    ) : AccessChecker<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>(std::make_shared<AclDbModels>(),{},std::move(objHierarchy))
{}

//--------------------------------------------------------------------------

template <typename ContextTraits, typename SubjectHierarchyT, typename ObjectHierarchyT, typename CacheT>
AccessChecker<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>::~AccessChecker()
{}

//--------------------------------------------------------------------------

template <typename ContextTraits, typename SubjectHierarchyT, typename ObjectHierarchyT, typename CacheT>
void AccessChecker<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>::setCache(std::shared_ptr<Cache> cache)
{
    d->cache=std::move(cache);
}

//--------------------------------------------------------------------------

template <typename ContextTraits, typename SubjectHierarchyT, typename ObjectHierarchyT, typename CacheT>
void AccessChecker<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>::checkAccess(
        common::SharedPtr<Context> ctx,
        Callback callback,
        lib::string_view object,
        lib::string_view subject,
        lib::string_view operation,
        lib::string_view topic
    ) const
{
    auto args=contextFactory(ctx)->template createObject<AccessCheckerArgs>(
        object,subject,operation,topic
    );
    checkAccess(std::move(ctx),std::move(callback),args,args);
}

struct ArgsThreadTopicBuilder
{
    common::SharedPtr<AccessCheckerArgs> args;

    ArgsThreadTopicBuilder(common::SharedPtr<AccessCheckerArgs> args) : args(std::move(args))
    {}

    lib::string_view operator() () const noexcept
    {
        return args->topic;
    }
};

//--------------------------------------------------------------------------

template <typename ContextTraits, typename SubjectHierarchyT, typename ObjectHierarchyT, typename CacheT>
void AccessChecker<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>::checkAccess(
    common::SharedPtr<Context> ctx,
    Callback callback,
    common::SharedPtr<AccessCheckerArgs> args,
    common::SharedPtr<AccessCheckerArgs> initialArgs
    ) const
{
    // try to find in cache
    if (d->cache)
    {
        auto cacheCb=[pimpl{std::weak_ptr<typename decltype(d)::element_type>(d)},callback,args,initialArgs](auto ctx, AclStatus status, const Error& ec)
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
        d->cache->find(std::move(ctx),std::move(callback),std::move(args),std::move(initialArgs));
        return;
    }

    // invoke lookup
    d->find(std::move(ctx),std::move(callback),std::move(args),std::move(initialArgs));
}

//--------------------------------------------------------------------------

template <typename ContextTraits, typename SubjectHierarchyT, typename ObjectHierarchyT, typename CacheT>
void AccessChecker_p<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>::iterateSubjHierarchy(
        common::SharedPtr<Context> ctx,
        Callback callback,
        AclStatus prevStatus,
        common::SharedPtr<AccessCheckerArgs> args,
        common::SharedPtr<AccessCheckerArgs> initialArgs
    ) const
{
    if (!subjHierarchy)
    {
        iterateObjHierarchy(std::move(ctx),std::move(callback),prevStatus,std::move(args),std::move(initialArgs));
        return;
    }

    auto parentCb=[this,self{this->shared_from_this()},initialArgs,args,callback,prevStatus](auto ctx,
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
            iterateObjHierarchy(std::move(ctx),std::move(callback),prevStatus,std::move(args),std::move(initialArgs));
            return;
        }

        auto nextArgs=ctrl->contextFactory(ctx)->template createObject<AccessCheckerArgs>(
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
        std::move(ctx),
        parentCb,
        args->subject
    );
}

//--------------------------------------------------------------------------

template <typename ContextTraits, typename SubjectHierarchyT, typename ObjectHierarchyT, typename CacheT>
void AccessChecker_p<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>::iterateObjHierarchy(
    common::SharedPtr<Context> ctx,
    Callback callback,
    AclStatus prevStatus,
    common::SharedPtr<AccessCheckerArgs> args,
    common::SharedPtr<AccessCheckerArgs> initialArgs
    ) const
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

        auto nextArgs=ctrl->contextFactory(ctx)->template createObject<AccessCheckerArgs>(
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
}

//--------------------------------------------------------------------------

template <typename ContextTraits, typename SubjectHierarchyT, typename ObjectHierarchyT, typename CacheT>
void AccessChecker_p<ContextTraits,SubjectHierarchyT,ObjectHierarchyT,CacheT>::find(
        common::SharedPtr<Context> ctx,
        Callback callback,
        common::SharedPtr<AccessCheckerArgs> args,
        common::SharedPtr<AccessCheckerArgs> initialArgs
    ) const
{
    auto self=this->shared_from_this();

    // list subject roles for given object
    auto listSubjObjRoles=[callback,args,initialArgs,self,this](auto* listRoleOperations, auto ctx)
    {
        auto listRolesQuery=db::wrapQueryBuilder(
            [args]()
            {
                return db::makeQuery(aclSubjRoleObjSubjIdx(),
                                     db::where(acl_subject_role::object,db::query::eq,args->object).
                                     and_(acl_subject_role::subject,db::query::eq,args->subject),
                                     args->topic
                                     );
            },
            ArgsThreadTopicBuilder{args}
        );

        auto cb=[listRoleOperations,callback{std::move(callback)},args,initialArgs,self,this](auto ctx, Result<common::pmr::vector<db::DbObject>> roles)
        {
            if (roles)
            {
                HATN_CTX_SCOPE_ERROR("failed to list ACL roles for object-subject pair")
                callback(std::move(ctx),AclStatus::Deny,roles.error());
                return;
            }
            if (roles->empty())
            {
                iterateSubjHierarchy(std::move(ctx),std::move(callback),AclStatus::Unknown,std::move(args),std::move(initialArgs));
                return;
            }

            (*listRoleOperations)(std::move(ctx),std::move(roles));
        };

        auto& db=ctrl->contextDb(ctx);
        db.dbClient(args->topic)->find(
            std::move(ctx),
            std::move(cb),
            dbModelsWrapper->aclSubjRoleModel(),
            listRolesQuery,
            args->topic
        );
    };

    // list operation access rules defined for found roles
    auto listRoleOperations=[callback,args,initialArgs,self,this](auto* checkRoleOperations, auto ctx, Result<common::pmr::vector<db::DbObject>> roles)
    {
        auto list=roles.takeValue();
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
            },
            ArgsThreadTopicBuilder{args}
            );

        auto cb=[checkRoleOperations,callback{std::move(callback)},args,initialArgs](auto ctx, Result<common::pmr::vector<db::DbObject>> dbObjResult)
        {
            // if db error then  break iteration
            if (dbObjResult)
            {
                HATN_CTX_SCOPE_ERROR("failed to find role-operation in ACL")
                callback(std::move(ctx),AclStatus::Deny,dbObjResult.error());
                return;
            }

            (*checkRoleOperations)(std::move(ctx),std::move(dbObjResult));
        };

        auto& db=ctrl->contextDb(ctx);
        db.dbClient(args->topic)->find(
            std::move(ctx),
            std::move(cb),
            dbModelsWrapper->aclRoleOperationModel(),
            roleOperationsQuery,
            args->topic
            );
    };

    // check access rules for found role operations
    auto checkRoleOperations=[callback,self,this,args,initialArgs](auto ctx, Result<common::pmr::vector<db::DbObject>> dbObjResult)
    {
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
            iterateSubjHierarchy(std::move(ctx),std::move(callback),status,std::move(args),std::move(initialArgs));
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

    // invoke
    auto nodes=chain::nodes(
            chain::node(std::move(listSubjObjRoles)),
            chain::node(std::move(listRoleOperations)),
            chain::node(std::move(checkRoleOperations))
        );
    auto start=chain::link(nodes);
    start(std::move(ctx));
}

//--------------------------------------------------------------------------

HATN_ACL_NAMESPACE_END

#endif // HATNACLACCESSCHECKER_IPP
