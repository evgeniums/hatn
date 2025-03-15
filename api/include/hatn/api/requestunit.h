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

HDU_UNIT(request,
    HDU_FIELD(session_auth,auth::TYPE,1)
    HDU_FIELD(tenancy,HDU_TYPE_FIXED_STRING(TenancyIdLengthMax),2)
    HDU_FIELD(id,TYPE_OBJECT_ID,3,true)
    HDU_FIELD(service,HDU_TYPE_FIXED_STRING(ServiceNameLengthMax),4,true)
    HDU_FIELD(service_version,TYPE_UINT8,5,false,1)
    HDU_FIELD(method,HDU_TYPE_FIXED_STRING(MethodNameLengthMax),6,true)
    HDU_FIELD(method_auth,auth::TYPE,7)
    HDU_FIELD(topic,TYPE_STRING,8)
    HDU_FIELD(message_type,TYPE_STRING,9)
    HDU_FIELD(message,TYPE_DATAUNIT,10)
    HDU_FIELD(foreign_server,TYPE_DATAUNIT,11)
    HDU_FIELD(locale,HDU_TYPE_FIXED_STRING(LocaleNameLengthMax),12)
)

} // namespace protocol

using RequestManaged=protocol::request::shared_managed;

HATN_API_NAMESPACE_END

#endif // HATNAPIREQUESTUNIT_H
