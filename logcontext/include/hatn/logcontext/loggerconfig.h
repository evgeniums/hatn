/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file logcontext/loggerconfig.h
  *
  * Shared logger-config HDU schema used by LoggerBase::loadLogConfig,
  * SystemService bridge methods (get/set_logger_config), and the desktop
  * DeveloperSettings controller.
  *
  * File-settings path for custom (persisted) logger config:
  *   LoggerConfigSettingsPath  →  "developer.loggerConfig"
  *
  * Log-level name strings accepted by parseLogLevel():
  *   INFO, DETAILS, ERROR, WARN, DEBUG, TRACE, FATAL, NONE
  */

/****************************************************************************/

#ifndef HATNLOGGERCONFIG_H
#define HATNLOGGERCONFIG_H

#include <hatn/base/configobject.h>

#include <hatn/dataunit/syntax.h>

#include <hatn/logcontext/logcontext.h>

HATN_LOGCONTEXT_NAMESPACE_BEGIN

/** @brief Canonical file-settings path where the persisted logger config is stored. */
constexpr const char* LoggerConfigSettingsPath = "developer.loggerConfig";

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

using LoggerConfig = HATN_BASE_NAMESPACE::ConfigObject<logger_config::type>;

HATN_LOGCONTEXT_NAMESPACE_END

#endif // HATNLOGGERCONFIG_H
