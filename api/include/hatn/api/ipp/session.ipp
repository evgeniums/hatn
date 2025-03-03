/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/ipp/session.ipp
  *
  */

/****************************************************************************/

#ifndef HATNAPICLIENTREQUEST_IPP
#define HATNAPICLIENTREQUEST_IPP

#include <hatn/dataunit/wirebufsolid.h>
#include <hatn/dataunit/visitors.h>

#include <hatn/api/authunit.h>
#include <hatn/api/requestunit.h>
#include <hatn/api/client/session.h>

HATN_API_NAMESPACE_BEGIN

namespace client {

//---------------------------------------------------------------

template <typename Traits>
template <typename UnitT>
Error Session<Traits>::serializeAuthHeader(lib::string_view protocol, uint32_t protocolVersion, common::SharedPtr<UnitT> content)
{
    Error ec;
    HATN_SCOPE_GUARD(
        [&ec](){
            du::fillError(du::UnitError::SERIALIZE_ERROR,ec);
            du::RawError::setEnabledTL(false);
        }
    )
    du::RawError::setEnabledTL(true);

    AuthManaged obj;
    obj.setFieldValue(auth::protocol,protocol);
    obj.setFieldValue(auth::version,protocolVersion);
    obj.setFieldValue(auth::content,std::move(content));

    du::WireBufSolidShared buf{m_allocatorFactory};
    auto ok=du::io::serializeAsSubunit(obj,buf,request::session_auth.id());
    if (ok)
    {
        m_authHeader=buf.sharedMainContainer();
    }
    else
    {
        m_authHeader->reset();
    }

    return ec;
}

//---------------------------------------------------------------

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPICLIENTREQUEST_IPP
