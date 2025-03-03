/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/ipp/auth.ipp
  *
  */

/****************************************************************************/

#ifndef HATNAPIAUTH_IPP
#define HATNAPIAUTH_IPP

#include <hatn/dataunit/wirebufsolid.h>
#include <hatn/dataunit/visitors.h>

#include <hatn/api/authunit.h>
#include <hatn/api/requestunit.h>

#include <hatn/api/auth.h>

HATN_API_NAMESPACE_BEGIN

//---------------------------------------------------------------

template <typename UnitT>
Error Auth::serializeAuthHeader(lib::string_view protocol, uint32_t protocolVersion, common::SharedPtr<UnitT> content,
        int fieldId,
        const common::pmr::AllocatorFactory* factory
    )
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

    du::WireBufSolidShared buf{factory};
    auto ok=du::io::serializeAsSubunit(obj,buf,fieldId);
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

HATN_API_NAMESPACE_END

#endif // HATNAPIAUTH_IPP
