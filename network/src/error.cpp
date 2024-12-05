/*
   Copyright (c) 2019 - current, Evgeny Sidorov (esid1976@gmail.com), All rights reserved
    
    
  */

/****************************************************************************/
/*
    
*/
/** \file network/error.сpp
  *
  *     Error classes for Hatn Network Library
  *
  */

#include <hatn/common/translate.h>
#include <hatn/network/error.h>

namespace hatn {
namespace network {

/********************** NetworkErrorCategory **************************/

static NetworkErrorCategory NetworkErrorCategoryInstance;

//---------------------------------------------------------------
const NetworkErrorCategory& NetworkErrorCategory::getCategory() noexcept
{
    return NetworkErrorCategoryInstance;
}

//---------------------------------------------------------------
std::string NetworkErrorCategory::message(int code) const
{
    std::string result{};
    if (result.empty())
    {
        switch (code)
        {
            case (static_cast<int>(ErrorCode::OK)):
                result=common::CommonErrorCategory::getCategory().message(code);
            break;

            case (static_cast<int>(ErrorCode::NOT_APPLICABLE)):
                result=common::_TR("Not applicable","network");
            break;

            case (static_cast<int>(ErrorCode::NOT_SUPPORTED)):
                result=common::_TR("Not supported","network");
            break;

            case (static_cast<int>(ErrorCode::CONNECT_FAILED)):
                result=common::_TR("Failed to connect","network");
            break;

            case (static_cast<int>(ErrorCode::ADDRESS_IN_USE)):
                result=common::_TR("Address already in use","network");
            break;

            case (static_cast<int>(ErrorCode::DNS_FAILED)):
                result=common::_TR("Failed to resolve DNS address","network");
            break;

            case (static_cast<int>(ErrorCode::OPEN_FAILED)):
                result=common::_TR("Failed to open socket","network");
            break;

            case (static_cast<int>(ErrorCode::PROXY_AUTH_FAILED)):
                result=common::_TR("SOCKS5 proxy authorization error: invalid login ot password","network");
            break;

            case (static_cast<int>(ErrorCode::PROXY_FAILED)):
                result=common::_TR("SOCKS5 failed","network");
            break;

            case (static_cast<int>(ErrorCode::PROXY_MAILFORMED_DATA)):
                result=common::_TR("SOCKS5 data mailformed","network");
            break;

            case (static_cast<int>(ErrorCode::PROXY_UNSUPPORTED_AUTH_METHOD)):
                result=common::_TR("SOCKS5 unsupported authorization method","network");
            break;

            case (static_cast<int>(ErrorCode::PROXY_UNSUPPORTED_IP_PROTOCOL)):
                result=common::_TR("SOCKS5 unsupported IP protocol","network");
            break;

            case (static_cast<int>(ErrorCode::PROXY_UNSUPPORTED_VERSION)):
                result=common::_TR("SOCKS5 unsupported version","network");
            break;

            case (static_cast<int>(ErrorCode::PROXY_REPORTED_ERROR)):
                result=common::_TR("SOCKS5 proxy server returned error status","network");
            break;

            case (static_cast<int>(ErrorCode::PROXY_INVALID_PARAMETERS)):
                result=common::_TR("SOCKS5 invalid parameters of proxy server","network");
            break;

            default:
                result=common::_TR("Unknown error");
        }
    }
    return result;
}

//---------------------------------------------------------------
    } // namespace network
} // namespace hatn
