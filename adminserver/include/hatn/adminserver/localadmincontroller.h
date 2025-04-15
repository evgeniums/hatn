/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file adminserver/localadmincontroller.h
  */

/****************************************************************************/

#ifndef HATNSERVERLOCALADMINCONTROLLER_H
#define HATNSERVERLOCALADMINCONTROLLER_H

#include <hatn/db/asyncclient.h>

#include <hatn/adminclient/admincontroller.h>

#include <hatn/adminserver/adminserver.h>

HATN_ADMIN_SERVER_NAMESPACE_BEGIN

template <typename Traits>
class LocalAdminControllerImpl : public common::WithTraits<Traits>
{
    public:

        using common::WithTraits<Traits>::WithTraits;

        template <typename ContextT>
        const std::shared_ptr<db::AsyncClient>& contextDb(const common::SharedPtr<ContextT>& ctx) const
        {
            return this->traits().adminDb(ctx);
        }

        template <typename ContextT>
        db::Topic contextTopic(const common::SharedPtr<ContextT>& ctx) const
        {
            return this->traits().adminTopic(ctx);
        }

        template <typename ContextT, typename CallbackT>
        void addAdmin(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback,
            common::SharedPtr<Admin> admin
        );

        template <typename ContextT, typename CallbackT>
        void removeAdmin(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback,
            const du::ObjectId& admin
        );

        template <typename ContextT, typename CallbackT>
        void updateAdmin(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback,
            const du::ObjectId& admin,
            common::SharedPtr<db::update::Request> updateRequest
        );

        template <typename ContextT, typename CallbackT>
        void listAdmins(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback
        );

        template <typename ContextT, typename CallbackT>
        void setGroupAdmin(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback,
            const du::ObjectId& admin,
            const du::ObjectId& group,
            uint32_t role
        );

        template <typename ContextT, typename CallbackT>
        void removeAdminGroup(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback,
            const du::ObjectId& admin,
            const du::ObjectId& group
        );

        template <typename ContextT, typename CallbackT>
        void listAdminGroups(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback,
            const du::ObjectId& admin
        );

        template <typename ContextT, typename CallbackT>
        void listGroupAdmins(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback,
            const du::ObjectId& group
        );

        template <typename ContextT, typename CallbackT>
        void findAdminGroup(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback,
            const du::ObjectId& admin,
            const du::ObjectId& group
        );
};

template <typename Traits>
using LocalAdminControllerT=AdminController<LocalAdminControllerImpl<Traits>>;

HATN_ADMIN_SERVER_NAMESPACE_END

#endif // HATNSERVERLOCALADMINCONTROLLER_H
