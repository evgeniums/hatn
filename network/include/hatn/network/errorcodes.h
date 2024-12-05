#ifndef HATNNETWORKENUMS_H
#define HATNNETWORKENUMS_H

#include <hatn/common/error.h>

namespace hatn {
    namespace network {

        //! Error codes
        enum class ErrorCode : int
        {
            OK=static_cast<int>(common::CommonError::OK),
            NOT_APPLICABLE,
            NOT_SUPPORTED,

            OPEN_FAILED,
            ADDRESS_IN_USE,
            CONNECT_FAILED,

            DNS_FAILED=0x100,

            PROXY_ERRORS=0x200,
            PROXY_AUTH_FAILED=PROXY_ERRORS,
            PROXY_FAILED,
            PROXY_UNSUPPORTED_VERSION,
            PROXY_UNSUPPORTED_AUTH_METHOD,
            PROXY_REPORTED_ERROR,
            PROXY_UNSUPPORTED_IP_PROTOCOL,
            PROXY_MAILFORMED_DATA,
            PROXY_INVALID_PARAMETERS
        };
    }
}

#endif // HATNNETWORK_H
