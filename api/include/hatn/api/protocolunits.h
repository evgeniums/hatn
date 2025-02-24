/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/protocolunits.h
  *
  */

/****************************************************************************/

#ifndef HATNAPIPROTOUNITS_H
#define HATNAPIPROTOUNITS_H

#include <hatn/dataunit/syntax.h>

#include <hatn/api/api.h>
#include <hatn/api/apiconstants.h>

HATN_API_NAMESPACE_BEGIN

HDU_UNIT(auth,
    HDU_FIELD(protocol,HDU_TYPE_FIXED_STRING(AuthProtocolNameLengthMax),1)
    HDU_FIELD(version,TYPE_UINT8,2,false,1)
    HDU_FIELD(content,TYPE_BYTES,3)
)

HDU_UNIT(request,
    HDU_FIELD(session_auth,auth::TYPE,1)
    HDU_FIELD(id,TYPE_OBJECT_ID,2,true)
    HDU_FIELD(service,HDU_TYPE_FIXED_STRING(ServiceNameLengthMax),3,true)
    HDU_FIELD(service_version,TYPE_UINT8,4,false,1)
    HDU_FIELD(method,HDU_TYPE_FIXED_STRING(MethodNameLengthMax),5,true)
    HDU_FIELD(method_auth,auth::TYPE,6)
    HDU_FIELD(topic,TYPE_STRING,7)
    HDU_FIELD(content,TYPE_BYTES,8)
)

HDU_UNIT(response,
    HDU_FIELD(id,TYPE_OBJECT_ID,1,true)
    HDU_FIELD(status,TYPE_UINT32,2,true)
    HDU_FIELD(category,HDU_TYPE_FIXED_STRING(ResponseCategoryNameLengthMax),3)
    HDU_FIELD(content,TYPE_BYTES,4)
)

HATN_API_NAMESPACE_END

#endif // HATNAPIPROTOUNITS_H
