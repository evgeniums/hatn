/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file logcontext/logger.сpp
  *
  *      Contains definition of logger.
  *
  */

#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/ipp/syntax.ipp>

#include <hatn/base/configobject.h>

#include <hatn/logcontext/loggererror.h>
#include <hatn/logcontext/logger.h>

HATN_LOGCONTEXT_NAMESPACE_BEGIN

namespace {

HDU_UNIT(mapped_level,
    HDU_FIELD(name,TYPE_STRING,1)
    HDU_FIELD(level,TYPE_STRING,2)
)

HDU_UNIT(logger_config,
    HDU_FIELD(level,TYPE_STRING,1)
    HDU_FIELD(debug_verbosity,TYPE_UINT8,2)
    HDU_REPEATED_FIELD(tags,mapped_level::TYPE,3)
    HDU_REPEATED_FIELD(modules,mapped_level::TYPE,4)
    HDU_REPEATED_FIELD(scopes,mapped_level::TYPE,5)
)

using LoggerConfig =  HATN_BASE_NAMESPACE::ConfigObject<logger_config::type>;

}

//---------------------------------------------------------------

AppConfig::~AppConfig()
{}

//---------------------------------------------------------------

Error LoggerBase::loadLogConfig(
        const HATN_BASE_NAMESPACE::ConfigTree& configTree,
        const std::string& configPath,
        HATN_BASE_NAMESPACE::config_object::LogRecords& logRecords
    )
{
    lib::unique_lock<lib::shared_mutex> l{m_mutex};

    // load config
    LoggerConfig cfg;
    auto ec=cfg.loadLogConfig(configTree,configPath,logRecords);
    HATN_CHECK_EC(ec)

    if (cfg.config().isSet(logger_config::level))
    {
        auto logLevel=parseLogLevel(cfg.config().fieldValue(logger_config::level));
        if (logLevel==LogLevel::Unknown)
        {
            return logcontextError(LogContextError::INVALID_DEFAULT_LOG_LEVEL);
        }
        setDefaultLogLevel(logLevel);
    }
    if (cfg.config().isSet(logger_config::debug_verbosity))
    {
        setDefaultDebugVerbosity(cfg.config().fieldValue(logger_config::debug_verbosity));
    }

    auto parseLevel=[](lib::string_view itemName, lib::string_view levelName, LogContextError errCode) -> Result<LogLevel>
    {
        auto logLevel=parseLogLevel(levelName);
        if (logLevel==LogLevel::Unknown)
        {
            auto nativeErr=std::make_shared<common::NativeError>(fmt::format("{}={}",itemName,levelName));
            return logcontextError(errCode,std::move(nativeErr));
        }
        return logLevel;
    };

#if 1
    const auto& tags=cfg.config().field(logger_config::tags);
    if (tags.isSet())
    {
        for (size_t i=0;i<tags.count();i++)
        {
            lib::string_view name=tags.at(i).field(mapped_level::name).value();
            lib::string_view levelName=tags.at(i).field(mapped_level::level).value();
            auto r=parseLevel(name,levelName,LogContextError::INVALID_TAG_LOG_LEVEL);
            HATN_CHECK_RESULT(r)
            setTagLevel(std::string{name},r.value());
        }
    }

    const auto& modules=cfg.config().field(logger_config::modules);
    if (modules.isSet())
    {
        for (size_t i=0;i<modules.count();i++)
        {
            lib::string_view name=modules.at(i).field(mapped_level::name).value();
            lib::string_view levelName=modules.at(i).field(mapped_level::level).value();
            auto r=parseLevel(name,levelName,LogContextError::INVALID_MODULE_LOG_LEVEL);
            HATN_CHECK_RESULT(r)
            setModuleLevel(std::string{name},r.value());
        }
    }

    const auto& scopes=cfg.config().field(logger_config::scopes);
    if (scopes.isSet())
    {
        for (size_t i=0;i<scopes.count();i++)
        {
            lib::string_view name=scopes.at(i).field(mapped_level::name).value();
            lib::string_view levelName=scopes.at(i).field(mapped_level::level).value();
            auto r=parseLevel(name,levelName,LogContextError::INVALID_SCOPE_LOG_LEVEL);
            HATN_CHECK_RESULT(r)
            setScopeLevel(std::string{name},r.value());
        }
    }
#else

    // parse tags
    auto tagsPath=HATN_BASE_NAMESPACE::ConfigTreePath{configPath}.copyAppend("tags");
    if (configTree.isSet(tagsPath))
    {
        Error ec;
        const auto& m=configTree.get(tagsPath)->asMap(ec);
        HATN_CHECK_EC(ec);
        for (auto&& it: m)
        {
            auto name=it.first;
            auto levelName=it.second->asString();
            HATN_CHECK_RESULT(levelName)
            auto r=parseLevel(name,levelName.value(),LogContextError::INVALID_TAG_LOG_LEVEL);
            HATN_CHECK_RESULT(r)
            setTagLevel(std::move(name),r.value());
        }
    }

    // parse modules
    auto modulesPath=HATN_BASE_NAMESPACE::ConfigTreePath{configPath}.copyAppend("modules");
    if (configTree.isSet(modulesPath))
    {
        Error ec;
        const auto& m=configTree.get(modulesPath)->asMap(ec);
        HATN_CHECK_EC(ec);
        for (auto&& it: m)
        {
            auto name=it.first;
            auto levelName=it.second->asString();
            HATN_CHECK_RESULT(levelName)
            auto r=parseLevel(name,levelName.value(),LogContextError::INVALID_MODULE_LOG_LEVEL);
            HATN_CHECK_RESULT(r)
            setModuleLevel(std::move(name),r.value());
        }
    }

    // parse scopes
    auto scopesPath=HATN_BASE_NAMESPACE::ConfigTreePath{configPath}.copyAppend("scopes");
    if (configTree.isSet(scopesPath))
    {
        Error ec;
        const auto& m=configTree.get(scopesPath)->asMap(ec);
        HATN_CHECK_EC(ec);
        for (auto&& it: m)
        {
            auto name=it.first;
            auto levelName=it.second->asString();
            HATN_CHECK_RESULT(levelName)
            auto r=parseLevel(name,levelName.value(),LogContextError::INVALID_SCOPE_LOG_LEVEL);
            HATN_CHECK_RESULT(r)
            setScopeLevel(std::move(name),r.value());
        }
    }

#endif
    // done
    return OK;
}

//---------------------------------------------------------------

HATN_LOGCONTEXT_NAMESPACE_END
