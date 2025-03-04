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

#include <hatn/api/apierror.h>
#include <hatn/api/requestunit.h>
#include <hatn/api/client/request.h>

HATN_API_NAMESPACE_BEGIN

namespace client {

//---------------------------------------------------------------

template <typename SessionTraits, typename MessageT, typename RequestUnitT>
Error Request<SessionTraits,MessageT,RequestUnitT>::serialize(
        const Service& service,
        const Method& method,
        lib::string_view topic,
        const Tenancy& tenancy
    )
{
    m_unit=m_factory->template createObject<RequestUnitT>();

    auto& id=m_unit->field(request::id);
    id.mutableValue()->generate();
    m_unit->setFieldValue(request::service,service.name());
    m_unit->setFieldValue(request::service,service.name());
    m_unit->setFieldValue(request::service_version,service.version());
    m_unit->setFieldValue(request::method,method.name());
    if (!topic.empty())
    {
        m_unit->setFieldValue(request::topic,topic);
    }
    if (!tenancy.tenancyId().empty())
    {
        m_unit->setFieldValue(request::tenancy,tenancy.tenancyId());
    }

    return serialize();
}

//---------------------------------------------------------------

template <typename SessionTraits, typename MessageT, typename RequestUnitT>
void Request<SessionTraits,MessageT,RequestUnitT>::regenId()
{
    auto& id=m_unit->field(request::id);
    id.mutableValue()->generate();
    return serialize();
}

//---------------------------------------------------------------

template <typename SessionTraits, typename MessageT, typename RequestUnitT>
Error Request<SessionTraits,MessageT,RequestUnitT>::serialize(
    )
{
    auto ok=du::io::serialize(*m_unit,requestData);
    if (!ok)
    {
        return apiError(ApiLibError::FAILED_SERIALIZE_REQUEST);
    }

    return OK;
}

//---------------------------------------------------------------

template <typename SessionTraits, typename MessageT, typename RequestUnitT>
lib::string_view Request<SessionTraits,MessageT,RequestUnitT>::id() const noexcept
{
    auto& id=m_unit->field(request::id);;
    return id.value();
}

//---------------------------------------------------------------

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPICLIENTREQUEST_IPP
