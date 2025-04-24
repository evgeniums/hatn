/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file utility/acldbmodelsprovider.h
  */

/****************************************************************************/

#ifndef HATNUTILITYACLDBMODELSPROVIDER_H
#define HATNUTILITYACLDBMODELSPROVIDER_H

#include <hatn/db/modelsprovider.h>

#include <hatn/utility/utility.h>

HATN_UTILITY_NAMESPACE_BEGIN

class AclDbModelsProvider_p;

class HATN_UTILITY_EXPORT AclDbModelsProvider : public db::ModelsProvider
{
    public:

        AclDbModelsProvider();
        AclDbModelsProvider(std::shared_ptr<db::ModelsWrapper> wrapper);

        ~AclDbModelsProvider();
        AclDbModelsProvider(const AclDbModelsProvider&)=delete;
        AclDbModelsProvider(AclDbModelsProvider&&)=default;
        AclDbModelsProvider& operator=(const AclDbModelsProvider&)=delete;
        AclDbModelsProvider& operator=(AclDbModelsProvider&&)=default;

        virtual void registerRocksdbModels() override;
        virtual void unregisterRocksdbModels() override;

        virtual std::vector<std::shared_ptr<db::ModelInfo>> models() const override;

    private:

        std::unique_ptr<AclDbModelsProvider_p> d;
};

HATN_UTILITY_NAMESPACE_END

#endif // HATNUTILITYACLDBMODELSPROVIDER_H
