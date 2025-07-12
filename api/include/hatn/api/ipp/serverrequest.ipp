/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/ipp/serverrequest.ipp
  *
  */

/****************************************************************************/

#ifndef HATNAPISERVERREQUEST_IPP
#define HATNAPISERVERREQUEST_IPP

#include <hatn/api/server/serverrequest.h>

HATN_API_NAMESPACE_BEGIN

namespace server {

//---------------------------------------------------------------

template <typename EnvT, typename RequestUnitT>
void Request<EnvT,RequestUnitT>::setResponseStatus()
{
    if (responseError)
    {
        response.setStatus(responseError.value().status,responseError.value().apiErrorPtr);
    }
    else
    {
        response.setStatus(protocol::ResponseStatus::Success);
    }
}

//---------------------------------------------------------------

template <typename EnvT, typename RequestUnitT>
void Request<EnvT,RequestUnitT>::close(const Error& ec)
{
    auto logClosing=[this]()
    {
        if (!complete)
        {
            HATN_CTX_PUSH_FIXED_VAR("complete",complete)
        }

        if (responseError)
        {
            HATN_CTX_PUSH_FIXED_VAR("status",responseStatusString(responseError.value().status))
            if (responseError.value().actualApiError()!=nullptr && !responseError.value().actualApiError()->isNull())
            {
                HATN_CTX_PUSH_FIXED_VAR("api_s",responseError.value().actualApiError()->status())
            }
        }
        else if (complete)
        {
            HATN_CTX_PUSH_FIXED_VAR("status","success")
        }

        HATN_CTX_CLOSE(error,"API EXEC")
    };

    // flush request's logs
    if (ec)
    {
        if (complete)
        {
            logClosing();
            HATN_CTX_LOG_ERR(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Warn,ec,"failed to send API response")
        }
        else
        {
            HATN_CTX_LOG_ERR(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Warn,ec,"failed to process API request")
            logClosing();
        }
    }
    else
    {
        logClosing();
    }
}

//---------------------------------------------------------------

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPISERVERREQUEST_IPP
