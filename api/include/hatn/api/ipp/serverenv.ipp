/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/ipp/serverenv.ipp
  *
  */

/****************************************************************************/

#ifndef HATNAPISERVERENV_IPP
#define HATNAPISERVERENV_IPP

#include <hatn/app/app.h>
#include <hatn/api/apiliberror.h>

#include <hatn/api/server/env.h>

HATN_API_NAMESPACE_BEGIN

namespace server {

//---------------------------------------------------------------

template <typename EnvT>
Error BasicEnvConfig::initEnv(
        EnvT& env,
        const HATN_APP_NAMESPACE::App& app,
        const HATN_BASE_NAMESPACE::ConfigTree& configTree,
        const HATN_BASE_NAMESPACE::ConfigTreePath& configTreePath
    )
{
    env.setEmbeddedEnv(app.env());

    // load protocol configuration
    auto path=configTreePath.copyAppend("protocol");
    auto& protocolConfig=env.template get<ProtocolConfig>();
    //! @todo Log protocol configuration?
    auto ec=protocolConfig.loadConfig(configTree,path);
    if (ec)
    {
        auto ec1=apiLibError(ApiLibError::PROTOCOL_CONFIGURATION_FAILED);
        ec.stackWith(std::move(ec1));
        return ec;
    }

    // done
    return OK;
}

//---------------------------------------------------------------

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPISERVERENV_IPP
