/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/client/response.h
  *
  */

/****************************************************************************/

#ifndef HATNAPICLIENTRESPONSE_H
#define HATNAPICLIENTRESPONSE_H

#include <hatn/api/api.h>
#include <hatn/api/responseunit.h>

HATN_API_NAMESPACE_BEGIN

namespace client
{

struct Response
{
    common::SharedPtr<ResponseManaged> unit;
    common::ByteArrayShared unitRawData;
    common::ByteArrayShared message;
};

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPICLIENTRESPONSE_H
