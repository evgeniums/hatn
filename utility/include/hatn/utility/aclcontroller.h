/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file utility/aclcontroller.h
  */

/****************************************************************************/

#ifndef HATNUTILITYACLCONTROLLER_H
#define HATNUTILITYACLCONTROLLER_H

#include <hatn/common/objecttraits.h>

#include <hatn/db/update.h>
#include <hatn/db/indexquery.h>
#include <hatn/db/asyncmodelcontroller.h>

#include <hatn/utility/utility.h>
#include <hatn/utility/aclconstants.h>
#include <hatn/utility/aclmodels.h>
#include <hatn/utility/systemsection.h>
#include <hatn/utility/operation.h>

HATN_UTILITY_NAMESPACE_BEGIN

/**

@todo Add notifier.

Notifier should be used for
1. operations jounral
2. invalidation of acl caches

 */


constexpr const char* AclOperationsFamily="acl";

struct HATN_UTILITY_EXPORT AclOperations : public OperarionFamily
{
    static const Operation& addRole();
    static const Operation& readRole();
    static const Operation& removeRole();
    static const Operation& listRoles();
    static const Operation& updateRole();

    static const Operation& addRoleOperation();
    static const Operation& readRoleOperation();
    static const Operation& removeRoleOperation();
    static const Operation& listRoleOperations();

    static const Operation& addRelation();
    static const Operation& readRelation();
    static const Operation& removeRelation();
    static const Operation& listRelations();

    AclOperations() : OperarionFamily(AclOperationsFamily)
    {}

    static const AclOperations& instance();
};

template <typename ContextT, typename Traits>
class AclController : public common::WithTraits<Traits>
{
    public:

        using common::WithTraits<Traits>::WithTraits;

        using Context=ContextT;
        using CallbackEc=db::AsyncCallbackEc<Context>;
        using CallbackList=db::AsyncCallbackList<Context>;
        using CallbackOid=db::AsyncCallbackOid<Context>;
        template <typename ModelT>
        using CallbackObj=db::AsyncCallbackObj<Context,ModelT>;

        void addRole(
            common::SharedPtr<Context> ctx,
            CallbackOid callback,
            common::SharedPtr<acl_role::managed> role,
            db::Topic topic=SystemTopic
        )
        {
            this->traits().addRole(std::move(ctx),std::move(callback),std::move(role),topic);
        }

        void readRole(
            common::SharedPtr<Context> ctx,
            CallbackObj<acl_role::managed> callback,
            const du::ObjectId& id,
            db::Topic topic=SystemTopic
        )
        {
            this->traits().readRole(std::move(ctx),std::move(callback),id,topic);
        }

        void removeRole(
            common::SharedPtr<Context> ctx,
            CallbackEc callback,
            const du::ObjectId& id,
            db::Topic topic=SystemTopic
        )
        {
            this->traits().removeRole(std::move(ctx),std::move(callback),id,topic);
        }

        template <typename QueryBuilderWrapperT>
        void listRoles(
            common::SharedPtr<Context> ctx,
            CallbackList callback,
            QueryBuilderWrapperT query,
            db::Topic topic=SystemTopic
        )
        {
            this->traits().listRoles(std::move(ctx),std::move(callback),std::move(query),topic);
        }

        void updateRole(
            common::SharedPtr<Context> ctx,
            CallbackEc callback,
            const du::ObjectId& id,
            common::SharedPtr<db::update::Request> request,
            db::Topic topic=SystemTopic
        )
        {
            this->traits().updateRole(std::move(ctx),std::move(callback),id,std::move(request),topic);
        }

        void addRoleOperation(
            common::SharedPtr<Context> ctx,
            CallbackOid callback,
            common::SharedPtr<acl_role_operation::type> role,
            db::Topic topic=SystemTopic
        )
        {
            this->traits().addRoleOperation(std::move(ctx),std::move(callback),std::move(role),topic);
        }

        void readRoleOperation(
            common::SharedPtr<Context> ctx,
            CallbackObj<acl_role_operation::managed> callback,
            const du::ObjectId& id,
            db::Topic topic=SystemTopic
            )
        {
            this->traits().readRoleOperation(std::move(ctx),std::move(callback),id,topic);
        }

        void removeRoleOperation(
            common::SharedPtr<Context> ctx,
            CallbackEc callback,
            const du::ObjectId& id,
            db::Topic topic=SystemTopic
        )
        {
            this->traits().removeRoleOperation(std::move(ctx),std::move(callback),id,topic);
        }

        template <typename QueryBuilderWrapperT>
        void listRoleOperations(
            common::SharedPtr<Context> ctx,
            CallbackList callback,
            QueryBuilderWrapperT query,
            db::Topic topic=SystemTopic
        )
        {
            this->traits().listRoleOperations(std::move(ctx),std::move(callback),std::move(query),topic);
        }

        void addRelation(
            common::SharedPtr<Context> ctx,
            CallbackOid callback,
            common::SharedPtr<acl_relation::managed> role,
            db::Topic topic=SystemTopic
        )
        {
            this->traits().addRelation(std::move(ctx),std::move(callback),std::move(role),topic);
        }

        void readRelation(
            common::SharedPtr<Context> ctx,
            CallbackObj<acl_relation::managed> callback,
            const du::ObjectId& id,
            db::Topic topic=SystemTopic
            )
        {
            this->traits().readRelation(std::move(ctx),std::move(callback),id,topic);
        }

        void removeRelation(
            common::SharedPtr<Context> ctx,
            CallbackEc callback,
            const du::ObjectId& id,
            db::Topic topic=SystemTopic
        )
        {
            this->traits().removeRelation(std::move(ctx),std::move(callback),id,topic);
        }

        template <typename QueryBuilderWrapperT>
        void listRelations(
            common::SharedPtr<Context> ctx,
            CallbackList callback,
            QueryBuilderWrapperT query,
            db::Topic topic=SystemTopic
        )
        {
            this->traits().listRelations(std::move(ctx),std::move(callback),std::move(query),topic);
        }
};

HATN_UTILITY_NAMESPACE_END

#endif // HATNUTILITYACLCONTROLLER_H
