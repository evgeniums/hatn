/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/server/servicedispatcher.h
  *
  */

/****************************************************************************/

#ifndef HATNAPISERVERRESPONSE_H
#define HATNAPISERVERRESPONSE_H

#include <hatn/common/sharedptr.h>
#include <hatn/common/allocatoronstack.h>

#include <hatn/api/api.h>
#include <hatn/api/responseunit.h>
#include <hatn/api/message.h>
#include <hatn/api/service.h>

HATN_API_NAMESPACE_BEGIN

namespace server {

using ResponseData=du::WireData;

class Response
{
    ResponseStatus status;
    common::StringOnStackT<ResponseCategoryNameLengthMax> categpry;
    common::SharedPtr<Message<ResponseData>> message;
};

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPISERVERRESPONSE_H
