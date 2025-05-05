/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file adminserver/userservicecontroller.h
  */

/****************************************************************************/

#ifndef HATNUSERSERVICECONTROLLER_H
#define HATNUSERSERVICECONTROLLER_H

#include <hatn/serverapp/localusercontroller.h>

#include <hatn/adminserver/adminserver.h>
#include <hatn/adminserver/requesttraits.h>

HATN_ADMIN_SERVER_NAMESPACE_BEGIN

namespace user_service {

template <typename RequestT=HATN_API_SERVER_NAMESPACE::Request<>>
using Controller=HATN_SERVERAPP_NAMESPACE::LocalUserController<RequestTraits<RequestT>>;

}

HATN_ADMIN_SERVER_NAMESPACE_END

#endif // HATNUSERSERVICECONTROLLER_H
