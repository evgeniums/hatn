/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/ipp/client.ipp
  *
  */

/****************************************************************************/

#ifndef HATNAPICLIENTREQUEST_IPP
#define HATNAPICLIENTREQUEST_IPP

#include <hatn/api/client/request.h>

HATN_API_NAMESPACE_BEGIN

namespace client {

//---------------------------------------------------------------

template <typename SessionTraits, typename ContextT, typename RequestUnitT>
template <typename UnitT>
Error Request<SessionTraits,ContextT,RequestUnitT>::makeUnit(
        const Service& service,
        const Method& method,
        const UnitT& content,
        lib::string_view topic
    )
{
    //! @todo Implement
    return CommonError::NOT_IMPLEMENTED;
}

//---------------------------------------------------------------

template <typename SessionTraits, typename ContextT, typename RequestUnitT>
void Request<SessionTraits,ContextT,RequestUnitT>::updateSession(
        std::function<void (const Error&)> cb
    )
{
    //! @todo Implement
    cb(Error{CommonError::NOT_IMPLEMENTED});
}

template <typename SessionTraits, typename ContextT, typename RequestUnitT>
void Request<SessionTraits,ContextT,RequestUnitT>::regenId()
{
    //! @todo Implement
}

//---------------------------------------------------------------

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPICLIENTREQUEST_IPP
