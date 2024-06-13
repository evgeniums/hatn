/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/schema.h
  *
  * Contains helpers for database schema definition.
  *
  */

/****************************************************************************/

#ifndef HATNDBSCHEMA_H
#define HATNDBSCHEMA_H

#include <hatn/db/db.h>
#include <hatn/db/index.h>
#include <hatn/db/model.h>

HATN_DB_NAMESPACE_BEGIN

template <typename ...Models>
struct Schema
{
    std::string name;
    hana::tuple<Models...> models;
    std::set<DatePartitionMode> partitionModes;
};

HATN_DB_NAMESPACE_END

#endif // HATNDBSCHEMA_H
