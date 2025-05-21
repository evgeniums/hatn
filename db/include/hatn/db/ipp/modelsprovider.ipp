/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/ipp/modelsprovider.ipp
  *
  */

/****************************************************************************/

#ifndef HATNDBMODELSPROVIDER_IPP
#define HATNDBMODELSPROVIDER_IPP

#include <hatn/db/db.h>
#include <hatn/db/model.h>

#include <hatn/db/modelswrapper.h>
#include <hatn/db/modelsprovider.h>

HATN_DB_NAMESPACE_BEGIN

template <typename T>
class DbModelsProviderT_p
{
    public:

        using DbModelsWrapper=T;

        DbModelsProviderT_p(std::shared_ptr<ModelsWrapper> wrp)
        {
            wrapper=std::dynamic_pointer_cast<T>(std::move(wrp));
            Assert(wrapper,"Invalid database models wrapper");
        }

        std::shared_ptr<T> wrapper;
};

//--------------------------------------------------------------------------

template <typename T>
DbModelsProviderT<T>::DbModelsProviderT(
    std::shared_ptr<ModelsWrapper> wrapper
    ) : d(std::make_unique<T>(std::move(wrapper)))
{}

//--------------------------------------------------------------------------

template <typename T>
DbModelsProviderT<T>::DbModelsProviderT(
    ) : DbModelsProviderT(std::make_shared<typename T::DbModelsWrapper>())
{}

//--------------------------------------------------------------------------

template <typename T>
DbModelsProviderT<T>::~DbModelsProviderT()
{}

//--------------------------------------------------------------------------

template <typename T>
std::vector<std::shared_ptr<ModelInfo>> DbModelsProviderT<T>::models() const
{
    return modelInfoList(d->wrapper->models());
}

//--------------------------------------------------------------------------

template <typename T>
void DbModelsProviderT<T>::registerRocksdbModels()
{
#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
    hana::for_each(
        d->wrapper->models(),
        [](const auto& model)
        {
            HATN_ROCKSDB_NAMESPACE::RocksdbModels::instance().registerModel(model());
        }
        );
#endif
}

//--------------------------------------------------------------------------

template <typename T>
void DbModelsProviderT<T>::unregisterRocksdbModels()
{
#ifdef HATN_ENABLE_PLUGIN_ROCKSDB
    hana::for_each(
        d->wrapper->models(),
        [](const auto& model)
        {
            HATN_ROCKSDB_NAMESPACE::RocksdbModels::instance().unregisterModel(model());
        }
        );
#endif
}

HATN_DB_NAMESPACE_END

#endif // HATNDBMODELSPROVIDER_IPP
