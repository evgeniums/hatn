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

constexpr const size_t PrioritiesCount=5;

template <typename InitT, typename EachT>
void handlePriorities(const EachT& each, const InitT& init={})
{
    if (init)
    {
        init(PrioritiesCount);
    }

    each(Priority::Lowest);
    each(Priority::Low);
    each(Priority::Normal);
    each(Priority::High);
    each(Priority::Highest);
}

HATN_API_NAMESPACE_END

#endif // HATNAPIPRIORITY_H
