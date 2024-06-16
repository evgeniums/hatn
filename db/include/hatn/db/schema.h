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

#include <hatn/common/meta/decaytuple.h>

#include <hatn/db/db.h>
#include <hatn/db/model.h>

HATN_DB_NAMESPACE_BEGIN

template <typename ModelsWithInfoT>
struct Schema
{
    std::string name;
    ModelsWithInfoT models;
};

struct makeSchemaT
{
    template <typename ...ModelsWithInfoT>
    auto operator ()(std::string name, ModelsWithInfoT&& ...models) const
    {
        auto xs=hana::make_tuple(std::forward<ModelsWithInfoT>(models)...);
        using modelsType=common::decayTuple<decltype(xs)>;
        return Schema<modelsType>{std::move(name),std::forward<ModelsWithInfoT>(models)...};
    }
};
constexpr makeSchemaT makeSchema{};

HATN_DB_NAMESPACE_END

#endif // HATNDBSCHEMA_H
