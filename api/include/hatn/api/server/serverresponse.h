/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/server/serverresponse.h
  *
  */

/****************************************************************************/

#ifndef HATNAPISERVERRESPONSE_H
#define HATNAPISERVERRESPONSE_H

#include <hatn/common/sharedptr.h>
#include <hatn/common/allocatoronstack.h>
#include <hatn/common/translate.h>

#include <hatn/api/api.h>
#include <hatn/api/requestunit.h>
#include <hatn/api/responseunit.h>
#include <hatn/api/message.h>
#include <hatn/api/server/env.h>

HATN_API_NAMESPACE_BEGIN

namespace server {

template <typename EnvT, typename RequestUnitT>
struct Request;

template <typename EnvT=BasicEnv, typename RequestUnitT=protocol::request::type>
struct Response
{
    Request<EnvT,RequestUnitT>* request;

    //! @todo Init with factory
    protocol::response::shared_type unit;
    du::WireBufChained message;

    const common::pmr::AllocatorFactory* factory;

    Response(
        Request<EnvT,RequestUnitT>* req
    );

    auto buffers(const common::pmr::AllocatorFactory* factory) const
    {
        return message.chainBuffers(factory);
    }

    auto size()
    {
        return message.size();
    }

    auto status() const noexcept
    {
        return unit.fieldValue(protocol::response::status);
    }

    Error serialize();

    void setStatus(protocol::ResponseStatus status=protocol::ResponseStatus::Success, const Error& ec=Error{});
};

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPISERVERRESPONSE_H
