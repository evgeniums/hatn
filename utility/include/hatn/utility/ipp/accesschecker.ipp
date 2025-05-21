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
#include <hatn/utility/topicdescriptor.h>
#include <hatn/utility/accesschecker.h>

HATN_UTILITY_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

template <typename ContextTraits, typename Config>
class AccessChecker_p : public std::enable_shared_from_this<AccessChecker_p<ContextTraits,Config>>
{
    public:

        using Context=typename ContextTraits::Context;
        using Callback=std::function<void (common::SharedPtr<Context>, AccessStatus, const Error&)>;

        using MacPolicyChecker=typename AccessChecker<ContextTraits,Config>::MacPolicyChecker;

        AccessChecker_p(
                std::shared_ptr<db::ModelsWrapper> dbModelsWrp
            ) : sectionDbModelsWrapper(std::make_shared<SectionDbModels>())
        {
            dbModelsWrapper=std::dynamic_pointer_cast<AclDbModels>(std::move(dbModelsWrp));
            Assert(dbModelsWrapper,"Invalid ACL database models dbModelsWrapper, must be utility::AclDbModels");
        }

        void setSectionDbModelsWrapper(std::shared_ptr<db::ModelsWrapper> wrapper)
        {
            sectionDbModelsWrapper=std::dynamic_pointer_cast<SectionDbModels>(std::move(wrapper));
            Assert(sectionDbModelsWrapper,"Invalid section database models dbModelsWrapper, must be utility::SectionDbModels");
        }

        std::shared_ptr<AclDbModels> dbModelsWrapper;
        std::shared_ptr<SectionDbModels> sectionDbModelsWrapper;

        mutable std::shared_ptr<typename AccessChecker<ContextTraits,Config>::Cache> cache;
        std::shared_ptr<typename AccessChecker<ContextTraits,Config>::SubjectHierarchy> subjHierarchy;
        std::shared_ptr<typename AccessChecker<ContextTraits,Config>::ObjectHierarchy> objHierarchy;

        std::shared_ptr<MacPolicyChecker> macPolicyChecker;

        void checkAccess(
            common::SharedPtr<Context> ctx,
            Callback callback,
            common::SharedPtr<AccessCheckerArgs> args,
            common::SharedPtr<AccessCheckerArgs> initialArgs
        ) const;

        void checkAccess(
            common::SharedPtr<Context> ctx,
            Callback callback,
            ObjectWrapperRef object,
            const Operation* operation
        ) const;

        void checkAccess(
            common::SharedPtr<Context> ctx,
            Callback callback,
            ObjectWrapperRef object,
            ObjectWrapperRef subject,
            const Operation* operation
        ) const;

        void find(
            common::SharedPtr<Context> ctx,
            Callback cb,
            common::SharedPtr<AccessCheckerArgs> args,
            common::SharedPtr<AccessCheckerArgs> initialArgs
        ) const;

        void iterateSubjHierarchy(
            common::SharedPtr<Context> ctx,
            Callback callback,
            AccessStatus prevStatus,
            common::SharedPtr<AccessCheckerArgs> args,
            common::SharedPtr<AccessCheckerArgs> initialArgs
        ) const;

