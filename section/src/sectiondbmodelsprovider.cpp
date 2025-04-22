/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file section/sectiondbmodelsprovider.сpp
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

#include <hatn/section/section.h>
#include <hatn/section/sectiondbmodels.h>
#include <hatn/section/sectiondbmodelsprovider.h>

HATN_SECTION_NAMESPACE_BEGIN

class SectionDbModelsProvider_p
{
    public:

        SectionDbModelsProvider_p(std::shared_ptr<db::ModelsWrapper> wrp)
        {
            wrapper=std::dynamic_pointer_cast<SectionDbModels>(std::move(wrp));
            Assert(wrapper,"Invalid SECTION database models wrapper, must be section::SectionDbModels");
        }

        std::shared_ptr<SectionDbModels> wrapper;
};

//--------------------------------------------------------------------------

SectionDbModelsProvider::SectionDbModelsProvider(
        std::shared_ptr<db::ModelsWrapper> wrapper
    ) : d(std::make_unique<SectionDbModelsProvider_p>(std::move(wrapper)))
{}

//--------------------------------------------------------------------------

SectionDbModelsProvider::SectionDbModelsProvider(
    ) : SectionDbModelsProvider(std::make_shared<SectionDbModels>())
{}

//--------------------------------------------------------------------------

SectionDbModelsProvider::~SectionDbModelsProvider()
{}

//--------------------------------------------------------------------------

std::vector<std::shared_ptr<db::ModelInfo>> SectionDbModelsProvider::models() const
{
    return modelInfoList(d->wrapper->models());
}

//--------------------------------------------------------------------------

void SectionDbModelsProvider::registerRocksdbModels()
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

void SectionDbModelsProvider::unregisterRocksdbModels()
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

HATN_SECTION_NAMESPACE_END
