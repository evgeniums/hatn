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

template <typename SessionTraits, typename MessageT, typename RequestUnitT>
Error Request<SessionTraits,MessageT,RequestUnitT>::makeUnit(
        const Service& service,
        const Method& method,
        lib::string_view topic
    )
{
    //! @todo Implement
    //! @todo Serialize to shared buffer and set it to request
    return CommonError::NOT_IMPLEMENTED;
}

//---------------------------------------------------------------

template <typename SessionTraits, typename MessageT, typename RequestUnitT>
void Request<SessionTraits,MessageT,RequestUnitT>::updateSession(
        std::function<void (const Error&)> cb
    )
{
    //! @todo Implement
    cb(Error{CommonError::NOT_IMPLEMENTED});
}

template <typename SessionTraits, typename MessageT, typename RequestUnitT>
void Request<SessionTraits,MessageT,RequestUnitT>::regenId()
{
    //! @todo Implement
}

//---------------------------------------------------------------

template <typename SessionTraits, typename MessageT, typename RequestUnitT>
Error Request<SessionTraits,MessageT,RequestUnitT>::serialize(
    )
{
    //! @todo Implement
    return CommonError::NOT_IMPLEMENTED;
}

//---------------------------------------------------------------

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPICLIENTREQUEST_IPP
