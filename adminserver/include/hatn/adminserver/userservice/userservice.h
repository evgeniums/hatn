/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file adminserver/userservice.h
  */

/****************************************************************************/

#ifndef HATNUSERSERVICE_H
#define HATNUSERSERVICE_H

#include <hatn/api/server/serverservice.h>

#include <hatn/adminserver/adminserver.h>
#include <hatn/adminserver/userservice/userservicecontroller.h>

HATN_ADMIN_SERVER_NAMESPACE_BEGIN

namespace user_service {

//---------------------------------------------------------------

template <typename RequestT>
class ServiceImpl : public HATN_API_SERVER_NAMESPACE::ServiceMultipleMethods<RequestT>
{
    public:

        ServiceImpl(std::shared_ptr<Controller<RequestT>> controller);
};

//---------------------------------------------------------------

template <typename RequestT>
class Service : public HATN_API_SERVER_NAMESPACE::ServerServiceV<ServiceImpl<RequestT>,RequestT>
{
    public:

        using Base=HATN_API_SERVER_NAMESPACE::ServerServiceV<ServiceImpl<RequestT>,RequestT>;

        Service(std::shared_ptr<Controller<RequestT>> controller);
};

}

//---------------------------------------------------------------

HATN_ADMIN_SERVER_NAMESPACE_END

#endif // HATNUSERSERVICE_H
