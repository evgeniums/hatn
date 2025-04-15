/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file adminclient/admincontroller.h
  */

/****************************************************************************/

#ifndef HATNADMINCONTROLLER_H
#define HATNADMINCONTROLLER_H

#include <hatn/common/objecttraits.h>

#include <hatn/db/update.h>

#include <hatn/adminclient/adminclient.h>
#include <hatn/adminclient/admin.h>

HATN_ADMIN_CLIENT_NAMESPACE_BEGIN

template <typename Traits>
class AdminController : public common::WithTraits<Traits>
{
    public:

        using common::WithTraits<Traits>::WithTraits;

        template <typename ContextT, typename CallbackT>
        void addAdmin(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback,
            common::SharedPtr<Admin> admin
        )
        {
            this->traits().addAdmin(std::move(ctx),std::move(callback),std::move(admin));
        }

        template <typename ContextT, typename CallbackT>
        void removeAdmin(
                common::SharedPtr<ContextT> ctx,
                CallbackT callback,
                const du::ObjectId& admin
            )
        {
            this->traits().removeAdmin(std::move(ctx),std::move(callback),admin);
        }

        template <typename ContextT, typename CallbackT>
        void updateAdmin(
                common::SharedPtr<ContextT> ctx,
                CallbackT callback,
                const du::ObjectId& admin,
                common::SharedPtr<db::update::Request> updateRequest
            )
        {
            this->traits().updateAdmin(std::move(ctx),std::move(callback),admin,std::move(updateRequest));
        }

        template <typename ContextT, typename CallbackT>
        void listAdmins(
                common::SharedPtr<ContextT> ctx,
                CallbackT callback
            )
        {
            this->traits().listAdmins(std::move(ctx),std::move(callback));
        }

        template <typename ContextT, typename CallbackT>
        void setAdminGroup(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback,
            const du::ObjectId& admin,
            const du::ObjectId& group,
            uint32_t role
            )
        {
            this->traits().setAdminGroup(std::move(ctx),std::move(callback),admin,group,role);
        }

        template <typename ContextT, typename CallbackT>
        void removeAdminGroup(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback,
            const du::ObjectId& admin,
            const du::ObjectId& group
            )
        {
            this->traits().removeAdminGroup(std::move(ctx),std::move(callback),admin,group);
        }

        template <typename ContextT, typename CallbackT>
        void listAdminGroups(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback,
            const du::ObjectId& admin
        )
        {
            this->traits().listAdminGroup(std::move(ctx),std::move(callback),admin);
        }

        template <typename ContextT, typename CallbackT>
        void listGroupAdmins(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback,
            const du::ObjectId& group
        )
        {
            this->traits().listGroupAdmins(std::move(ctx),std::move(callback),group);
        }

        template <typename ContextT, typename CallbackT>
        void findAdminGroup(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback,
            const du::ObjectId& admin,
            const du::ObjectId& group
            )
        {
            this->traits().findAdminGroup(std::move(ctx),std::move(callback),admin,group);
        }
};

HATN_ADMIN_CLIENT_NAMESPACE_END

#endif // HATNADMINCONTROLLER_H
