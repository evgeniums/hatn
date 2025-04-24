/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file utility/ipp/accesschecker.ipp
  */

/****************************************************************************/

#ifndef HATNUTILITYACLACCESSCHECKER_IPP
#define HATNUTILITYACLACCESSCHECKER_IPP

#include <hatn/common/meta/chain.h>

#include <hatn/logcontext/contextlogger.h>

#include <hatn/db/indexquery.h>

#include <hatn/utility/utilityerror.h>
#include <hatn/utility/acldbmodels.h>
#include <hatn/utility/accesschecker.h>

HATN_UTILITY_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

template <typename ContextTraits, typename Config>
class AccessChecker_p : public std::enable_shared_from_this<AccessChecker_p<ContextTraits,Config>>
{
    public:

        using Context=typename ContextTraits::Context;
        using Callback=std::function<void (common::SharedPtr<Context>, AclStatus, const Error&)>;

        using MacPolicyChecker=typename AccessChecker<ContextTraits,Config>::MacPolicyChecker;

        AccessChecker_p(
                const AccessChecker<ContextTraits,Config>* ctrl,
                std::shared_ptr<db::ModelsWrapper> wrp
            ) : ctrl(ctrl)
        {
            dbModelsWrapper=std::dynamic_pointer_cast<AclDbModels>(std::move(wrp));
            Assert(dbModelsWrapper,"Invalid ACL database models dbModelsWrapper, must be acl::AclDbModels");
        }

        const AccessChecker<ContextTraits,Config>* ctrl;
        std::shared_ptr<AclDbModels> dbModelsWrapper;

        mutable std::shared_ptr<typename AccessChecker<ContextTraits,Config>::Cache> cache;
        std::shared_ptr<typename AccessChecker<ContextTraits,Config>::SubjectHierarchy> subjHierarchy;
        std::shared_ptr<typename AccessChecker<ContextTraits,Config>::ObjectHierarchy> objHierarchy;

        std::shared_ptr<MacPolicyChecker> macPolicyChecker;

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

template <typename ContextTraits, typename Config>
AccessChecker<ContextTraits,Config>::AccessChecker(
        std::shared_ptr<db::ModelsWrapper> dbModelsWrapper,
        std::shared_ptr<SubjectHierarchy> subjHierarchy,
        std::shared_ptr<ObjectHierarchy> objHierarchy,
        std::shared_ptr<MacPolicyChecker> macPolicyChecker
    ) : d(std::make_shared<AccessChecker_p<ContextTraits,Config>>(this,std::move(dbModelsWrapper)))
{
    d->subjHierarchy=std::move(subjHierarchy);
    d->objHierarchy=std::move(objHierarchy);
    d->macPolicyChecker=std::move(macPolicyChecker);
}

//--------------------------------------------------------------------------

template <typename ContextTraits, typename Config>
AccessChecker<ContextTraits,Config>::AccessChecker(
        std::shared_ptr<SubjectHierarchy> subjHierarchy,
        std::shared_ptr<ObjectHierarchy> objHierarchy
    ) : AccessChecker<ContextTraits,Config>(std::make_shared<AclDbModels>(),std::move(subjHierarchy),std::move(objHierarchy))
{}

//--------------------------------------------------------------------------

template <typename ContextTraits, typename Config>
AccessChecker<ContextTraits,Config>::AccessChecker(
    std::shared_ptr<db::ModelsWrapper> dbModelsWrapper,
    std::shared_ptr<ObjectHierarchy> objHierarchy
    ) : AccessChecker<ContextTraits,Config>(std::move(dbModelsWrapper),{},std::move(objHierarchy))
{}

//--------------------------------------------------------------------------

template <typename ContextTraits, typename Config>
AccessChecker<ContextTraits,Config>::AccessChecker(
    std::shared_ptr<ObjectHierarchy> objHierarchy
    ) : AccessChecker<ContextTraits,Config>(std::make_shared<AclDbModels>(),{},std::move(objHierarchy))
{}

//--------------------------------------------------------------------------

template <typename ContextTraits, typename Config>
AccessChecker<ContextTraits,Config>::~AccessChecker()
{}

//--------------------------------------------------------------------------

template <typename ContextTraits, typename Config>
void AccessChecker<ContextTraits,Config>::setCache(std::shared_ptr<Cache> cache)
{
    d->cache=std::move(cache);
}

//--------------------------------------------------------------------------

template <typename ContextTraits, typename Config>
void AccessChecker<ContextTraits,Config>::checkAccess(
        common::SharedPtr<Context> ctx,
        Callback callback,
        lib::string_view object,
        lib::string_view subject,
        const Operation* operation,
        lib::string_view objectTopic,
        lib::string_view subjectTopic
    ) const
{
    auto args=contextFactory(ctx)->template createObject<AccessCheckerArgs>(
        object,objectTopic,subject,subjectTopic,operation
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
        return args->objectTopic;
    }
};

//--------------------------------------------------------------------------

template <typename ContextTraits, typename Config>
void AccessChecker<ContextTraits,Config>::checkAccess(
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

template <typename ContextTraits, typename Config>
void AccessChecker_p<ContextTraits,Config>::iterateSubjHierarchy(
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
                                                                                             lib::optional<HierarchyItem> parent,
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
            args->objectTopic,
            parent.value().id,
            parent.value().topic,
            args->operation
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
        args->subject,
        args->subjectTopic
    );
}

//--------------------------------------------------------------------------

template <typename ContextTraits, typename Config>
void AccessChecker_p<ContextTraits,Config>::iterateObjHierarchy(
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
                                                                                        lib::optional<HierarchyItem> parent,
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
            parent.value().id,
            parent.value().topic,
            args->subject,
            args->subjectTopic,
            args->operation
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
        args->object,
        args->objectTopic
    );
}

