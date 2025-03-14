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

#include <hatn/dataunit/syntax.h>
#include <hatn/db/asyncclient.h>

#include <hatn/api/api.h>
#include <hatn/api/protocol.h>

HATN_API_NAMESPACE_BEGIN

namespace server {

using Threads=common::WithMappedThreads;
using Db=db::AsyncDb;

//! @todo Add tenancy to Env
//! @todo Add logger to Env

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
};

using SimpleEnv = common::EnvType<Threads,Db,ProtocolConfig>;

template <typename EnvT=SimpleEnv>
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

} // namespace server

HATN_API_NAMESPACE_END

HATN_TASK_CONTEXT_DECLARE(HATN_API_NAMESPACE::server::WithEnv<HATN_API_NAMESPACE::server::SimpleEnv>,HATN_API_EXPORT)

#endif // HATNAPISERVERCONTEXT_H

