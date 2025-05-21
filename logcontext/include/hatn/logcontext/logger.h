/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file logcontext/logger.h
  *
  *  Defines context logger.
  *
  */

/****************************************************************************/

#ifndef HATNCONTEXTLOGGER_H
#define HATNCONTEXTLOGGER_H

#include <functional>

#include <hatn/common/objecttraits.h>
#include <hatn/common/flatmap.h>
#include <hatn/common/singleton.h>
#include <hatn/common/pmr/pmrtypes.h>

#include <hatn/base/base.h>
#include <hatn/base/configobject.h>

#include <hatn/logcontext/logcontext.h>
#include <hatn/logcontext/loggerhandler.h>

HATN_LOGCONTEXT_NAMESPACE_BEGIN

constexpr LogLevel DefaultLogLevel=LogLevel::Info;
constexpr uint8_t DefaultDebugVerbosity=0;

class HATN_LOGCONTEXT_EXPORT AppConfig
{
    public:

        AppConfig(const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault())
            : m_factory(factory)
        {}

        virtual ~AppConfig();
        AppConfig(const AppConfig&)=delete;
        AppConfig(AppConfig&&)=default;
        AppConfig& operator=(const AppConfig&)=delete;
        AppConfig& operator=(AppConfig&&)=default;

        const common::pmr::AllocatorFactory* allocatorFactory() const noexcept
        {
            return m_factory;
        }

    private:

        const common::pmr::AllocatorFactory* m_factory;
};

class HATN_LOGCONTEXT_EXPORT LoggerBase
{
    public:

        using levelMapT=common::FlatMap<std::string,LogLevel,std::less<>>;

        Error loadLogConfig(
            const HATN_BASE_NAMESPACE::ConfigTree& configTree,
            const std::string& configPath,
            HATN_BASE_NAMESPACE::config_object::LogRecords& logRecords
        );

        template <typename ContextT>
        static LogLevel contextLogLevel(
                const LoggerBase& logger,
                const ContextT* ctx,
                lib::string_view module=lib::string_view{}
            )
        {
            logger.lockRd();

            // init current level from context
            auto level=ctx->logLevel();

            // figure out current level from tags
            for (auto&& tag: logger.tags())
            {
                if (ctx->containsTag(tag.first))
                {
                    if (tag.second>level)
                    {
                        level=tag.second;
                    }
                }
            }

            // figure out current level by module
            if (!module.empty())
            {
                auto it=logger.modules().find(module);
                if (it!=logger.modules().end())
                {
                    if (it->second>level)
                    {
                        level=it->second;
                    }
                }
            }

            // figure out current level by stack function
            if (!ctx->scopeStack().empty())
            {
                const auto* scope=ctx->currentScope();
                if (scope!=nullptr)
                {
                    auto it=logger.scopes().find(common::lib::string_view(scope->first));
                    if (it!=logger.scopes().end())
                    {
                        if (it->second>level)
                        {
                            level=it->second;
                        }
                    }
                }
            }

            // use default level
            if (level==LogLevel::Default)
            {
                level=logger.defaultLevel();
            }

            // done
            logger.unlockRd();
            return level;
        }

        template <typename ContextT>
        static uint8_t contextDebugVerbosity(
            const LoggerBase& logger,
            const ContextT* ctx
            )
        {
            logger.lockRd();

            // init current verbosity from context
            auto v=ctx->debugVerbosity();

            // use default verbosity
            if (v==0)
            {
                v=logger.defaultDebugVerbosity();
            }

            // done
            logger.unlockRd();
            return v;
        }

        template <typename ContextT>
        static bool passLog(
                const LoggerBase& logger,
                LogLevel level,
                const ContextT* ctx,
                lib::string_view module=lib::string_view{}
            )
        {
            return level<=contextLogLevel(logger,ctx,module);
        }

        template <typename ContextT>
        static bool passDebugLog(
            const LoggerBase& logger,
            const ContextT* ctx,
            uint8_t debugVerbosity=0,
            lib::string_view module=lib::string_view{}
            )
        {
            return LogLevel::Debug<=contextLogLevel(logger,ctx,module) && debugVerbosity<=contextDebugVerbosity(logger,ctx);
        }

        LogLevel defaultLevel() const noexcept
        {
            return m_defaultLevel;
        }

        void setDefaultLogLevel(LogLevel level) noexcept
        {
            m_defaultLevel=level;
        }

        uint8_t defaultDebugVerbosity() const noexcept
        {
            return m_defaultDebugVerbosity;
        }

        void setDefaultDebugVerbosity(uint8_t val) noexcept
        {
            m_defaultDebugVerbosity=val;
        }

        void setTagLevel(std::string tag, LogLevel level)
        {
            common::lib::unique_lock<common::lib::shared_mutex> l(m_mutex);
            m_tags.emplace(std::move(tag),level);
        }

        void unsetTag(const common::lib::string_view& tag)
        {
            common::lib::unique_lock<common::lib::shared_mutex> l(m_mutex);
            m_tags.erase(tag);
        }

        const levelMapT& tags() const noexcept
        {
            return m_tags;
        }

