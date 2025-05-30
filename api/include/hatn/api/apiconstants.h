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

#include <cstddef>

#include <hatn/api/api.h>

HATN_API_NAMESPACE_BEGIN

constexpr const size_t DefaultSessionCallbacksCapacity=4;
constexpr const size_t DefaultMaxPoolPriorityConnections=4;

HATN_API_NAMESPACE_END

#endif // HATNAPICONSTANTS_H
