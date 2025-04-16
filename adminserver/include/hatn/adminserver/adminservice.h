/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file adminserver/adminservice.h
  */

/****************************************************************************/

#ifndef HATNADMINSERVICE_H
#define HATNADMINSERVICE_H

#include <hatn/api/server/serverservice.h>

#include <hatn/adminserver/adminserver.h>
#include <hatn/adminserver/apiadmincontroller.h>

HATN_ADMIN_SERVER_NAMESPACE_BEGIN

//---------------------------------------------------------------

template <typename RequestT>
class AdminServiceImpl : public HATN_API_SERVER_NAMESPACE::ServiceMultipleMethods<RequestT>
{
    public:

        AdminServiceImpl(std::shared_ptr<ApiAdminController> controller);
};

//---------------------------------------------------------------

template <typename RequestT>
class AdminService : public HATN_API_SERVER_NAMESPACE::ServerServiceV<AdminServiceImpl<RequestT>,RequestT>
{
    public:

        using Base=HATN_API_SERVER_NAMESPACE::ServerServiceV<AdminServiceImpl<RequestT>,RequestT>;

        AdminService(std::shared_ptr<ApiAdminController> controller);
};

//---------------------------------------------------------------

HATN_ADMIN_SERVER_NAMESPACE_END

#endif // HATNADMINSERVICE_H
