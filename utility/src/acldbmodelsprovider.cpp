/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file acl/acldbmodelsprovider.сpp
  *
  */

#include <hatn/db/config.h>

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB

#include <hatn/dataunit/ipp/objectid.ipp>
#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>

#include <hatn/db/plugins/rocksdb/ipp/fieldvaluetobuf.ipp>
#include <hatn/db/plugins/rocksdb/ipp/rocksdbmodels.ipp>

#endif

#include <hatn/utility/utility.h>
#include <hatn/utility/acldbmodels.h>
#include <hatn/utility/acldbmodelsprovider.h>

HATN_UTILITY_NAMESPACE_BEGIN

class AclDbModelsProvider_p
{
    public:

        AclDbModelsProvider_p(std::shared_ptr<db::ModelsWrapper> wrp)
        {
            wrapper=std::dynamic_pointer_cast<AclDbModels>(std::move(wrp));
            Assert(wrapper,"Invalid ACL database models wrapper, must be acl::AclDbModels");
        }

        std::shared_ptr<AclDbModels> wrapper;
};

//--------------------------------------------------------------------------

AclDbModelsProvider::AclDbModelsProvider(
        std::shared_ptr<db::ModelsWrapper> wrapper
    ) : d(std::make_unique<AclDbModelsProvider_p>(std::move(wrapper)))
{}

//--------------------------------------------------------------------------

AclDbModelsProvider::AclDbModelsProvider(
    ) : AclDbModelsProvider(std::make_shared<AclDbModels>())
{}

//--------------------------------------------------------------------------

AclDbModelsProvider::~AclDbModelsProvider()
{}

//--------------------------------------------------------------------------

std::vector<std::shared_ptr<db::ModelInfo>> AclDbModelsProvider::models() const
{
    return modelInfoList(d->wrapper->models());
}

//--------------------------------------------------------------------------

void AclDbModelsProvider::registerRocksdbModels()
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

void AclDbModelsProvider::unregisterRocksdbModels()
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

//--------------------------------------------------------------------------

HATN_UTILITY_NAMESPACE_END
