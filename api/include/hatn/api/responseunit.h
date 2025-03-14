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
#include <hatn/api/protocol.h>

HATN_API_NAMESPACE_BEGIN

namespace protocol
{

HDU_UNIT(response,
         HDU_FIELD(id,TYPE_OBJECT_ID,1,true)
         HDU_FIELD(status,HDU_TYPE_ENUM(ResponseStatus),2,false,protocol::ResponseStatus::OK)
         HDU_FIELD(category,HDU_TYPE_FIXED_STRING(ResponseCategoryNameLengthMax),3)
         HDU_FIELD(message,TYPE_DATAUNIT,4)
         )

HDU_UNIT(response_error,
         HDU_FIELD(code,TYPE_INT32,1,true)
         HDU_FIELD(family,HDU_TYPE_FIXED_STRING(ResponseFamilyNameLengthMax),3,true)
         HDU_FIELD(status,HDU_TYPE_FIXED_STRING(ResponseStatusLengthMax),4,true)
         HDU_FIELD(message,TYPE_STRING,5)
         )

} // namespace protocol

using ResponseManaged=protocol::response::managed;

HATN_API_NAMESPACE_END

#endif // HATNAPIRESPONSEUNIT_H
