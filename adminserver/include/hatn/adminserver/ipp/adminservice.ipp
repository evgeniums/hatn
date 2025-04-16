/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file adminserver/ipp/adminservice.ipp
  */

/****************************************************************************/

#ifndef HATNSERVERADMINSERVICE_IPP
#define HATNSERVERADMINSERVICE_IPP

#include <hatn/adminclient/adminserviceconfig.h>

#include <hatn/adminserver/adminserver.h>
#include <hatn/adminserver/adminservice.h>

#include <hatn/adminserver/methods/addadmin.h>

HATN_ADMIN_SERVER_NAMESPACE_BEGIN

//---------------------------------------------------------------

template <typename RequestT>
AdminServiceImpl<RequestT>::AdminServiceImpl(std::shared_ptr<ApiAdminController> controller)
{
    //! @todo Add the rest admin methods
    this->registerMethod(std::make_shared<methods::AddAdmin<RequestT>>(controller));
}

//---------------------------------------------------------------

template <typename RequestT>
AdminService<RequestT>::AdminService(std::shared_ptr<ApiAdminController> controller) : Base(AdminServiceConfig::Service, std::move(controller))
{}

//---------------------------------------------------------------

HATN_ADMIN_SERVER_NAMESPACE_END

#endif // HATNSERVERADMINSERVICE_IPP
