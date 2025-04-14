/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/server/microservicefactory.h
  *
  */

/****************************************************************************/

#ifndef HATNAPIMICROSERVICEFACTORY_H
#define HATNAPIMICROSERVICEFACTORY_H

#include <hatn/common/error.h>

#include <hatn/app/baseapp.h>

#include <hatn/api/api.h>
#include <hatn/api/apiliberror.h>
#include <hatn/api/server/microservice.h>

HATN_API_NAMESPACE_BEGIN

namespace server {

using MicroServiceBuilder=std::function<Result<std::shared_ptr<MicroService>> (std::string name, const std::string& dispatcher, const std::string& authDispatcher)>;

class HATN_API_EXPORT MicroServiceFactory
{
    public:

        constexpr static const char* MicroserivicesConfigSection="microservices";
        constexpr static const char* MicroserviceConfigSection="microservice";

        using Builder=MicroServiceBuilder;

        void registerBuilder(
                std::string name,
                Builder builder
            )
        {
            m_builders[std::move(name)]=std::move(builder);
        }

        Result<std::shared_ptr<MicroService>> makeAndRun(
            const HATN_APP_NAMESPACE::BaseApp& app,
            const HATN_BASE_NAMESPACE::ConfigTree& configTree,
            const HATN_BASE_NAMESPACE::ConfigTreePath& configTreePath={}
        ) const;

        Result<std::map<std::string,std::shared_ptr<MicroService>>> makeAndRunAll(
            const HATN_APP_NAMESPACE::BaseApp& app,
            const HATN_BASE_NAMESPACE::ConfigTree& configTree,
            const HATN_BASE_NAMESPACE::ConfigTreePath& configTreePath={}
        ) const;

    private:

        std::map<std::string,Builder> m_builders;

        friend struct makeMicroserviceAndRunT;
};

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPIMICROSERVICEFACTORY_H
