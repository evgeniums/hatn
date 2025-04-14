/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/serverenv.сpp
  *
  */

#include <hatn/dataunit/ipp/syntax.ipp>

#include <hatn/app/baseapp.h>

#include <hatn/api/apiliberror.h>
#include <hatn/api/server/env.h>

HATN_API_NAMESPACE_BEGIN

namespace server {

Result<common::SharedPtr<BasicEnv>> BasicEnvConfig::makeEnv(
        const HATN_APP_NAMESPACE::BaseApp& app,
        const HATN_BASE_NAMESPACE::ConfigTree& configTree,
        const HATN_BASE_NAMESPACE::ConfigTreePath& configTreePath
    )
{
    std::ignore=configTree;
    std::ignore=configTreePath;

    auto allocator=app.allocatorFactory().factory()->objectAllocator<BasicEnv>();

    //! @todo Setup server threads
    //! @todo Copy db clients
    auto env=common::allocateEnvType<BasicEnv>(
        allocator,
        HATN_COMMON_NAMESPACE::contexts(
                HATN_COMMON_NAMESPACE::context(app.allocatorFactory().factory()),
                HATN_COMMON_NAMESPACE::context(app.appThread()),
                HATN_COMMON_NAMESPACE::context(app.logger().loggerShared()),
                HATN_COMMON_NAMESPACE::context(),
                HATN_COMMON_NAMESPACE::context()
            )
        );

    // load protocol configuration
    auto path=configTreePath.copyAppend("protocol");
    auto& protocolConfig=env->get<ProtocolConfig>();
    //! @todo Log protocol configuration?
    auto ec=protocolConfig.loadConfig(configTree,path);
    if (ec)
    {
        auto ec1=apiLibError(ApiLibError::PROTOCOL_CONFIGURATION_FAILED);
        ec.stackWith(std::move(ec1));
        return ec;
    }

    // done
    return env;
}

} // namespace server

HATN_API_NAMESPACE_END

