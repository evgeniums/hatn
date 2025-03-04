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

#ifndef HATNAPIRESPONSEUNIT_H
#define HATNAPIRESPONSEUNIT_H

#include <hatn/dataunit/syntax.h>

#include <hatn/api/api.h>
#include <hatn/api/apiconstants.h>

HATN_API_NAMESPACE_BEGIN

enum class ResponseStatus : int
{
    OK=0,
    AuthError=1
};

HDU_UNIT(response,
    HDU_FIELD(id,TYPE_OBJECT_ID,1,true)
    HDU_FIELD(status,TYPE_UINT32,2,false,0)
    HDU_FIELD(category,HDU_TYPE_FIXED_STRING(ResponseCategoryNameLengthMax),3)
    HDU_FIELD(message,TYPE_DATAUNIT,4)
)

using ResponseManaged=response::managed;

struct Response
{
    common::SharedPtr<ResponseManaged> unit;
    common::ByteArrayShared rawData;
    common::ByteArrayShared message;
};

HATN_API_NAMESPACE_END

#endif // HATNAPIRESPONSEUNIT_H
