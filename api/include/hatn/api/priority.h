/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/priority.h
  *
  */

/****************************************************************************/

#ifndef HATNAPIPRIORITY_H
#define HATNAPIPRIORITY_H

#include <hatn/common/stdwrappers.h>

#include <hatn/api/api.h>

HATN_API_NAMESPACE_BEGIN

enum class Priority : uint8_t
{
    Lowest,
    Low,
    Normal,
    High,
    Highest
};

HATN_API_NAMESPACE_END

#endif // HATNAPIPRIORITY_H