        void clearTags()
        {
            common::lib::unique_lock<common::lib::shared_mutex> l(m_mutex);
            m_tags.clear();
        }

        void setModuleLevel(std::string module, LogLevel level)
        {
            common::lib::unique_lock<common::lib::shared_mutex> l(m_mutex);
            m_modules.emplace(std::move(module),level);
        }

        void unsetModule(const common::lib::string_view& tag)
        {
            common::lib::unique_lock<common::lib::shared_mutex> l(m_mutex);
            m_modules.erase(tag);
        }

        const levelMapT& modules() const noexcept
        {
            return m_modules;
        }

        void clearModules()
        {
            common::lib::unique_lock<common::lib::shared_mutex> l(m_mutex);
            m_modules.clear();
        }

        void setScopeLevel(std::string scope, LogLevel level)
        {
            common::lib::unique_lock<common::lib::shared_mutex> l(m_mutex);
            m_scopes.emplace(std::move(scope),level);
        }

        void unsetScope(const common::lib::string_view& scope)
        {
            common::lib::unique_lock<common::lib::shared_mutex> l(m_mutex);
            m_scopes.erase(scope);
        }

        void clearScopes()
        {
            common::lib::unique_lock<common::lib::shared_mutex> l(m_mutex);
            m_scopes.clear();
        }

        const levelMapT& scopes() const noexcept
        {
            return m_scopes;
        }

        void reset()
        {
            m_defaultLevel=DefaultLogLevel;
            clearScopes();
            clearModules();
            clearTags();
        }

    private:

        void lockWr() const
        {
            m_mutex.lock();
        }

        void unlockWr() const
        {
            m_mutex.unlock();
        }

        void lockRd() const
        {
            m_mutex.lock_shared();
        }

        void unlockRd() const
        {
            m_mutex.unlock_shared();
        }

        LogLevel m_defaultLevel=DefaultLogLevel;
        uint8_t m_defaultDebugVerbosity=DefaultDebugVerbosity;

        levelMapT m_tags;
        levelMapT m_modules;
        levelMapT m_scopes;

        mutable common::lib::shared_mutex m_mutex;
};

template <typename Traits>
class LoggerT : public LoggerBase,
                public common::WithTraits<Traits>
{
    public:

        using baseWithTraits=common::WithTraits<Traits>;
        using levelMapT=common::FlatMap<std::string,LogLevel,std::less<>>;

        using baseWithTraits::baseWithTraits;

        template <typename ContextT>
        void log(
            LogLevel level,
            const ContextT* ctx,
            const char* msg,
            const ImmediateRecords& records,
            lib::string_view module
            )
        {
            this->traits().log(level,ctx,msg,records,module);
        }

        template <typename ContextT>
        void logError(
            LogLevel level,
            const Error& ec,
            const ContextT* ctx,
            const char* msg,
            const ImmediateRecords& records,
            lib::string_view module
            )
        {
            this->traits().logError(level,ec,ctx,msg,records,module);
        }

        template <typename ContextT>
        void logClose(
            LogLevel level,
            const Error& ec,
            const ContextT* ctx,
            const char* msg,
            const ImmediateRecords& records,
            lib::string_view module
            )
        {
            this->traits().logClose(level,ec,ctx,msg,records,module);
        }

        template <typename ContextT>
        void logCloseApi(
            LogLevel level,
            const Error& ec,
            const ContextT* ctx,
            const char* msg,
            const ImmediateRecords& records,
            lib::string_view module
            )
        {
            this->traits().logCloseApi(level,ec,ctx,msg,records,module);
        }

        template <typename ContextT>
        void log(
            LogLevel level,
            const ContextT* ctx,
            const char* msg,
            lib::string_view module=lib::string_view{}
            )
        {
            this->traits().log(level,ctx,msg,module);
        }

        template <typename ContextT>
        void logError(
            LogLevel level,
            const Error& ec,
            const ContextT* ctx,
            const char* msg,
            lib::string_view module=lib::string_view{}
            )
        {
            this->traits().logError(level,ec,ctx,msg,module);
        }

        template <typename ContextT>
        void logClose(
            LogLevel level,
            const Error& ec,
            const ContextT* ctx,
            const char* msg,
            lib::string_view module=lib::string_view{}
            )
        {
            this->traits().logClose(level,ec,ctx,msg,module);
        }

        template <typename ContextT>
        void logCloseApi(
            LogLevel level,
            const Error& ec,
            const ContextT* ctx,
            const char* msg,
            lib::string_view module=lib::string_view{}
            )
        {
            this->traits().logCloseApi(level,ec,ctx,msg,module);
        }

        Error close()
        {
            return this->traits().close();
        }

        Error start()
        {
            return this->traits().start();
        }

        void setAppConfig(const AppConfig& cfg)
        {
            this->traits().setAppConfig(cfg);
        }
};

template <typename ContextT>
using LoggerWithHandler=LoggerT<LoggerHandlerTraits<ContextT>>;

HATN_LOGCONTEXT_NAMESPACE_END

#endif // HATNCONTEXTLOGGER_H