//--------------------------------------------------------------------------

template <typename ContextTraits, typename Config>
void AccessChecker_p<ContextTraits,Config>::find(
        common::SharedPtr<Context> ctx,
        Callback callback,
        common::SharedPtr<AccessCheckerArgs> args,
        common::SharedPtr<AccessCheckerArgs> initialArgs
    ) const
{
    auto self=this->shared_from_this();

    auto checkMacPolicy=[callback,args,self,this](auto&& listRelations, auto ctx)
    {
        if constexpr (std::is_same_v<MacPolicyChecker,MacPolicyNone>)
        {
            std::ignore=this;
            listRelations(std::move(ctx));
        }
        else
        {
            if (macPolicyChecker)
            {
                auto cb=[self=std::move(self), this, callback=std::move(callback), listRelations=std::move(listRelations)](auto ctx, const Error& ec)
                {
                    if (ec)
                    {
                        if (ec.is(UtilityError::MAC_FORBIDDEN,UtilityErrorCategory::getCategory()))
                        {
                            callback(std::move(ctx),AclStatus::Deny,ec);
                        }
                        else
                        {
                            callback(std::move(ctx),AclStatus::Unknown,ec);
                        }
                        return;
                    }

                    listRelations(std::move(ctx));
                };
                macPolicyChecker->checkMacPolicy(std::move(ctx),std::move(cb),args);
            }
            else
            {
                listRelations(std::move(ctx));
            }
        }
    };

    // list subject roles for given object
    auto listRelations=[callback,args,initialArgs,self,this](auto&& listOperations, auto ctx)
    {
        auto listRolesQuery=db::wrapQueryBuilder(
            [args]()
            {
                return db::makeQuery(aclRelationObjSubjIdx(),
                                     db::where(acl_relation::object,db::query::eq,args->object).
                                     and_(acl_relation::subject,db::query::eq,args->subject),
                                     args->objectTopic
                                     );
            },
            ArgsThreadTopicBuilder{args}
        );

        auto cb=[listOperations{std::move(listOperations)},callback{std::move(callback)},args,initialArgs,self,this]
            (auto ctx, Result<common::pmr::vector<db::DbObject>> roles) mutable
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

            const auto* factory=ctrl->contextFactory(ctx);
            auto roleIds=factory->template allocateObjectVector<du::ObjectId>();
            roleIds->reserve(roles->size());
            for (auto&& item: roles.value())
            {
                const auto* rule=item.as<acl_relation::type>();
                roleIds->push_back(rule->fieldValue(acl_relation::role));
            }

            listOperations(std::move(ctx),std::move(roleIds));
        };

        auto& db=ctrl->contextDb(ctx);
        db.dbClient(args->objectTopic)->find(
            std::move(ctx),
            std::move(cb),
            dbModelsWrapper->aclRelationModel(),
            listRolesQuery,
            args->objectTopic
        );
    };

    // list rules at operation level
    auto listOperations=[callback,args,initialArgs,self,this]
        (auto&& checkOperations, auto ctx, auto roles)
    {
        auto roleOperationsQuery=db::wrapQueryBuilder(
            [args,self,roles,ctx]()
            {
                return db::makeQuery(aclRoleOperationIdx(),
                                     db::where(acl_role_operation::role,db::query::in,*roles).
                                     and_(acl_role_operation::operation,db::query::eq,args->operation->name()),
                                     args->objectTopic
                                     );
            },
            ArgsThreadTopicBuilder{args}
        );

        auto cb=[checkOperations{std::move(checkOperations)},callback{std::move(callback)},args,initialArgs=std::move(initialArgs),roles=std::move(roles)]
                        (auto ctx, Result<common::pmr::vector<db::DbObject>> dbObjResult) mutable
        {
            // if db error then  break iteration
            if (dbObjResult)
            {
                HATN_CTX_SCOPE_ERROR("failed to find role-operation in ACL")
                callback(std::move(ctx),AclStatus::Deny,dbObjResult.error());
                return;
            }

            checkOperations(std::move(ctx),std::move(roles),std::move(dbObjResult));
        };

        auto& db=ctrl->contextDb(ctx);
        db.dbClient(args->objectTopic)->find(
            std::move(ctx),
            std::move(cb),
            dbModelsWrapper->aclRoleOperationModel(),
            roleOperationsQuery,
            args->objectTopic
        );
    };

    // check rules at operation level
    auto checkOperations=[callback,self,this,args,initialArgs]
        (auto&& listOpFamilyAccess, auto ctx, auto roles, Result<common::pmr::vector<db::DbObject>> dbObjResult) mutable
    {
        AclStatus status=AclStatus::Unknown;

        // evaluate access, access is granted if at least one role grants it
        if (!dbObjResult->empty())
        {
            for (auto&& item: dbObjResult.value())
            {
                const auto* roleOp=item.as<acl_role_operation::type>();
                if (roleOp->fieldValue(acl_role_operation::grant))
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

        // if access is unknown then check access to operation family
        if (status==AclStatus::Unknown)
        {
            listOpFamilyAccess(std::move(ctx),std::move(roles));
            return;
        }
        else if (status==AclStatus::Deny)
        {
            // if denied then iterate next subject
            iterateSubjHierarchy(std::move(ctx),std::move(callback),status,std::move(args),std::move(initialArgs));
            return;
        }
    };

    // list rules at operation family level
    auto listOpFamilyAccess=[callback,args,initialArgs,self,this](auto&& opFamilyAccess, auto ctx, auto roles)
    {
        auto query=db::wrapQueryBuilder(
            [args,self,roles=std::move(roles)]()
            {
                //! @todo look for in_or_null with familyName where null is for all operation families
                //! order desc
                return db::makeQuery(aclOpFamilyAccessIdx(),
                                     db::where(acl_op_family_access::role,db::query::in,*roles).
                                     and_(acl_op_family_access::op_family,db::query::eq,args->operation->opFamily()->familyName()),
                                     args->objectTopic
                                     );
            },
            ArgsThreadTopicBuilder{args}
        );

        auto cb=[callback{std::move(callback)},args,initialArgs, opFamilyAccess=std::move(opFamilyAccess)]
            (auto ctx, Result<common::pmr::vector<db::DbObject>> dbObjResult) mutable
        {
            // if db error then  break iteration
            if (dbObjResult)
            {
                HATN_CTX_SCOPE_ERROR("failed to find role-operation in ACL")
                callback(std::move(ctx),AclStatus::Deny,dbObjResult.error());
                return;
            }

            opFamilyAccess(std::move(ctx),dbObjResult.takeValue());
        };

        auto& db=ctrl->contextDb(ctx);
        db.dbClient(args->objectTopic)->find(
            std::move(ctx),
            std::move(cb),
            dbModelsWrapper->aclOpFamilyAccessModel(),
            query,
            args->objectTopic
        );
    };

    // check rules at operation family level
    auto checkOpFamilyAccess=[callback,self,this,args,initialArgs]
        (auto ctx, common::pmr::vector<db::DbObject> objVector) mutable
    {
        AclStatus status=AclStatus::Unknown;

        // evaluate access, access is granted if at least one role grants it
        if (!objVector.empty())
        {
            for (auto&& item: objVector)
            {
                const auto* obj=item.as<acl_op_family_access::type>();
                if (obj->fieldValue(acl_op_family_access::access) & args->operation->accessMask())
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

        // if access is unknown or denied iterate next subject
        if (status==AclStatus::Unknown || status==AclStatus::Deny)
        {
            // if denied then iterate next subject
            iterateSubjHierarchy(std::move(ctx),std::move(callback),status,std::move(args),std::move(initialArgs));
            return;
        }

        // complete iteration

        // check if cache is used
        if (cache)
        {
            // keep in cache
            auto cacheCb=[status,callback](auto ctx, const Error&)
            {
                // done after keeping in cache
                callback(std::move(ctx),status,Error{});
            };
            cache->set(std::move(ctx),std::move(cacheCb),std::move(args),std::move(initialArgs),status);
        }
        else
        {
            // done without keeping in cache
            callback(std::move(ctx),status,Error{});
        }
    };

    // invoke
    auto chain=hatn::chain(
            std::move(checkMacPolicy),
            std::move(listRelations),
            std::move(listOperations),
            std::move(checkOperations),
            std::move(listOpFamilyAccess),
            std::move(checkOpFamilyAccess)
        );
    chain(std::move(ctx));
}

//--------------------------------------------------------------------------

HATN_UTILITY_NAMESPACE_END

#endif // HATNUTILITYACLACCESSCHECKER_IPP
