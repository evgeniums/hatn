/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file utility/localaclcontroller.h
  */

/****************************************************************************/

#ifndef HATNLOCALACLCONTROLLER_H
#define HATNLOCALACLCONTROLLER_H

#include <hatn/db/modelsprovider.h>

#include <hatn/utility/utility.h>
#include <hatn/utility/journal.h>
#include <hatn/utility/notifier.h>
#include <hatn/utility/aclcontroller.h>
#include <hatn/utility/journalnotify.h>
#include <hatn/utility/accesschecker.h>

HATN_UTILITY_NAMESPACE_BEGIN

template <typename ContextTraits, typename AccessCheckerT=AccessChecker<ContextTraits>, typename JournalNotifyT=JournalNotifyNone>
class LocalAclController_p;

template <typename ContextTraits, typename AccessCheckerT=AccessChecker<ContextTraits>, typename JournalNotifyT=JournalNotifyNone>
class LocalAclControllerImpl
{
    public:

        using Context=typename ContextTraits::Context;
        using CallbackEc=db::AsyncCallbackEc<Context>;
        using CallbackList=db::AsyncCallbackList<Context>;
        using CallbackOid=db::AsyncCallbackOid<Context>;
        template <typename ModelT>
        using CallbackObj=db::AsyncCallbackObj<Context,ModelT>;

        LocalAclControllerImpl(
            std::shared_ptr<db::ModelsWrapper> modelsWrapper,
            std::shared_ptr<AccessCheckerT> accessChecker={},
            std::shared_ptr<JournalNotifyT> journalNotify={}
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

        void readRole(
            common::SharedPtr<Context> ctx,
            CallbackObj<acl_role::managed> callback,
            const du::ObjectId& id,
            db::Topic topic={}
        );

        void removeRole(
            common::SharedPtr<Context> ctx,
            CallbackEc callback,
            const du::ObjectId& id,
            db::Topic topic={}
        );

        template <typename QueryBuilderWrapperT>
        void listRoles(
            common::SharedPtr<Context> ctx,
            CallbackList callback,
            QueryBuilderWrapperT query,
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

        template <typename QueryBuilderWrapperT>
        void listRoleOperations(
            common::SharedPtr<Context> ctx,
            CallbackList callback,
            QueryBuilderWrapperT query,
            db::Topic topic={}
        );

        void addRelation(
            common::SharedPtr<Context> ctx,
            CallbackOid callback,
            common::SharedPtr<acl_relation::managed> role,
            db::Topic topic={}
        );

        void removeRelation(
            common::SharedPtr<Context> ctx,
            CallbackEc callback,
            const du::ObjectId& id,
            db::Topic topic={}
        );

        template <typename QueryBuilderWrapperT>
        void listRelations(
            common::SharedPtr<Context> ctx,
            CallbackList callback,
            QueryBuilderWrapperT query,
            db::Topic topic={}
        );

    private:

        std::shared_ptr<LocalAclController_p<ContextTraits,AccessCheckerT,JournalNotifyT>> d;

        template <typename ContextTraits1, typename AccessCheckerT1, typename JournalNotifyT1>
        friend class LocalAclController_p;
};

template <typename ContextTraits, typename AccessCheckerT=AccessChecker<ContextTraits>, typename JournalNotifyT=JournalNotifyNone>
using LocalAclController=AclController<typename ContextTraits::Context,LocalAclControllerImpl<ContextTraits,AccessCheckerT,JournalNotifyT>>;

HATN_UTILITY_NAMESPACE_END

#endif // HATNLOCALACLCONTROLLER_H
