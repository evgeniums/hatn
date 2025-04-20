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

#include <hatn/acl/acl.h>
#include <hatn/acl/aclconstants.h>
#include <hatn/acl/aclmodels.h>

HATN_ACL_NAMESPACE_BEGIN

template <typename ContextT, typename Traits>
class AclController : public common::WithTraits<Traits>
{
    public:

        using common::WithTraits<Traits>::WithTraits;

        using Context=ContextT;
        using CallbackEc=db::AsyncCallbackEc<Context>;
        using CallbackList=db::AsyncCallbackList<Context>;

        void addRole(
            common::SharedPtr<Context> ctx,
            CallbackEc callback,
            common::SharedPtr<acl_role::managed> role,
            db::Topic topic={}
        )
        {
            this->traits().addRole(std::move(ctx),std::move(callback),std::move(role),topic);
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

        void listRoles(
            common::SharedPtr<Context> ctx,
            CallbackList callback,
            db::AsyncQueryBuilder query,
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
            CallbackEc callback,
            common::SharedPtr<acl_role_operation::type> role,
            db::Topic topic={}
        )
        {
            this->traits().addRoleOperation(std::move(ctx),std::move(callback),std::move(role),topic);
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

        void listRoleOperations(
            common::SharedPtr<Context> ctx,
            CallbackList callback,
            db::AsyncQueryBuilder query,
            db::Topic topic={}
        )
        {
            this->traits().listRoleOperations(std::move(ctx),std::move(callback),std::move(query),topic);
        }

        void addSubjectObjectRole(
            common::SharedPtr<Context> ctx,
            CallbackEc callback,
            common::SharedPtr<acl_role_operation::managed> role,
            db::Topic topic={}
        )
        {
            this->traits().addSubjectObjectRole(std::move(ctx),std::move(callback),std::move(role),topic);
        }

        void removeSubjectObjectRole(
            common::SharedPtr<Context> ctx,
            CallbackEc callback,
            const du::ObjectId& id,
            db::Topic topic={}
        )
        {
            this->traits().removeSubjectObjectRole(std::move(ctx),std::move(callback),id,topic);
        }

        void listSubjectObjectRoles(
            common::SharedPtr<Context> ctx,
            CallbackList callback,
            db::AsyncQueryBuilder query,
            db::Topic topic={}
        )
        {
            this->traits().listSubjectObjectRoles(std::move(ctx),std::move(callback),std::move(query),topic);
        }
};

HATN_ACL_NAMESPACE_END

#endif // HATNACLCONTROLLER_H
