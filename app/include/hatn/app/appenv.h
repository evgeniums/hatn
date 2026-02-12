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
#include <hatn/common/withsharedvalue.h>

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

class Databases
{
    public:

        void add(std::string name, std::shared_ptr<Db> db)
        {
            common::MutexScopedLock l{m_mutex};
            m_dbs.emplace(std::move(name),std::move(db));
        }

        std::shared_ptr<Db>& get(lib::string_view name)
        {
            common::MutexScopedLock l{m_mutex};
            static std::shared_ptr<Db> none;

            auto it=m_dbs.find(name);
            if (it!=m_dbs.end())
            {
                return it->second;
            }

            return none;
        }

        const std::shared_ptr<Db>& get(lib::string_view name) const
        {
            common::MutexScopedLock l{m_mutex};
            static std::shared_ptr<Db> none;

            auto it=m_dbs.find(name);
            if (it!=m_dbs.end())
            {
                return it->second;
            }

            return none;
        }

        void remove(const std::string& name)
        {
            common::MutexScopedLock l{m_mutex};
            m_dbs.erase(name);
        }

        void clear()
        {
            common::MutexScopedLock l{m_mutex};
            m_dbs.clear();
        }

        Error close()
        {
            Error ec;

            for (auto& db: m_dbs)
            {
                auto ec1=db.second->close();
                if (ec1)
                {
                    if (!ec)
                    {
                        ec=std::move(ec1);
                    }
                }
            }

            return ec;
        }

    private:

        mutable common::MutexLock m_mutex;
        common::FlatMap<std::string,std::shared_ptr<Db>,std::less<void>> m_dbs;
};

using WithDatabases = common::WithStdSharedValue<Databases>;
using Dbs=WithDatabases;

// using AppEnv=common::Env<AllocatorFactory,Threads,Logger,Db,Dbs,CipherSuites,Translator>;

class AppEnv : public common::Env<AllocatorFactory,Threads,Logger,Db,Dbs,CipherSuites,Translator>
{
    public:

        using common::Env<AllocatorFactory,Threads,Logger,Db,Dbs,CipherSuites,Translator>::Env;

        void lock() const
        {
            m_mutex.lock();
        }

        void unlock() const
        {
            m_mutex.unlock();
        }

        virtual void clear()
        {}

    private:

        mutable common::MutexLock m_mutex;
};

template <typename FromT, typename ToT>
inline void cloneAppEnv(const FromT& from, ToT& to, bool ignoreDatabases=true)
{
    to.template get<AllocatorFactory>().setFactory(from.template get<AllocatorFactory>().factory());
    to.template get<Threads>().setMappedThreads(from.template get<Threads>().threads());
    to.template get<Logger>().setLogger(from.template get<Logger>().loggerShared());
    if (!ignoreDatabases)
    {
        to.template get<Db>().setDbClient(from.template get<Db>().dbClient());
        to.template get<Db>().setDbClients(from.template get<Db>().dbClients());
        to.template get<Dbs>().setValue(from.template get<Dbs>().sharedValue());
    }
    to.template get<CipherSuites>().setSuites(from.template get<CipherSuites>().suitesShared());
    to.template get<Translator>().setTranslator(from.template get<Translator>().translatorShared());
}

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
