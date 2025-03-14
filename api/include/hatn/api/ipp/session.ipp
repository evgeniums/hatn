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

template <typename UnitT>
Error SessionAuth::serializeAuthHeader(lib::string_view protocol, uint32_t protocolVersion, common::SharedPtr<UnitT> content,
                                       const common::pmr::AllocatorFactory* factory
                                       )
{
    return Auth::serializeAuthHeader(protocol,protocolVersion,std::move(content),protocol::request::session_auth.id(),factory);
}

//---------------------------------------------------------------

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPICLIENTREQUEST_IPP
