/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/ipp/request.ipp
  *
  */

/****************************************************************************/

#ifndef HATNAPICLIENTREQUEST_IPP
#define HATNAPICLIENTREQUEST_IPP

#include <hatn/api/apiliberror.h>
#include <hatn/api/requestunit.h>
#include <hatn/api/client/request.h>

HATN_API_NAMESPACE_BEGIN

namespace client {

//---------------------------------------------------------------

template <typename SessionT, typename MessageT, typename RequestUnitT>
Error Request<SessionT,MessageT,RequestUnitT>::serialize(
        const Service& service,
        const Method& method,
        lib::string_view topic,
        const Tenancy& tenancy
    )
{
    m_unit=m_factory->template createObject<RequestUnitT>();

    auto& id=m_unit->field(protocol::request::id);
    id.mutableValue()->generate();
    m_unit->setFieldValue(protocol::request::service,service.name());
    m_unit->setFieldValue(protocol::request::service,service.name());
    m_unit->setFieldValue(protocol::request::service_version,service.version());
    m_unit->setFieldValue(protocol::request::method,method.name());
    if (!topic.empty())
    {
        m_unit->setFieldValue(protocol::request::topic,topic);
    }
    if (!tenancy.tenancyId().empty())
    {
        m_unit->setFieldValue(protocol::request::tenancy,tenancy.tenancyId());
    }
    m_unit->setFieldValue(protocol::request::message_type,m_message.typeName());

    return serialize();
}

//---------------------------------------------------------------

template <typename SessionT, typename MessageT, typename RequestUnitT>
void Request<SessionT,MessageT,RequestUnitT>::regenId()
{
    auto& id=m_unit->field(protocol::request::id);
    id.mutableValue()->generate();
    return serialize();
}

//---------------------------------------------------------------

template <typename SessionT, typename MessageT, typename RequestUnitT>
Error Request<SessionT,MessageT,RequestUnitT>::serialize(
    )
{
    auto ok=du::io::serialize(*m_unit,requestData);
    if (!ok)
    {
        return apiLibError(ApiLibError::FAILED_SERIALIZE_REQUEST);
    }

    return OK;
}

//---------------------------------------------------------------

template <typename SessionT, typename MessageT, typename RequestUnitT>
lib::string_view Request<SessionT,MessageT,RequestUnitT>::id() const noexcept
{
    auto& id=m_unit->field(protocol::request::id);;
    return id.value();
}

template <typename SessionT, typename MessageT, typename RequestUnitT>
common::Result<common::SharedPtr<ResponseManaged>> Request<SessionT,MessageT,RequestUnitT>::parseResponse() const
{
    Error ec;

    auto resp=m_factory->createObject<ResponseManaged>(m_factory);
    du::io::deserialize(*resp,responseData,ec);
    HATN_CHECK_EC(ec)

    return resp;
}

//---------------------------------------------------------------

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPICLIENTREQUEST_IPP
