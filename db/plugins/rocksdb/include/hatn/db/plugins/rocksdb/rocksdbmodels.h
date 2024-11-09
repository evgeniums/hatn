/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/plugins/rocksdb/rocksdbmodels.h
  *
  *   RocksDB database models singleton.
  *
  */

/****************************************************************************/

#ifndef HATNROCKSDBMODELS_H
#define HATNROCKSDBMODELS_H

#include <map>
#include <memory>

#include <hatn/common/singleton.h>
#include <hatn/common/stdwrappers.h>
#include <hatn/common/pmr/allocatorfactory.h>

#include <hatn/db/model.h>

#include <hatn/db/plugins/rocksdb/rocksdbschemadef.h>
#include <hatn/db/plugins/rocksdb/rocksdbmodel.h>

HATN_ROCKSDB_NAMESPACE_BEGIN

class HATN_ROCKSDB_SCHEMA_EXPORT RocksdbModels : public common::Singleton
{
    public:

        HATN_SINGLETON_DECLARE()

        static RocksdbModels& instance();
        static void free() noexcept;

        template <typename ModelT>
        void registerModel(std::shared_ptr<ModelWithInfo<ModelT>> model,
                           AllocatorFactory* allocatorFactory=AllocatorFactory::getDefault());

        std::shared_ptr<RocksdbModel> model(const ModelInfo& info) const
        {
            auto it=m_models.find(info.modelId());
            if (it==m_models.end())
            {
                return std::shared_ptr<RocksdbModel>{};
            }
            return it->second;
        }

        std::shared_ptr<RocksdbModel> model(const std::shared_ptr<ModelInfo>& info) const
        {
            return model(*info);
        }

    private:

        RocksdbModels();
        std::map<uint32_t,std::shared_ptr<RocksdbModel>> m_models;
};

HATN_ROCKSDB_NAMESPACE_END

#endif // HATNROCKSDBMODELS_H
