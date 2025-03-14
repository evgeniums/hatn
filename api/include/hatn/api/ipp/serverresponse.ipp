/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/serverresponse.ipp
  *
  */

/****************************************************************************/

#ifndef HATNAPISERVERRESPONSE_IPP
#define HATNAPISERVERRESPONSE_IPP

#include <hatn/api/server/serverresponse.h>
#include <hatn/api/server/serverrequest.h>

HATN_API_NAMESPACE_BEGIN

namespace server {

//---------------------------------------------------------------

template <typename EnvT, typename RequestUnitT>
Response<EnvT,RequestUnitT>::Response(
        Request<EnvT,RequestUnitT>* req
    ) : request(req),
        message(req->env->template get<AllocatorFactory>().factory()),
        factory(req->env->template get<AllocatorFactory>().factory())
{}

//---------------------------------------------------------------

template <typename EnvT, typename RequestUnitT>
Error Response<EnvT,RequestUnitT>::serialize()
{
    Error ec;
    du::io::serialize(unit,message,ec);
    return ec;
}

//---------------------------------------------------------------

template <typename EnvT, typename RequestUnitT>
void Response<EnvT,RequestUnitT>::setStatus(protocol::ResponseStatus status, const Error& ec)
{
    unit.setFieldValue(protocol::response::status,status);

    if (ec && ec.apiError()!=nullptr)
    {
        unit.setFieldValue(protocol::response::category,protocol::ResponseCategoryError);

        const common::ApiError* apiErr=ec.native()->apiError();

        auto errorUnit=factory->createObject<protocol::response_error_message::shared_managed>();        
        errorUnit->setFieldValue(protocol::response_error_message::code,apiErr->code());
        errorUnit->setFieldValue(protocol::response_error_message::family,apiErr->family());
        errorUnit->setFieldValue(protocol::response_error_message::status,apiErr->status());
        errorUnit->setFieldValue(protocol::response_error_message::message,apiErr->message(request->translator));
        auto data=apiErr->data();
        if (data)
        {
            errorUnit->field(protocol::response_error_message::data).set(std::move(data));
            errorUnit->setFieldValue(protocol::response_error_message::data_type,apiErr->dataType());
        }
        unit.field(protocol::response::message).set(std::move(errorUnit));
    }
}

//---------------------------------------------------------------

template <typename ErrCodeT, typename ApiCodeT, typename ErrorCatergoryT, typename ApiCategoryT, typename DataT>
Error makeApiError(ErrCodeT code,
               const ErrorCatergoryT* errCat,
               ApiCodeT apiCode,
               const ApiCategoryT* apiCat,
               const DataT dataUnit,
               std::string dataType,
               const common::pmr::AllocatorFactory* factory
    )
{
    auto nativeError=std::make_shared<common::NativeError>(static_cast<int>(apiCode),apiCat,errCat);
    if (dataUnit!=nullptr)
    {
        nativeError->apiError()->setDataType(std::move(dataType));
        du::WireBufSolidShared buf{factory};
        Error ec;
        int r=du::io::serializeAsSubunit(*dataUnit,buf,protocol::response_error_message::data);
        if (r<0)
        {
            //! @todo Log error
        }
        else
        {
            nativeError->mutableApiError()->setData(buf.sharedMainContainer());
        }
    }
    return Error{code,std::move(nativeError)};
}

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPISERVERRESPONSE_IPP
