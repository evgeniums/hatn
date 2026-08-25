/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/responseunit.h
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
    HDU_FIELD(id,TYPE_STRING,1)
    HDU_FIELD(status,HDU_TYPE_ENUM(ResponseStatus),2,false,protocol::ResponseStatus::Success)
    HDU_FIELD(message_type,HDU_TYPE_FIXED_STRING(ResponseMsgTypeLengthMax),3)
    HDU_FIELD(message,TYPE_DATAUNIT,4)
)

// Fields 8-10 (disposition/retry_after/details) and field 2 (string_code) implement the
// terminal/retryable contract in whitemdesktop/docs/error-contract.md. string_code is the evgo
// generic_error.Code equivalent for peers that speak it (hatn's own native api families stay on
// the int `code` above); disposition/retry_after mirror evgo's Disposition/RetryAfter.
HDU_UNIT(response_error_message,
    HDU_FIELD(code,TYPE_INT32,1,true)
    HDU_FIELD(string_code,HDU_TYPE_FIXED_STRING(ResponseCodeLengthMax),2)
    HDU_FIELD(family,HDU_TYPE_FIXED_STRING(ResponseFamilyNameLengthMax),3,true)
    HDU_FIELD(status,HDU_TYPE_FIXED_STRING(ResponseStatusLengthMax),4)
    HDU_FIELD(description,TYPE_STRING,5)
    HDU_FIELD(data_type,HDU_TYPE_FIXED_STRING(UnitNameLengthMax),6)
    HDU_FIELD(data,TYPE_BYTES,7)
    HDU_FIELD(disposition,HDU_TYPE_FIXED_STRING(ResponseDispositionLengthMax),8)
    HDU_FIELD(retry_after,TYPE_INT32,9)
    HDU_FIELD(details,TYPE_STRING,10)
)

} // namespace protocol

using ResponseManaged=protocol::response::managed;
using ResponseErrorManaged=protocol::response_error_message::managed;

HATN_API_NAMESPACE_END

#endif // HATNAPIRESPONSEUNIT_H
