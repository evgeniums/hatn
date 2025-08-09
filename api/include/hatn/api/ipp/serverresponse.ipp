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

#include <hatn/api/ipp/serverrequest.ipp>

HATN_API_NAMESPACE_BEGIN

namespace server {

//---------------------------------------------------------------

template <typename EnvT, typename RequestUnitT>
Response<EnvT,RequestUnitT>::Response(
        Request<EnvT,RequestUnitT>* req
    ) : request(req),
        unit(req->env->template get<AllocatorFactory>().factory()),
        message(req->env->template get<AllocatorFactory>().factory())
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
void Response<EnvT,RequestUnitT>::setStatus(protocol::ResponseStatus status, const common::ApiError* apiError)
{
    unit.setFieldValue(protocol::response::status,status);
    if (status!=protocol::ResponseStatus::Success)
    {
        unit.field(protocol::response::message).fieldReset();
        unit.field(protocol::response::message_type).fieldReset();
    }

    if (apiError!=nullptr)
    {
        unit.setFieldValue(protocol::response::message_type,protocol::response_error_message::conf().name);

        auto errorUnit=request->env->template get<AllocatorFactory>().factory()->template createObject<protocol::response_error_message::managed>();
        errorUnit->setFieldValue(protocol::response_error_message::code,apiError->code());
        errorUnit->setFieldValue(protocol::response_error_message::family,apiError->family());
        errorUnit->setFieldValue(protocol::response_error_message::status,apiError->status());
        errorUnit->setFieldValue(protocol::response_error_message::description,apiError->message(request->translator));
        auto data=apiError->data();
        if (data)
        {
            errorUnit->field(protocol::response_error_message::data).set(std::move(data));
            errorUnit->setFieldValue(protocol::response_error_message::data_type,apiError->dataType());
        }
        unit.field(protocol::response::message).set(std::move(errorUnit));
    }
}

//---------------------------------------------------------------

template <typename EnvT, typename RequestUnitT>
void Response<EnvT,RequestUnitT>::setSuccess()
{
    unit.setFieldValue(protocol::response::status,protocol::ResponseStatus::Success);
}

//---------------------------------------------------------------

template <typename EnvT, typename RequestUnitT>
template <typename MessageT>
void Response<EnvT,RequestUnitT>::setSuccessMessage(MessageT msg)
{
    unit.setFieldValue(protocol::response::status,protocol::ResponseStatus::Success);
    unit.setFieldValue(protocol::response::message_type,msg->unitName());
    unit.field(protocol::response::message).set(std::move(msg));
}

//---------------------------------------------------------------

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPISERVERRESPONSE_IPP
