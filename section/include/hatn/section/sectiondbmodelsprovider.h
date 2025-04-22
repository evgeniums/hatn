/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file section/sectiondbmodelsprovider.h
  */

/****************************************************************************/

#ifndef HATNSECTIONDBMODELSPROVIDER_H
#define HATNSECTIONDBMODELSPROVIDER_H

#include <hatn/db/modelsprovider.h>

#include <hatn/section/section.h>

HATN_SECTION_NAMESPACE_BEGIN

class SectionDbModelsProvider_p;

class HATN_SECTION_EXPORT SectionDbModelsProvider : public db::ModelsProvider
{
    public:

        SectionDbModelsProvider();
        SectionDbModelsProvider(std::shared_ptr<db::ModelsWrapper> wrapper);

        ~SectionDbModelsProvider();
        SectionDbModelsProvider(const SectionDbModelsProvider&)=delete;
        SectionDbModelsProvider(SectionDbModelsProvider&&)=default;
        SectionDbModelsProvider& operator=(const SectionDbModelsProvider&)=delete;
        SectionDbModelsProvider& operator=(SectionDbModelsProvider&&)=default;

        virtual void registerRocksdbModels() override;
        virtual void unregisterRocksdbModels() override;

        virtual std::vector<std::shared_ptr<db::ModelInfo>> models() const override;

    private:

        std::unique_ptr<SectionDbModelsProvider_p> d;
};

HATN_SECTION_NAMESPACE_END

#endif // HATNSECTIONDBMODELSPROVIDER_H
