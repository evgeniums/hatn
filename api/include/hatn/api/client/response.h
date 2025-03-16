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
#include <hatn/api/makeapierror.h>
#include <hatn/api/apiliberror.h>
#include <hatn/api/genericerror.h>

HATN_API_NAMESPACE_BEGIN

namespace client
{

struct Response
{
    common::SharedPtr<ResponseManaged> unit;
    common::ByteArrayShared unitRawData;
    common::ByteArrayShared message;

    bool isSuccess() const noexcept
    {
        return unit->fieldValue(protocol::response::status)==protocol::ResponseStatus::Success;
    }

    Error parseError(const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()) const
    {
        if (isSuccess())
        {
            return Error{};
        }

        const auto& messageField=unit->field(protocol::response::message);

        // if error message field not set then construct api error from response status
        if (unit->fieldValue(protocol::response::category)!=protocol::ResponseCategoryError || !messageField.isSet())
        {
            return makeApiError(
                ApiLibError::SERVER_RESPONDED_WITH_ERROR,
                &ApiLibErrorCategory::getCategory(),
                unit->fieldValue(protocol::response::status),
                &ApiGenericErrorCategory::getCategory()
            );
        }

        // if error message field is set then parse it
        du::WireBufSolidShared buf{messageField.skippedNotParsedContent()};
        protocol::response_error_message::shared_managed errUnit{factory};
        if (!du::io::deserialize(errUnit,buf))
        {
            return apiLibError(ApiLibError::FAILED_DESERIALIZE_RESPONSE_ERROR);
        }

        // make api error from response_error_message
        auto nativeError=std::make_shared<common::NativeError>(errUnit.fieldValue(protocol::response_error_message::code));
        common::ApiError apiError{errUnit.fieldValue(protocol::response_error_message::code)};
        nativeError->setApiError(std::move(apiError));
        nativeError->mutableApiError()->setDescription(std::string{errUnit.fieldValue(protocol::response_error_message::description)});
        nativeError->mutableApiError()->setFamily(std::string{errUnit.fieldValue(protocol::response_error_message::family)});
        nativeError->mutableApiError()->setStatus(std::string{errUnit.fieldValue(protocol::response_error_message::status)});
        nativeError->mutableApiError()->setDataType(std::string{errUnit.fieldValue(protocol::response_error_message::data_type)});
        const auto& dataField=errUnit.field(protocol::response_error_message::data);
        if (dataField.isSet())
        {
            nativeError->mutableApiError()->setData(dataField.byteArrayShared());
        }
        return Error{ApiLibError::SERVER_RESPONDED_WITH_ERROR,std::move(nativeError)};
    }
};

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPICLIENTRESPONSE_H
