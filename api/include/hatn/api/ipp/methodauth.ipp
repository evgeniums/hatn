/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/ipp/methodauth.ipp
  *
  */

/****************************************************************************/

#ifndef HATNAPIAUTH_IPP
#define HATNAPIAUTH_IPP

#include <hatn/api/requestunit.h>

#include <hatn/api/methodauth.h>
#include <hatn/api/ipp/auth.ipp>

HATN_API_NAMESPACE_BEGIN

namespace client {

//---------------------------------------------------------------

template <typename UnitT>
Error MethodAuth::serializeAuthHeader(lib::string_view protocol, uint32_t protocolVersion, common::SharedPtr<UnitT> content,
                                       const common::pmr::AllocatorFactory* factory
                                       )
{
    return Auth::serializeAuthHeader(protocol,protocolVersion,std::move(content),request::method_auth.id(),factory);
}

}

//---------------------------------------------------------------

HATN_API_NAMESPACE_END

#endif // HATNAPIAUTH_IPP
