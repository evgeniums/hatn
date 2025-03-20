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
        errorUnit->setFieldValue(protocol::response_error_message::description,apiErr->message(request->translator));
        auto data=apiErr->data();
        if (data)
        {
            errorUnit->field(protocol::response_error_message::data).set(std::move(data));
            errorUnit->setFieldValue(protocol::response_error_message::data_type,apiErr->dataType());
        }
        unit.field(protocol::response::message).set(std::move(errorUnit));
    }
}

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPISERVERRESPONSE_IPP
