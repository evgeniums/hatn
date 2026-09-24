/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientapp/mobileusererror.h
  *
  *     UserErrorMapper / setUserErrorMapper / fillError - split out of mobileapp.h because they
  *     are the only reason that header used to need <hatn/common/error.h> (boost) and
  *     <hatn/common/stdwrappers.h> (function2). mobileapp.h is on the whitembridge C-bridge
  *     include path, which has neither available; this header is not, and is included only by
  *     the two .cpp files that actually install/use the mapper.
  */

/****************************************************************************/

#ifndef HATNMOBILEAPPUSERERROR_H
#define HATNMOBILEAPPUSERERROR_H

#include <functional>

#include <hatn/common/error.h>
#include <hatn/common/stdwrappers.h>

#include <hatn/clientapp/mobileapp.h>

HATN_CLIENTAPP_MOBILE_NAMESPACE_BEGIN

//! App-installable hook that fills Error::userCode/disposition/retryAfter from an internal
//! HATN_NAMESPACE::Error, optionally scoped by service/method (mirrors exec()'s own
//! service/method parameters). hatn itself never interprets userCode - see Error::userCode
//! above. Not thread-safe to call concurrently with fillError(); intended to be installed once,
//! early, by app startup code (whitemclient's MobileApp constructor).
using UserErrorMapper=std::function<void (const HATN_NAMESPACE::Error& ec,
                                          lib::string_view service,
                                          lib::string_view method,
                                          Error& out)>;

//! Installs the process-wide mapper used by fillError(). A default no-op mapper is installed
//! initially, so hatn behaves exactly as before this hook existed until an app installs one.
HATN_CLIENTAPP_EXPORT void setUserErrorMapper(UserErrorMapper mapper);

//! Fills out's code/codeString/message from ec, then runs the installed UserErrorMapper (if any)
//! to also fill userCode/disposition/retryAfter. The single place mobileapp.cpp turns an
//! internal Error into the bridge-facing one - see its call sites for why: several of them are
//! lambdas that capture neither `this` nor a MobileApp_p, so the mapper is looked up via this
//! process-wide accessor rather than threaded through as a parameter.
HATN_CLIENTAPP_EXPORT void fillError(Error& out, const HATN_NAMESPACE::Error& ec,
                                     lib::string_view service={},
                                     lib::string_view method={});

HATN_CLIENTAPP_MOBILE_NAMESPACE_END

#endif // HATNMOBILEAPPUSERERROR_H