        void iterateObjHierarchy(
            common::SharedPtr<Context> ctx,
            Callback callback,
            AccessStatus prevStatus,
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
    ) : d(std::make_shared<AccessChecker_p<ContextTraits,Config>>(std::move(dbModelsWrapper)))
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
void AccessChecker<ContextTraits,Config>::setSectionDbModelsWrapper(std::shared_ptr<db::ModelsWrapper> wrapper)
{
    d->setSectionDbModelsWrapper(std::move(wrapper));
}

//--------------------------------------------------------------------------

template <typename ContextTraits, typename Config>
void AccessChecker<ContextTraits,Config>::checkAccess(
        common::SharedPtr<Context> ctx,
        Callback callback,
        const Operation* operation,
        lib::string_view objectTopic
    ) const
{
    auto cb=[d=d,callback=std::move(callback),operation,objectTopic=TopicType{objectTopic}]
        (auto ctx, const Error& ec, auto topicDescriptor)
    {
        if (ec)
        {
            //! @todo Log error?

            callback(std::move(ctx),AccessStatus::Deny,ec);
            return;
        }

        if (topicDescriptor->isSet(topic_descriptor::parent))
        {
            ObjectWrapperRef object{topicDescriptor->fieldValue(db::Oid).string(),
                                 topicDescriptor->fieldValue(topic_descriptor::parent).string(),
                                 d->sectionDbModelsWrapper->topicModel()->info->modelIdStr()};

            d->checkAccess(
                std::move(ctx),
                std::move(callback),
                object,
                operation
            );
            return;
        }

        ObjectWrapperRef object{topicDescriptor->fieldValue(db::Oid).string(),
                             topicDescriptor->fieldValue(db::Oid).string(),
                             d->sectionDbModelsWrapper->topicModel()->info->modelIdStr()};
        d->checkAccess(
            std::move(ctx),
            std::move(callback),
            object,
            operation
        );
    };
    topicDescriptor<ContextTraits>(std::move(ctx),std::move(cb),*d->sectionDbModelsWrapper,objectTopic,objectTopic);
}

//--------------------------------------------------------------------------

template <typename ContextTraits, typename Config>
void AccessChecker<ContextTraits,Config>::checkAccess(
    common::SharedPtr<Context> ctx,
        Callback callback,
        ObjectWrapperRef object,
        const Operation* operation
    ) const
{
    checkAccess(std::move(ctx),std::move(callback),object,ContextTraits::contextSubject(ctx),operation);
}

//--------------------------------------------------------------------------

template <typename ContextTraits, typename Config>
void AccessChecker_p<ContextTraits,Config>::checkAccess(
    common::SharedPtr<Context> ctx,
    Callback callback,
    ObjectWrapperRef object,
    const Operation* operation
    ) const
{
    checkAccess(std::move(ctx),std::move(callback),object,ContextTraits::contextSubject(ctx),operation);
}

//--------------------------------------------------------------------------

template <typename ContextTraits, typename Config>
void AccessChecker<ContextTraits,Config>::checkAccess(
        common::SharedPtr<Context> ctx,
        Callback callback,
        ObjectWrapperRef object,
        ObjectWrapperRef subject,
        const Operation* operation
    ) const
{
    d->checkAccess(std::move(ctx),std::move(callback),object,subject,operation);
}

//--------------------------------------------------------------------------

template <typename ContextTraits, typename Config>
void AccessChecker_p<ContextTraits,Config>::checkAccess(
    common::SharedPtr<Context> ctx,
    Callback callback,
    ObjectWrapperRef object,
    ObjectWrapperRef subject,
    const Operation* operation
    ) const
{
    auto args=ContextTraits::contextFactory(ctx)->template createObject<AccessCheckerArgs>(
            object,subject,operation
        );
    checkAccess(std::move(ctx),std::move(callback),args,args);
}

//--------------------------------------------------------------------------

template <typename ContextTraits, typename Config>
void AccessChecker<ContextTraits,Config>::checkAccess(
        common::SharedPtr<Context> ctx,
        Callback callback,
        common::SharedPtr<AccessCheckerArgs> args,
        common::SharedPtr<AccessCheckerArgs> initialArgs
    ) const
{
    d->checkAccess(std::move(ctx),std::move(callback),std::move(args),std::move(initialArgs));
}

//--------------------------------------------------------------------------

template <typename ContextTraits, typename Config>
void AccessChecker_p<ContextTraits,Config>::checkAccess(
        common::SharedPtr<Context> ctx,
        Callback callback,
        common::SharedPtr<AccessCheckerArgs> args,
        common::SharedPtr<AccessCheckerArgs> initialArgs
    ) const
{
    // try to find in cache
    if (cache)
    {
        auto self=this->shared_from_this();
        auto cacheCb=[self=std::move(self),callback,args,initialArgs](auto ctx, AccessStatus status, const Error& ec)
        {
            if (!ec && status!=AccessStatus::Unknown)
            {
                callback(std::move(ctx),status,ec);
                return;
            }

            self->find(std::move(ctx),std::move(callback),args,args,initialArgs);
        };
        cache->find(std::move(ctx),std::move(callback),std::move(args),std::move(initialArgs));
        return;
    }

    // invoke lookup
    find(std::move(ctx),std::move(callback),std::move(args),std::move(initialArgs));
}

//--------------------------------------------------------------------------

template <typename ContextTraits, typename Config>
void AccessChecker_p<ContextTraits,Config>::iterateSubjHierarchy(
        common::SharedPtr<Context> ctx,
        Callback callback,
        AccessStatus prevStatus,
        common::SharedPtr<AccessCheckerArgs> args,
        common::SharedPtr<AccessCheckerArgs> initialArgs
    ) const
{
    if (!subjHierarchy)
    {
        iterateObjHierarchy(std::move(ctx),std::move(callback),prevStatus,std::move(args),std::move(initialArgs));
        return;
    }

    auto parentCb=[self{this->shared_from_this()},initialArgs,args,callback,prevStatus](auto ctx,
                                                                                             lib::optional<HierarchyItem> parent,
                                                                                             const Error& ec,
                                                                                             auto nextCb
                                                                                             )
    {
        if (ec)
        {
            HATN_CTX_SCOPE_ERROR("failed to find subject parent for ACL")
            callback(std::move(ctx),AccessStatus::Deny,ec);
            nextCb(false);
            return;
        }

        if (!parent)
        {
            // no more parents, iterate objects
            self->iterateObjHierarchy(std::move(ctx),std::move(callback),prevStatus,std::move(args),std::move(initialArgs));
            return;
        }

        auto nextArgs=ContextTraits::contextFactory(ctx)->template createObject<AccessCheckerArgs>(
            args->object,
            parent.value(),
            args->operation
        );

        auto cb=[nextCb,callback](auto ctx, AccessStatus status, const Error& ec)
        {
            // break subject iteration if access is granted or in case of error
            if (ec || status==AccessStatus::Grant)
            {
                nextCb(false);
                callback(std::move(ctx),status,ec);
                return;
            }
            nextCb(true);
        };
        self->checkAccess(std::move(ctx),std::move(cb),std::move(nextArgs),std::move(initialArgs));
    };
    subjHierarchy->eachParent(
        std::move(ctx),
        parentCb,
        args->subject
    );
}

//--------------------------------------------------------------------------

template <typename ContextTraits, typename Config>
void AccessChecker_p<ContextTraits,Config>::iterateObjHierarchy(
    common::SharedPtr<Context> ctx,
    Callback callback,
    AccessStatus prevStatus,
    common::SharedPtr<AccessCheckerArgs> args,
    common::SharedPtr<AccessCheckerArgs> initialArgs
    ) const
{
    // break object iteration if hierarchy not set or access was denied at previous steps
    if (!objHierarchy || prevStatus==AccessStatus::Deny)
    {
        //! @todo Keep in cache denied status

        // nothing found, operation is forbidden
        callback(std::move(ctx),AccessStatus::Deny,Error{});
        return;
    }

    auto parentCb=[self{this->shared_from_this()},initialArgs,args,callback](auto ctx,
                                                                                        lib::optional<HierarchyItem> parent,
                                                                                        const Error& ec,
                                                                                        auto nextCb
                                                                                        )
    {
        // break object iteration in case of error
        if (ec)
        {
            HATN_CTX_SCOPE_ERROR("failed to find object parent for ACL")
            callback(std::move(ctx),AccessStatus::Deny,ec);
            nextCb(false);
            return;
        }

        if (!parent)
        {
            //! @todo Keep in cache denied status

            // no more parents, operation forbidden
            callback(std::move(ctx),AccessStatus::Deny,Error{});
            nextCb(false);
            return;
        }

        auto nextArgs=ContextTraits::contextFactory(ctx)->template createObject<AccessCheckerArgs>(
            parent.value(),
            args->subject,
            args->operation
        );
        auto cb=[callback,nextCb](auto ctx, AccessStatus status, const Error& ec)
        {
            // break object iteration if access is either granted or denied or in case of error
            if (ec || status==AccessStatus::Grant || status==AccessStatus::Deny)
            {
                callback(std::move(ctx),status,ec);
                nextCb(false);
                return;
            }
            nextCb(true);
        };
        self->checkAccess(std::move(ctx),std::move(cb),std::move(nextArgs),std::move(initialArgs));
    };
    objHierarchy->eachParent(
        ctx,
        parentCb,
        args->object
    );
}

//--------------------------------------------------------------------------

struct ArgsThreadTopicBuilder
{
    common::SharedPtr<AccessCheckerArgs> args;

