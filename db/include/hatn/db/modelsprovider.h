/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/modelsprovider.h
  *
  */

/****************************************************************************/

#ifndef HATNDBMODELSPROVIDER_H
#define HATNDBMODELSPROVIDER_H

#include <hatn/db/db.h>
#include <hatn/db/model.h>

HATN_DB_NAMESPACE_BEGIN

class ModelsWrapper;

class HATN_DB_EXPORT ModelsProvider
{
    public:

        ModelsProvider()=default;

        virtual ~ModelsProvider();
        ModelsProvider(const ModelsProvider&)=delete;
        ModelsProvider(ModelsProvider&&)=default;
        ModelsProvider& operator=(const ModelsProvider&)=delete;
        ModelsProvider& operator=(ModelsProvider&&)=default;

        virtual void registerRocksdbModels()=0;
        virtual void unregisterRocksdbModels()=0;

        virtual std::vector<std::shared_ptr<ModelInfo>> models() const=0;

        template <typename Models>
        static std::vector<std::shared_ptr<ModelInfo>> modelInfoList(const Models& models)
        {
            std::vector<std::shared_ptr<ModelInfo>> res;
            hana::for_each(
                models,
                [&res](auto&& model)
                {
                    res.push_back(model()->info);
                }
            );
            return res;
        }
};

template <typename T>
class DbModelsProviderT : public ModelsProvider
{
    public:

        DbModelsProviderT();
        DbModelsProviderT(std::shared_ptr<ModelsWrapper> wrapper);

        ~DbModelsProviderT();
        DbModelsProviderT(const DbModelsProviderT&)=delete;
        DbModelsProviderT(DbModelsProviderT&&)=default;
        DbModelsProviderT& operator=(const DbModelsProviderT&)=delete;
        DbModelsProviderT& operator=(DbModelsProviderT&&)=default;

        virtual void registerRocksdbModels() override;
        virtual void unregisterRocksdbModels() override;

        virtual std::vector<std::shared_ptr<ModelInfo>> models() const override;

    private:

        std::unique_ptr<T> d;
};


HATN_DB_NAMESPACE_END

#endif // HATNDBMODELSPROVIDER_H
