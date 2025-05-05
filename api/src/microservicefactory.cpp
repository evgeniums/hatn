/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/microservicefactory.cpp
  *
  */

#include <hatn/base/configobject.h>

#include <hatn/logcontext/logconfigrecords.h>

#include <hatn/dataunit/ipp/syntax.ipp>

#include <hatn/api/server/microserviceconfig.h>
#include <hatn/api/server/microservicefactory.h>

HATN_API_NAMESPACE_BEGIN

namespace server {

using MicroserviceConfig=HATN_BASE_NAMESPACE::ConfigObject<microservice_config::type>;

struct makeMicroserviceAndRunT
{
    Result<std::shared_ptr<MicroService>> operator () (
            const MicroServiceFactory* factory,
            MicroserviceConfig& cfg,
            const HATN_APP_NAMESPACE::App& app,
            const HATN_BASE_NAMESPACE::ConfigTree& configTree,
            const HATN_BASE_NAMESPACE::ConfigTreePath& configTreePath
        ) const
    {
        Error ec;

        // find builder
        std::string microserviceName{cfg.config().fieldValue(microservice_config::name)};
        auto it=factory->m_builders.find(microserviceName);
        if (it==factory->m_builders.end())
        {
            // failed to find microservice
            auto ec1=apiLibError(ApiLibError::UNKNOWN_MICROSERVICE_TYPE,
                                   std::make_shared<common::NativeError>(fmt::format("\"{}\"",microserviceName))
                                   );
            return ec1;
        }

        // create microservice
        std::string dispatcherName{cfg.config().fieldValue(microservice_config::dispatcher)};
        if (dispatcherName.empty())
        {
            dispatcherName=microserviceName;
        }
        std::string authDispatcherName{cfg.config().fieldValue(microservice_config::auth_dispatcher)};
        if (authDispatcherName.empty())
        {
            authDispatcherName=microserviceName;
        }
        auto microservice=it->second(
            microserviceName,
            dispatcherName,
            authDispatcherName
        );
        if (microservice)
        {
            // failed to load microservice config
            ec=microservice.takeError();
            auto ec1=apiLibError(ApiLibError::MICROSERVICE_CREATE_FAILED,
                                   std::make_shared<common::NativeError>(fmt::format(_TR("microservice \"{}\"","api"),microserviceName))
                                   );
            ec.stackWith(std::move(ec1));
            return ec;
        }

        // run microservice
        ec=microservice.value()->start(app,configTree,configTreePath.copyAppend(factory->MicroserviceConfigSection));
        if (ec)
        {
            auto ec1=apiLibError(ApiLibError::MICROSERVICE_RUN_FAILED,
                                   std::make_shared<common::NativeError>(fmt::format(_TR("microservice \"{}\"","api"),microserviceName))
                                   );
            ec.stackWith(std::move(ec1));
            return ec;
        }

        // done
        return microservice;
    }
};

namespace
{
constexpr makeMicroserviceAndRunT makeMicroserviceAndRun{};
}

//---------------------------------------------------------------

Result<std::shared_ptr<MicroService>> MicroServiceFactory::makeAndRun(
        const HATN_APP_NAMESPACE::App& app,
        const HATN_BASE_NAMESPACE::ConfigTree& configTree,
        const HATN_BASE_NAMESPACE::ConfigTreePath& configTreePath
    ) const
{
    MicroserviceConfig cfg;

    // load config
    auto ec=loadLogConfig(_TR("configuration of microservice"),cfg,configTree,configTreePath);
    if (ec)
    {
        // failed to load microservice config
        auto ec1=apiLibError(ApiLibError::MICROSERVICE_CONFIG_INVALID,
                               std::make_shared<common::NativeError>(fmt::format(_TR("at path \"{}\"","api"),configTreePath.path()))
                               );
        ec.stackWith(std::move(ec1));
        return ec;
    }

    // make and tun microservice
    return makeMicroserviceAndRun(this,cfg,app,configTree,configTreePath);
}

//---------------------------------------------------------------

Result<std::map<std::string,std::shared_ptr<MicroService>>> MicroServiceFactory::makeAndRunAll(
        const HATN_APP_NAMESPACE::App& app,
        const HATN_BASE_NAMESPACE::ConfigTree& configTree,
        const HATN_BASE_NAMESPACE::ConfigTreePath& configTreePath
    ) const
{
    std::map<std::string,std::shared_ptr<MicroService>> microservices;

    // load microservices section as array
    auto path=configTreePath.copyAppend(MicroserivicesConfigSection);
    if (!configTree.isSet(path))
    {
        return apiLibError(ApiLibError::MICROSERVICES_CONFIG_SECTION_MISSING,
                           std::make_shared<common::NativeError>(fmt::format(_TR("at path \"{}\"","api"),path.path()))
                           );
    }
    auto microservicesSection=configTree.get(path);
    Assert(!microservicesSection,"Missing microservices section");
    if (microservicesSection->type()!=HATN_BASE_NAMESPACE::config_tree::Type::ArrayTree)
    {
        return apiLibError(ApiLibError::MICROSERVICES_CONFIG_SECTION_INVALID,
                           std::make_shared<common::NativeError>(fmt::format(_TR("at path \"{}\"","api"),path.path()))
                           );
    }
    auto configs=microservicesSection->asArray<HATN_BASE_NAMESPACE::ConfigTree>();
    Assert(!configs,"Invalid microservices section");

    // process each microservice config
    for (size_t i=0;i<configs->size();i++)
    {
        auto microservicePath=path.copyAppend(i);
        MicroserviceConfig cfg;

        auto ec=loadLogConfig(_TR("configuration of microservice"),cfg,configTree,microservicePath);
        if (ec)
        {
            // failed to load microservice config
            auto ec1=apiLibError(ApiLibError::MICROSERVICE_CONFIG_INVALID,
                                   std::make_shared<common::NativeError>(fmt::format(_TR("at path \"{}.{}\"","api"),path.path(),i))
                                   );
            ec.stackWith(std::move(ec1));
        }
        else
        {
            // microservice config was loaded
            std::string microserviceName{cfg.config().fieldValue(microservice_config::name)};

            // check for duplicate microservice
            if (microservices.find(microserviceName)!=microservices.end())
            {
                ec=apiLibError(ApiLibError::DUPLICATE_MICROSERVICE,
                                 std::make_shared<common::NativeError>(fmt::format(_TR("microservice \"{}\" at path \"{}.{}\"","api"),microserviceName,path.path(),i))
                                       );
            }

            // create and run microservice for config
            if (!ec)
            {
                auto microservice=makeMicroserviceAndRun(this,cfg,app,configTree,microservicePath);
                if (microservice)
                {
                    ec=microservice.takeError();
                }
                else
                {
                    // add microservice to map
                    microservices[microserviceName]=microservice.takeValue();
                }
            }
        }

        if (ec)
        {
            // close all microservices in case of error
            for (auto&& microservice: microservices)
            {
                microservice.second->close();
            }

            // return error
            return ec;
        }
    }

    // done
    return microservices;
}

} // namespace server

HATN_API_NAMESPACE_END