    ArgsThreadTopicBuilder(common::SharedPtr<AccessCheckerArgs> args) : args(std::move(args))
    {}

    lib::string_view operator() () const noexcept
    {
        return args->object.topic;
    }
};

template <typename ContextTraits, typename Config>
void AccessChecker_p<ContextTraits,Config>::find(
        common::SharedPtr<Context> ctx,
        Callback callback,
        common::SharedPtr<AccessCheckerArgs> args,
        common::SharedPtr<AccessCheckerArgs> initialArgs
    ) const
{
    auto self=this->shared_from_this();

    //! @todo Ensure that topics are topic IDs

    auto checkMacPolicy=[args,self,this](auto&& listRelations, auto ctx, auto callback)
    {
        if constexpr (std::is_same_v<MacPolicyChecker,MacPolicyNone>)
        {
            std::ignore=this;
            listRelations(std::move(ctx),std::move(callback));
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
                            callback(std::move(ctx),AccessStatus::Deny,ec);
                        }
                        else
                        {
                            callback(std::move(ctx),AccessStatus::Unknown,ec);
                        }
                        return;
                    }

                    listRelations(std::move(ctx),std::move(callback));
                };
                macPolicyChecker->checkMacPolicy(std::move(ctx),std::move(cb),args);
            }
            else
            {
                listRelations(std::move(ctx),std::move(callback));
            }
        }
    };

    // list subject roles for given object
    auto listRelations=[args,initialArgs,self,this](auto&& listOperations, auto ctx, auto callback)
    {
        auto listRelationsQuery=db::wrapQueryBuilder(
            [args]()
            {
                //! @todo use in_or_null for model id in relation
                return db::makeQuery(aclRelationObjSubjIdx(),
                                     db::where(acl_relation::object,db::query::eq,args->object.id).
                                     and_(acl_relation::subject,db::query::eq,args->subject.id),
                                     args->object.topic
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
                callback(std::move(ctx),AccessStatus::Deny,roles.error());
                return;
            }
            if (roles->empty())
            {
                iterateSubjHierarchy(std::move(ctx),std::move(callback),AccessStatus::Unknown,std::move(args),std::move(initialArgs));
                return;
            }

            const auto* factory=ContextTraits::contextFactory(ctx);
            auto roleIds=factory->template allocateObjectVector<du::ObjectId>();
            roleIds->reserve(roles->size());
            for (auto&& item: roles.value())
            {
                const auto* rule=item.as<acl_relation::type>();
                roleIds->push_back(rule->fieldValue(acl_relation::role));
            }

            listOperations(std::move(ctx),std::move(callback),std::move(roleIds));
        };

        auto& db=ContextTraits::contextDb(ctx);
        db.dbClient(args->object.topic)->find(
            std::move(ctx),
            std::move(cb),
            dbModelsWrapper->aclRelationModel(),
            listRelationsQuery,
            args->object.topic
        );
    };

    // list rules at operation level
    auto listOperations=[args,initialArgs,self,this]
        (auto&& checkOperations, auto ctx, auto callback, auto roles)
    {
        auto roleOperationsQuery=db::wrapQueryBuilder(
            [args,self,roles,ctx]()
            {                
                return db::makeQuery(aclRoleOperationIdx(),
                                     db::where(acl_role_operation::role,db::query::in,*roles).
                                     and_(acl_role_operation::operation,db::query::eq,args->operation->name()),
                                     args->object.topic
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
                callback(std::move(ctx),AccessStatus::Deny,dbObjResult.error());
                return;
            }

            checkOperations(std::move(ctx),std::move(callback),std::move(roles),std::move(dbObjResult));
        };

        auto& db=ContextTraits::contextDb(ctx);
        db.dbClient(args->object.topic)->find(
            std::move(ctx),
            std::move(cb),
            dbModelsWrapper->aclRoleOperationModel(),
            roleOperationsQuery,
            args->object.topic
        );
    };

    // check rules at operation level
    auto checkOperations=[self,this,args,initialArgs]
        (auto&& listOpFamilyAccess, auto ctx, auto callback, auto roles, Result<common::pmr::vector<db::DbObject>> dbObjResult) mutable
    {
        AccessStatus status=AccessStatus::Unknown;

        // evaluate access, access is granted if at least one role grants it
        if (!dbObjResult->empty())
        {
            for (auto&& item: dbObjResult.value())
            {
                const auto* roleOp=item.as<acl_role_operation::type>();
                if (roleOp->fieldValue(acl_role_operation::grant))
                {
                    status=AccessStatus::Grant;
                    break;
                }
                else
                {
                    status=AccessStatus::Deny;
                }
            }
        }

        // if access is unknown then check access to operation family
        if (status==AccessStatus::Unknown)
        {
            listOpFamilyAccess(std::move(ctx),std::move(callback),std::move(roles));
            return;
        }
        else if (status==AccessStatus::Deny)
        {
            // if denied then iterate next subject
            iterateSubjHierarchy(std::move(ctx),std::move(callback),status,std::move(args),std::move(initialArgs));
            return;
        }
    };

    // list rules at operation family level
    auto listOpFamilyAccess=[args,initialArgs,self,this](auto&& checkOpFamilyAccess, auto ctx, auto callback, auto roles)
    {
        auto query=db::wrapQueryBuilder(
            [args,self,roles=std::move(roles)]()
            {
                //! @todo look for in_or_null with familyName where null is for all operation families
                //! @todo use in_or_null model id for op damily access
                //! order desc
                return db::makeQuery(aclOpFamilyAccessIdx(),
                                     db::where(acl_op_family_access::role,db::query::in,*roles).
                                     and_(acl_op_family_access::op_family,db::query::eq,args->operation->opFamily()->familyName()),
                                     args->object.topic
                                     );
            },
            ArgsThreadTopicBuilder{args}
        );

        auto cb=[callback{std::move(callback)},args,initialArgs, checkOpFamilyAccess=std::move(checkOpFamilyAccess)]
            (auto ctx, Result<common::pmr::vector<db::DbObject>> dbObjResult) mutable
        {
            // if db error then  break iteration
            if (dbObjResult)
            {
                HATN_CTX_SCOPE_ERROR("failed to find role-operation in ACL")
                callback(std::move(ctx),AccessStatus::Deny,dbObjResult.error());
                return;
            }

            checkOpFamilyAccess(std::move(ctx),std::move(callback),dbObjResult.takeValue());
        };

        auto& db=ContextTraits::contextDb(ctx);
        db.dbClient(args->object.topic)->find(
            std::move(ctx),
            std::move(cb),
            dbModelsWrapper->aclOpFamilyAccessModel(),
            query,
            args->object.topic
        );
    };

    // check rules at operation family level
    auto checkOpFamilyAccess=[self,this,args,initialArgs]
        (auto ctx, auto callback, common::pmr::vector<db::DbObject> objVector) mutable
    {
        AccessStatus status=AccessStatus::Unknown;

        // evaluate access, access is granted if at least one role grants it
        if (!objVector.empty())
        {
            for (auto&& item: objVector)
            {
                const auto* obj=item.as<acl_op_family_access::type>();
                if (obj->fieldValue(acl_op_family_access::access) & args->operation->accessMask())
                {
                    status=AccessStatus::Grant;
                    break;
                }
                else
                {
                    status=AccessStatus::Deny;
                }
            }
        }

        // if access is unknown or denied iterate next subject
        if (status==AccessStatus::Unknown || status==AccessStatus::Deny)
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
    chain(std::move(ctx),std::move(callback));
}

//--------------------------------------------------------------------------

HATN_UTILITY_NAMESPACE_END

#endif // HATNUTILITYACLACCESSCHECKER_IPP
