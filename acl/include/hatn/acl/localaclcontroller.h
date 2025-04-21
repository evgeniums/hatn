/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file acl/localaclcontroller.h
  */

/****************************************************************************/

#ifndef HATNLOCALACLCONTROLLER_H
#define HATNLOCALACLCONTROLLER_H

#include <hatn/db/modelsprovider.h>

#include <hatn/acl/acl.h>
#include <hatn/acl/aclcontroller.h>

HATN_ACL_NAMESPACE_BEGIN

template <typename ContextTraits>
class LocalAclController_p;

template <typename ContextTraits>
class LocalAclControllerImpl
{
    public:

        using Context=typename ContextTraits::Context;
        using CallbackEc=db::AsyncCallbackEc<Context>;
        using CallbackList=db::AsyncCallbackList<Context>;
        using CallbackOid=db::AsyncCallbackOid<Context>;

        LocalAclControllerImpl(
            std::shared_ptr<db::ModelsWrapper> modelsWrapper
        );

        LocalAclControllerImpl();

        ~LocalAclControllerImpl();
        LocalAclControllerImpl(const LocalAclControllerImpl&)=delete;
        LocalAclControllerImpl(LocalAclControllerImpl&&)=default;
        LocalAclControllerImpl& operator=(const LocalAclControllerImpl&)=delete;
        LocalAclControllerImpl& operator=(LocalAclControllerImpl&&)=default;

        void addRole(
            common::SharedPtr<Context> ctx,
            CallbackOid callback,
            common::SharedPtr<acl_role::managed> role,
            db::Topic topic={}
        );

        void removeRole(
            common::SharedPtr<Context> ctx,
            CallbackEc callback,
            const du::ObjectId& id,
            db::Topic topic={}
        );

        void listRoles(
            common::SharedPtr<Context> ctx,
            CallbackList callback,
            db::AsyncQueryBuilder query,
            db::Topic topic={}
        );

        void updateRole(
            common::SharedPtr<Context> ctx,
            CallbackEc callback,
            const du::ObjectId& id,
            common::SharedPtr<db::update::Request> request,
            db::Topic topic={}
        );

        void addRoleOperation(
            common::SharedPtr<Context> ctx,
            CallbackOid callback,
            common::SharedPtr<acl_role_operation::managed> role,
            db::Topic topic={}
        );

        void removeRoleOperation(
            common::SharedPtr<Context> ctx,
            CallbackEc callback,
            const du::ObjectId& id,
            db::Topic topic={}
        );

        void listRoleOperations(
            common::SharedPtr<Context> ctx,
            CallbackList callback,
            db::AsyncQueryBuilder query,
            db::Topic topic={}
        );

        void addSubjectObjectRole(
            common::SharedPtr<Context> ctx,
            CallbackOid callback,
            common::SharedPtr<acl_subject_role::managed> role,
            db::Topic topic={}
        );

        void removeSubjectObjectRole(
            common::SharedPtr<Context> ctx,
            CallbackEc callback,
            const du::ObjectId& id,
            db::Topic topic={}
        );

        void listSubjectObjectRoles(
            common::SharedPtr<Context> ctx,
            CallbackList callback,
            db::AsyncQueryBuilder query,
            db::Topic topic={}
        );

    private:

        std::shared_ptr<LocalAclController_p<ContextTraits>> d;

        template <typename ContextTraits1>
        friend class LocalAclController_p;
};

template <typename ContextTraits>
using LocalAclController=AclController<typename ContextTraits::Context,LocalAclControllerImpl<ContextTraits>>;

HATN_ACL_NAMESPACE_END

#endif // HATNLOCALACLCONTROLLER_H
