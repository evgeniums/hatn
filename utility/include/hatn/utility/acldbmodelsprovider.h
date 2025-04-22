/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file acl/acldbmodelsprovider.h
  */

/****************************************************************************/

#ifndef HATNACLDBMODELSPROVIDER_H
#define HATNACLDBMODELSPROVIDER_H

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

#endif // HATNACLDBMODELSPROVIDER_H
