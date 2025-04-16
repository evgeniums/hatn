/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file adminserver/ipp/localadmincontroller.ipp
  */

/****************************************************************************/

#ifndef HATNSERVERLOCALADMINCONTROLLER_IPP
#define HATNSERVERLOCALADMINCONTROLLER_IPP

#include <hatn/adminserver/adminserver.h>

#include <hatn/adminserver/admindb.h>

#include <hatn/adminserver/localadmincontroller.h>

HATN_ADMIN_SERVER_NAMESPACE_BEGIN

//---------------------------------------------------------------

template <typename Traits>
template <typename ContextT, typename CallbackT>
void LocalAdminControllerImpl<Traits>::addAdmin(
        common::SharedPtr<ContextT> ctx,
        CallbackT callback,
        common::SharedPtr<Admin> admin
    )
{
    db::initObject(*admin);

    auto cb=[callback,admin](auto ctx, const Error& ec)
    {
        callback(std::move(ctx),std::move(admin),ec);
    };

    contextDb(ctx)->create(
        ctx,
        std::move(cb),
        contextTopic(ctx),
        adminModel(),
        admin.get()
    );
}

//---------------------------------------------------------------

template <typename Traits>
template <typename ContextT, typename CallbackT>
void LocalAdminControllerImpl<Traits>::removeAdmin(
        common::SharedPtr<ContextT> ctx,
        CallbackT callback,
        const du::ObjectId& admin
    )
{
    contextDb(ctx)->deleteObject(
        ctx,
        std::move(callback),
        contextTopic(ctx),
        adminModel(),
        admin
    );
}

//---------------------------------------------------------------

template <typename Traits>
template <typename ContextT, typename CallbackT>
void LocalAdminControllerImpl<Traits>::updateAdmin(
        common::SharedPtr<ContextT> ctx,
        CallbackT callback,
        const du::ObjectId& admin,
        common::SharedPtr<db::update::Request> updateRequest
    )
{
    contextDb(ctx)->update(
        ctx,
        std::move(callback),
        contextTopic(ctx),
        adminModel(),
        admin,
        std::move(updateRequest)
    );
}

//---------------------------------------------------------------

template <typename Traits>
template <typename ContextT, typename CallbackT>
void LocalAdminControllerImpl<Traits>::listAdmins(
        common::SharedPtr<ContextT> ctx,
        CallbackT callback
    )
{
    contextDb(ctx)->findAll(
        ctx,
        std::move(callback),
        contextTopic(ctx),
        adminModel()
    );
}

//---------------------------------------------------------------

HATN_ADMIN_SERVER_NAMESPACE_END

#endif // HATNSERVERLOCALADMINCONTROLLER_IPP
