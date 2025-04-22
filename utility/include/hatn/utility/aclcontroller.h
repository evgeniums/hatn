/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file acl/aclcontroller.h
  */

/****************************************************************************/

#ifndef HATNACLCONTROLLER_H
#define HATNACLCONTROLLER_H

#include <hatn/common/objecttraits.h>

#include <hatn/db/update.h>
#include <hatn/db/indexquery.h>
#include <hatn/db/asyncmodelcontroller.h>

#include <hatn/utility/utility.h>
#include <hatn/utility/aclconstants.h>
#include <hatn/utility/aclmodels.h>

HATN_UTILITY_NAMESPACE_BEGIN

/**

@todo Add notifier.

Notifier should be used for
1. operations jounral
2. invalidation of acl caches

 */

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
            db::Topic topic={}
        )
        {
            this->traits().addRole(std::move(ctx),std::move(callback),std::move(role),topic);
        }

        void readRole(
            common::SharedPtr<Context> ctx,
            CallbackObj<acl_role::managed> callback,
            const du::ObjectId& id,
            db::Topic topic={}
        )
        {
            this->traits().readRole(std::move(ctx),std::move(callback),id,topic);
        }

        void removeRole(
            common::SharedPtr<Context> ctx,
            CallbackEc callback,
            const du::ObjectId& id,
            db::Topic topic={}
        )
        {
            this->traits().removeRole(std::move(ctx),std::move(callback),id,topic);
        }

        template <typename QueryBuilderWrapperT>
        void listRoles(
            common::SharedPtr<Context> ctx,
            CallbackList callback,
            QueryBuilderWrapperT query,
            db::Topic topic={}
        )
        {
            this->traits().listRoles(std::move(ctx),std::move(callback),std::move(query),topic);
        }

        void updateRole(
            common::SharedPtr<Context> ctx,
            CallbackEc callback,
            const du::ObjectId& id,
            common::SharedPtr<db::update::Request> request,
            db::Topic topic={}
        )
        {
            this->traits().updateRole(std::move(ctx),std::move(callback),id,std::move(request),topic);
        }

        void addRoleOperation(
            common::SharedPtr<Context> ctx,
            CallbackOid callback,
            common::SharedPtr<acl_role_operation::type> role,
            db::Topic topic={}
        )
        {
            this->traits().addRoleOperation(std::move(ctx),std::move(callback),std::move(role),topic);
        }

        void readRoleOperation(
            common::SharedPtr<Context> ctx,
            CallbackObj<acl_role_operation::managed> callback,
            const du::ObjectId& id,
            db::Topic topic={}
            )
        {
            this->traits().readRoleOperation(std::move(ctx),std::move(callback),id,topic);
        }

        void removeRoleOperation(
            common::SharedPtr<Context> ctx,
            CallbackEc callback,
            const du::ObjectId& id,
            db::Topic topic={}
        )
        {
            this->traits().removeRoleOperation(std::move(ctx),std::move(callback),id,topic);
        }

        template <typename QueryBuilderWrapperT>
        void listRoleOperations(
            common::SharedPtr<Context> ctx,
            CallbackList callback,
            QueryBuilderWrapperT query,
            db::Topic topic={}
        )
        {
            this->traits().listRoleOperations(std::move(ctx),std::move(callback),std::move(query),topic);
        }

        void addRelation(
            common::SharedPtr<Context> ctx,
            CallbackOid callback,
            common::SharedPtr<acl_relation::managed> role,
            db::Topic topic={}
        )
        {
            this->traits().addRelation(std::move(ctx),std::move(callback),std::move(role),topic);
        }

        void readRelation(
            common::SharedPtr<Context> ctx,
            CallbackObj<acl_relation::managed> callback,
            const du::ObjectId& id,
            db::Topic topic={}
            )
        {
            this->traits().readRelation(std::move(ctx),std::move(callback),id,topic);
        }

        void removeRelation(
            common::SharedPtr<Context> ctx,
            CallbackEc callback,
            const du::ObjectId& id,
            db::Topic topic={}
        )
        {
            this->traits().removeRelation(std::move(ctx),std::move(callback),id,topic);
        }

        template <typename QueryBuilderWrapperT>
        void listRelations(
            common::SharedPtr<Context> ctx,
            CallbackList callback,
            QueryBuilderWrapperT query,
            db::Topic topic={}
        )
        {
            this->traits().listRelations(std::move(ctx),std::move(callback),std::move(query),topic);
        }
};

HATN_UTILITY_NAMESPACE_END

#endif // HATNACLCONTROLLER_H
