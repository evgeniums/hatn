/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file common/apierrordisposition.h
  *
  *     Contains definition of ApiErrorDisposition, split out of apierror.h so that it can be
  *     used without pulling in apierror.h's <hatn/common/sharedptr.h> -> function2 dependency.
  *     In particular, hatn/clientapp/mobileapp.h names this enum and must stay free of both
  *     boost and function2 (see the comment at the top of that header).
  *
  */

/****************************************************************************/

#ifndef HATNAPIERRORDISPOSITION_H
#define HATNAPIERRORDISPOSITION_H

#include <cstdint>

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

//! What a client should do about an ApiError - the terminal/retryable distinction stated by the
//! server, per whitemdesktop/docs/error-contract.md. Mirrors evgo's generic_error.Disposition
//! string values one-for-one so the wire encoding (x-hatn-edisposition et al) round-trips.
enum class ApiErrorDisposition : uint8_t
{
    //! Server did not state a disposition (absent field, or a peer predating this contract).
    //! The zero value - a client must fall back to its own heuristics.
    Unknown=0,
    //! This request will never succeed as issued.
    Permanent,
    //! The server does not implement this call, or the API version is too old. Terminal like
    //! Permanent, but distinct: a client should stop offering the feature, not just fail this
    //! one call.
    Unsupported,
    //! Transient; retry with backoff.
    Retry,
    //! Retryable, but not yet - see ApiError::retryAfter() for the delay in seconds.
    RetryAfter,
    //! Retryable only after the user does something: re-auth, free storage, raise a quota.
    UserAction
};

HATN_COMMON_NAMESPACE_END

#endif // HATNAPIERRORDISPOSITION_H
