/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/apiconstants.h
  *
  */

/****************************************************************************/

#ifndef HATNAPICONSTANTS_H
#define HATNAPICONSTANTS_H

#include <hatn/api/api.h>
#include <hatn/api/connection.h>

HATN_API_NAMESPACE_BEGIN

//! @todo move some constants to protocol.h
constexpr const size_t ServiceNameLengthMax=16;
constexpr const size_t MethodNameLengthMax=32;
constexpr const size_t AuthProtocolNameLengthMax=8;
constexpr const size_t ResponseCategoryNameLengthMax=32;
constexpr const size_t DefaultSessionCallbacksCapacity=4;

constexpr const size_t DefaultMaxPoolPriorityConnections=4;

HATN_API_NAMESPACE_END

#endif // HATNAPICONSTANTS_H
