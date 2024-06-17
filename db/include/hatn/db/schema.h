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

class HATN_DB_EXPORT DbSchema
{
    public:

        DbSchema(
            std::string name
            ) : m_name(std::move(name))
        {}

        virtual ~DbSchema();

        DbSchema()=default;
        DbSchema(const DbSchema&)=delete;
        DbSchema(DbSchema&&)=default;
        DbSchema& operator=(const DbSchema&)=delete;
        DbSchema& operator=(DbSchema&&)=default;

        std::string name() const noexcept
        {
            return m_name;
        }

    private:

        std::string m_name;
};

template <typename ModelsWithInfoT>
class Schema : public DbSchema
{
    public:

        Schema(
                std::string name,
                ModelsWithInfoT models
            ) : DbSchema(std::move(name)),
                m_models(std::move(models))
        {}

        const ModelsWithInfoT& models() const noexcept
        {
            return m_models;
        }

        ModelsWithInfoT& models() noexcept
        {
            return m_models;
        }

    private:

        ModelsWithInfoT m_models;
};

struct makeSchemaT
{
    template <typename ...ModelsWithInfoT>
    auto operator ()(std::string name, ModelsWithInfoT&& ...models) const
    {
        auto xs=hana::make_tuple(std::forward<ModelsWithInfoT>(models)...);
        using modelsType=common::decayTuple<decltype(xs)>;
        return std::make_shared<Schema<modelsType>>(std::move(name),std::move(xs));
    }
};
constexpr makeSchemaT makeSchema{};

HATN_DB_NAMESPACE_END

#endif // HATNDBSCHEMA_H
