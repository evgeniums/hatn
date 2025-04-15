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
#include <hatn/db/modelsprovider.h>

HATN_DB_NAMESPACE_BEGIN

class HATN_DB_EXPORT Schema
{
    public:

        Schema(
                std::string name
            ) : m_name(std::move(name))
        {}

        virtual ~Schema();

        Schema()=default;
        Schema(const Schema&)=delete;
        Schema(Schema&&)=default;
        Schema& operator=(const Schema&)=delete;
        Schema& operator=(Schema&&)=default;

        std::string name() const noexcept
        {
            return m_name;
        }

        void addModel(std::shared_ptr<ModelInfo> model)
        {
            std::string coll=model->collection();
            m_models[coll]=std::move(model);
        }

        const std::map<std::string,std::shared_ptr<ModelInfo>>& models() const noexcept
        {
            return m_models;
        }

        void addModels(const ModelsProvider* provider)
        {
            auto models=provider->models();
            for (auto&& model: models)
            {
                addModel(model);
            }
        }

    private:

        std::string m_name;
        std::map<std::string,std::shared_ptr<ModelInfo>> m_models;
};

struct makeSchemaT
{
    template <typename ...ModelsWithInfoT>
    auto operator ()(std::string name, ModelsWithInfoT&& ...models) const
    {
        auto xs=hana::make_tuple(std::forward<ModelsWithInfoT>(models)...);
        auto s=std::make_shared<Schema>(std::move(name));
        hana::for_each(xs,[&s](auto&& model) {s->addModel(model->info);});
        return s;
    }
};
constexpr makeSchemaT makeSchema{};

HATN_DB_NAMESPACE_END

#endif // HATNDBSCHEMA_H
