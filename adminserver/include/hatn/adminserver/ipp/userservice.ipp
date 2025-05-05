/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file adminserver/ipp/userservice.ipp
  */

/****************************************************************************/

#ifndef HATNUSERSERVICE_IPP
#define HATNUSERSERVICE_IPP

#include <hatn/adminclient/userserviceconstants.h>

#include <hatn/adminserver/adminserver.h>
#include <hatn/adminserver/userservice/userservice.h>
#include <hatn/adminserver/userservice/adduser.h>

HATN_ADMIN_SERVER_NAMESPACE_BEGIN

namespace user_service {

//---------------------------------------------------------------

template <typename RequestT>
ServiceImpl<RequestT>::ServiceImpl(std::shared_ptr<Controller<RequestT>> controller)
{
    this->registerMethod(std::make_shared<AddUser<RequestT>>(controller));
}

//---------------------------------------------------------------

template <typename RequestT>
Service<RequestT>::Service(std::shared_ptr<Controller<RequestT>> controller)
    : Base(UserServiceConstants::Service, std::move(controller))
{}

}

//---------------------------------------------------------------

HATN_ADMIN_SERVER_NAMESPACE_END

#endif // HATNUSERSERVICE_IPP
