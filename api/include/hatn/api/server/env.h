/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/server/env.h
  *
  */

/****************************************************************************/

#ifndef HATNAPISERVERENV_H
#define HATNAPISERVERENV_H

#include <hatn/common/threadwithqueue.h>
#include <hatn/common/env.h>

#include <hatn/base/configobject.h>

#include <hatn/logcontext/withlogger.h>

#include <hatn/dataunit/syntax.h>

#include <hatn/db/asyncclient.h>

#include <hatn/app/app.h>

#include <hatn/api/api.h>
#include <hatn/api/protocol.h>

HATN_APP_NAMESPACE_BEGIN
class BaseApp;
HATN_APP_NAMESPACE_END

HATN_API_NAMESPACE_BEGIN

namespace server {

using Threads=common::WithMappedThreads;
using Db=db::AsyncDb;
using AllocatorFactory=common::pmr::WithFactory;
using Logger=logcontext::WithLogger;

//! @todo Add tenancy to Env

HDU_UNIT(protocol_config,
    HDU_FIELD(max_message_size,TYPE_UINT32,1,false,protocol::DEFAULT_MAX_MESSAGE_SIZE)
)

class ProtocolConfig : public HATN_BASE_NAMESPACE::ConfigObject<protocol_config::type>
{
    public:

        size_t maxMessageSize() const noexcept
        {
            return config().fieldValue(protocol_config::max_message_size);
        }
    //! @todo protect with mutex
};

using BasicEnv = common::Env<AllocatorFactory,Threads,Logger,Db,ProtocolConfig>;

template <typename EnvT=BasicEnv>
class WithEnv
{
    public:

        WithEnv()
        {}

        WithEnv(common::SharedPtr<EnvT> env):m_env(std::move(env))
        {}

        void setEnv(common::SharedPtr<EnvT> env)
        {
            m_env=std::move(env);
        }

        common::SharedPtr<EnvT> envShared() const noexcept
        {
            return m_env;
        }

        EnvT* env() const noexcept
        {
            return m_env.get();
        }

    private:

        common::SharedPtr<EnvT> m_env;
};

struct HATN_API_EXPORT BasicEnvConfig
{
    using Env=BasicEnv;

    static Result<common::SharedPtr<Env>> makeEnv(
        const HATN_APP_NAMESPACE::BaseApp& app,
        const HATN_BASE_NAMESPACE::ConfigTree& configTree,
        const HATN_BASE_NAMESPACE::ConfigTreePath& configTreePath
    );
};

template <typename Traits=BasicEnvConfig>
struct EnvConfigT
{
    using Env=typename Traits::Env;

    static Result<common::SharedPtr<Env>> makeEnv(
        const HATN_APP_NAMESPACE::BaseApp& app,
        const HATN_BASE_NAMESPACE::ConfigTree& configTree,
        const HATN_BASE_NAMESPACE::ConfigTreePath& configTreePath
    )
    {
        return Traits::makeEnv(app,configTree,configTreePath);
    }
};
using EnvConfig=EnvConfigT<>;

} // namespace server

HATN_API_NAMESPACE_END

HATN_TASK_CONTEXT_DECLARE(HATN_API_NAMESPACE::server::WithEnv<HATN_API_NAMESPACE::server::BasicEnv>,HATN_API_EXPORT)

#endif // HATNAPISERVERCONTEXT_H

