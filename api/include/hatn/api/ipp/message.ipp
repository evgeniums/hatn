/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/ipp/message.ipp
  *
  */

/****************************************************************************/

#ifndef HATNAPIMESSAGE_IPP
#define HATNAPIMESSAGE_IPP

#include <hatn/dataunit/visitors.h>

#include <hatn/api/apierror.h>
#include <hatn/api/requestunit.h>
#include <hatn/api/message.h>

HATN_API_NAMESPACE_BEGIN

template <typename BufT>
template <typename UnitT>
Error Message<BufT>::setContent(const UnitT& message, const common::pmr::AllocatorFactory* factory)
{
    Error ec;
    HATN_SCOPE_GUARD(
        [&ec](){
            du::fillError(du::UnitError::SERIALIZE_ERROR,ec);
            du::RawError::setEnabledTL(false);
        }
        )
    du::RawError::setEnabledTL(true);

    du::WireBufSolidShared buf{factory};
    auto ok=du::io::serializeAsSubunit(message,buf,request::session_auth.id());
    if (ok)
    {
        m_content=buf.sharedMainContainer();
    }
    else
    {
        m_content->reset();
    }

    return ec;
};

HATN_API_NAMESPACE_END

#endif // HATNAPIMESSAGE_IPP
