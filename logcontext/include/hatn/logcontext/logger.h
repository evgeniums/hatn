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

#include <hatn/logcontext/logcontext.h>
#include <hatn/logcontext/context.h>

HATN_LOGCONTEXT_NAMESPACE_BEGIN

constexpr LogLevel DefaultLogLevel=LogLevel::Info;

template <typename ContextT=Context>
class LoggerBaseT
{
    public:

        using contextT=ContextT;
        using levelMapT=common::FlatMap<std::string,LogLevel,std::less<>>;

        static LogLevel contextLogLevel(
                const LoggerBaseT& logger,
                const contextT* ctx,
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
                const auto& scope=ctx->currentScope();
                auto it=logger.scopes().find(common::lib::string_view(scope.first));
                if (it!=logger.scopes().end())
                {
                    if (it->second>level)
                    {
                        level=it->second;
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

        static bool passLog(
                const LoggerBaseT& logger,
                LogLevel level,
                const contextT* ctx,
                lib::string_view module=lib::string_view{}
            )
        {
            return level<=contextLogLevel(logger,ctx,module);
        }

        LogLevel defaultLevel() const noexcept
        {
            return m_defaultLevel;
        }

        void setDefaultLogLevel(LogLevel level) noexcept
        {
            m_defaultLevel=level;
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

        levelMapT m_tags;
        levelMapT m_modules;
        levelMapT m_scopes;

        mutable common::lib::shared_mutex m_mutex;
};

using LoggerBase=LoggerBaseT<>;

template <typename ContextT=Context>
class LoggerHandlerT
{
    public:

        virtual ~LoggerHandlerT()
        {}

        LoggerHandlerT()=default;
        LoggerHandlerT(const LoggerHandlerT&)=default;
        LoggerHandlerT(LoggerHandlerT&&)=default;
        LoggerHandlerT& operator=(const LoggerHandlerT&)=default;
        LoggerHandlerT& operator=(LoggerHandlerT&&)=default;

        virtual void log(
            LogLevel level,
            const ContextT* ctx,
            common::pmr::string msg,
            lib::string_view module=lib::string_view{},
            common::pmr::vector<Record> records=common::pmr::vector<Record>{}
        )=0;

        virtual void logError(
            LogLevel level,
            const Error& ec,
            const ContextT* ctx,
            common::pmr::string msg,
            lib::string_view module=lib::string_view{},
            common::pmr::vector<Record> records=common::pmr::vector<Record>{}
        )=0;

        virtual void logClose(
            LogLevel level,
            const Error& ec,
            const ContextT* ctx,
            common::pmr::string msg,
            lib::string_view module=lib::string_view{},
            common::pmr::vector<Record> records=common::pmr::vector<Record>{}
        )=0;

        virtual void logCloseApi(
            LogLevel level,
            const Error& ec,
            const ContextT* ctx,
            common::pmr::string msg,
            lib::string_view module=lib::string_view{},
            common::pmr::vector<Record> records=common::pmr::vector<Record>{}
        )=0;
};
using LoggerHandler=LoggerHandlerT<>;

template <typename ContextT=Context>
class LoggerHandlerTraitsT
{
    public:

        LoggerHandlerTraitsT(std::shared_ptr<LoggerHandlerT<ContextT>> handler)
            : m_handler(std::move(handler))
        {}

        void log(
            LogLevel level,
            const ContextT* ctx,
            common::pmr::string msg,
            lib::string_view module=lib::string_view{},
            common::pmr::vector<Record> records=common::pmr::vector<Record>{}
        )
        {
            m_handler->log(level,ctx,std::move(msg),module,std::move(records));
        }

        void logError(
            LogLevel level,
            const Error& ec,
            const ContextT* ctx,
            common::pmr::string msg,
            lib::string_view module=lib::string_view{},
            common::pmr::vector<Record> records=common::pmr::vector<Record>{}
        )
        {
            m_handler->logError(level,ec,ctx,std::move(msg),module,std::move(records));
        }

        void logClose(
            LogLevel level,
            const Error& ec,
            const ContextT* ctx,
            common::pmr::string msg,
            lib::string_view module=lib::string_view{},
            common::pmr::vector<Record> records=common::pmr::vector<Record>{}
        )
        {
            m_handler->logClose(level,ec,ctx,std::move(msg),module,std::move(records));
        }

        void logCloseApi(
            LogLevel level,
            const Error& ec,
            const ContextT* ctx,
            common::pmr::string msg,
            lib::string_view module=lib::string_view{},
            common::pmr::vector<Record> records=common::pmr::vector<Record>{}
        )
        {
            m_handler->logCloseApi(level,ec,ctx,std::move(msg),module,std::move(records));
        }

    private:

        std::shared_ptr<LoggerHandlerT<ContextT>> m_handler;
};
using LoggerHandlerTraits=LoggerHandlerTraitsT<>;

template <template <typename> class Traits=LoggerHandlerTraitsT, typename ContextT=Context>
class LoggerT : public LoggerBaseT<ContextT>,
                public common::WithTraits<Traits<ContextT>>
{
    public:

        using baseWithTraits=common::WithTraits<Traits<ContextT>>;
        using contextT=ContextT;
        using levelMapT=common::FlatMap<std::string,LogLevel,std::less<>>;

        using baseWithTraits::baseWithTraits;

        void log(
            LogLevel level,
            const ContextT* ctx,
            common::pmr::string msg,
            lib::string_view module=lib::string_view{},
            common::pmr::vector<Record> records=common::pmr::vector<Record>{}
            )
        {
            this->traits().log(level,ctx,std::move(msg),module,std::move(records));
        }

        void logError(
            LogLevel level,
            const Error& ec,
            const ContextT* ctx,
            common::pmr::string msg,
            lib::string_view module=lib::string_view{},
            common::pmr::vector<Record> records=common::pmr::vector<Record>{}
            )
        {
            this->traits().logError(level,ec,ctx,std::move(msg),module,std::move(records));
        }

        void logClose(
            LogLevel level,
            const Error& ec,
            const ContextT* ctx,
            common::pmr::string msg,
            lib::string_view module=lib::string_view{},
            common::pmr::vector<Record> records=common::pmr::vector<Record>{}
            )
        {
            this->traits().logClose(level,ec,ctx,std::move(msg),module,std::move(records));
        }

        void logCloseApi(
            LogLevel level,
            const Error& ec,
            const ContextT* ctx,
            common::pmr::string msg,
            lib::string_view module=lib::string_view{},
            common::pmr::vector<Record> records=common::pmr::vector<Record>{}
            )
        {
            this->traits().logCloseApi(level,ec,ctx,std::move(msg),module,std::move(records));
        }
};

using Logger=LoggerT<>;

HATN_LOGCONTEXT_NAMESPACE_END

#endif // HATNCONTEXTLOGGER_H
