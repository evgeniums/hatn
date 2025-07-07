/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file app/appenv.h
  *
  */

/****************************************************************************/

#ifndef HATNAPPENV_H
#define HATNAPPENV_H

#include <hatn/common/env.h>
#include <hatn/common/translate.h>
#include <hatn/common/logger.h>

#include <hatn/logcontext/withlogger.h>
#include <hatn/logcontext/context.h>

#include <hatn/crypt/ciphersuite.h>

#include <hatn/db/asyncclient.h>

#include <hatn/app/appdefs.h>

HATN_APP_NAMESPACE_BEGIN

using Logger=HATN_LOGCONTEXT_NAMESPACE::WithLogger;
using LoggerHandlerBuilder=HATN_LOGCONTEXT_NAMESPACE::LoggerHandlerBuilder;
using Db=HATN_DB_NAMESPACE::AsyncDb;
using AllocatorFactory=common::pmr::WithFactory;
using CipherSuites=HATN_CRYPT_NAMESPACE::WithCipherSuites;
using Translator=common::WithTranslator;
using Threads=common::WithMappedThreads;

using AppEnv=common::Env<AllocatorFactory,Threads,Logger,Db,CipherSuites,Translator>;

class WithAppEnv
{
    public:

        WithAppEnv(common::SharedPtr<AppEnv> env={}) : m_env(std::move(env))
        {}

        void setEnv(common::SharedPtr<AppEnv> env)
        {
            m_env=std::move(env);
        }

        common::SharedPtr<AppEnv> envShared() const noexcept
        {
            return m_env;
        }

        const AppEnv* env() const noexcept
        {
            return m_env.get();
        }

        AppEnv* env() noexcept
        {
            return m_env.get();
        }

        void setAppEnv(common::SharedPtr<AppEnv> env)
        {
            m_env=std::move(env);
        }

        common::SharedPtr<AppEnv> appEnvShared() const noexcept
        {
            return m_env;
        }

        const AppEnv* appEnv() const noexcept
        {
            return m_env.get();
        }

        AppEnv* appEnv() noexcept
        {
            return m_env.get();
        }

    private:

        common::SharedPtr<AppEnv> m_env;
};

template <typename Contexts, typename BaseT=common::BaseEnv>
using EnvWithAppEnvT = common::WithEmbeddedEnvT<AppEnv,Contexts,BaseT>;

using AppEnvContext=common::TaskContextType<WithAppEnv,HATN_LOGCONTEXT_NAMESPACE::TaskLogContext>;

struct makeAppEnvContextT
{
    template <typename ...BaseArgs>
    auto operator()(common::SharedPtr<AppEnv> env, BaseArgs&&... args) const
    {
        return common::makeTaskContextType<AppEnvContext>(
            common::subcontexts(
                common::subcontext(WithAppEnv{std::move(env)})
            ),
            std::forward<BaseArgs>(args)...
            );
    }
};
constexpr makeAppEnvContextT makeAppEnvContext{};

struct allocateAppEnvContextT
{
    template <typename ...BaseArgs>
    auto operator()(common::SharedPtr<AppEnv> env, BaseArgs&&... args) const
    {
        auto allocator=env->template get<AllocatorFactory>().factory()->objectAllocator<AppEnvContext>();
        return common::allocateTaskContextType<AppEnvContext>(
            allocator,
            common::subcontexts(
                common::subcontext(WithAppEnv{std::move(env)})
            ),
            std::forward<BaseArgs>(args)...
            );
    }
};
constexpr allocateAppEnvContextT allocateAppEnvContext{};

HATN_APP_NAMESPACE_END

HATN_TASK_CONTEXT_DECLARE_EXPORT(HATN_APP_NAMESPACE::WithAppEnv,HATN_APP_EXPORT)

#endif // HATNAPPENV_H
