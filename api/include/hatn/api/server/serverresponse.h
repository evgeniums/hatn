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

    const common::pmr::AllocatorFactory* factory;

    Response(
        Request<EnvT,RequestUnitT>* req
    );

    auto buffers() const
    {
        return message.buffers();
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

template <typename ErrCodeT, typename ErrorCatergoryT, typename ApiCodeT, typename ApiCategoryT, typename DataT=std::nullptr_t>
Error makeApiError(ErrCodeT code,
               const ErrorCatergoryT* errCat,
               ApiCodeT apiCode,
               const ApiCategoryT* apiCat,
               const DataT dataUnit=nullptr,
               std::string dataType={},
               const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
);

template <typename ApiCodeT, typename ApiCategoryT, typename DataT=std::nullptr_t>
Error makeApiError(const Error& ec,
    ApiCodeT apiCode,
    const ApiCategoryT* apiCat,
    const DataT dataUnit=nullptr,
    std::string dataType={},
    const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
)
{
    Assert(ec.category()!=nullptr,"Can be used only for errors of ErrorCategory category");
    return makeApiError(ec.code(),ec,ec.category(),apiCode,apiCat,dataUnit,dataType,factory);
}

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPISERVERRESPONSE_H
