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

    protocol::response::shared_type unit;
    du::WireBufChained message;

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

    template <typename MessageT>
    void setSuccessMessage(MessageT msg);
    void setSuccess();

    private:

        void setStatus(protocol::ResponseStatus status=protocol::ResponseStatus::Success, const common::ApiError* apiError=nullptr);

        template <typename EnvT1, typename RequestUnitT1>
        friend struct Request;
};

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPISERVERRESPONSE_H
