/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/requestunit.h
  *
  */

/****************************************************************************/

#ifndef HATNAPIREQUESTUNIT_H
#define HATNAPIREQUESTUNIT_H

#include <hatn/dataunit/syntax.h>

#include <hatn/api/api.h>
#include <hatn/api/protocol.h>
#include <hatn/api/authunit.h>

HATN_API_NAMESPACE_BEGIN

namespace protocol
{

HDU_UNIT(proxy_request,
    HDU_FIELD(ip,HDU_TYPE_FIXED_STRING(IpAddressLength),1)
    HDU_FIELD(port,TYPE_UINT16,2)
)

HDU_UNIT(request,
    HDU_FIELD(session_auth,auth::TYPE,1)
    HDU_FIELD(id,TYPE_OBJECT_ID,2,true)
    HDU_FIELD(service,HDU_TYPE_FIXED_STRING(ServiceNameLengthMax),3,true)
    HDU_FIELD(service_version,TYPE_UINT32,4,false,1)
    HDU_FIELD(method,HDU_TYPE_FIXED_STRING(MethodNameLengthMax),5,true)
    HDU_FIELD(method_auth,auth::TYPE,6)
    HDU_FIELD(topic,TYPE_STRING,7)
    HDU_FIELD(message_type,TYPE_STRING,8)
    HDU_FIELD(message,TYPE_DATAUNIT,9)
    HDU_FIELD(foreign_server,TYPE_DATAUNIT,10)
    HDU_FIELD(locale,HDU_TYPE_FIXED_STRING(LocaleNameLengthMax),11)
    HDU_FIELD(tenancy,HDU_TYPE_FIXED_STRING(TenancyIdLengthMax),12)

    HDU_FIELD(proxy,proxy_request::TYPE,50)
)

} // namespace protocol

using RequestManaged=protocol::request::shared_managed;

HATN_API_NAMESPACE_END

#endif // HATNAPIREQUESTUNIT_H
