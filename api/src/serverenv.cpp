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

#include <hatn/app/app.h>

#include <hatn/api/apiliberror.h>
#include <hatn/api/server/env.h>

#include <hatn/api/ipp/serverenv.ipp>

HATN_API_NAMESPACE_BEGIN

namespace server {

Result<common::SharedPtr<BasicEnv>> BasicEnvConfig::makeEnv(
        const HATN_APP_NAMESPACE::App& app,
        const HATN_BASE_NAMESPACE::ConfigTree& configTree,
        const HATN_BASE_NAMESPACE::ConfigTreePath& configTreePath
    )
{
    // allocate
    auto allocator=app.allocatorFactory().factory()->objectAllocator<BasicEnv>();
    auto env=common::allocateEnvType<BasicEnv>(
        allocator
    );

    // init
    auto ec=initEnv(*env,app,configTree,configTreePath);
    HATN_CHECK_EC(ec)

    // done
    return env;
}

} // namespace server

HATN_API_NAMESPACE_END

