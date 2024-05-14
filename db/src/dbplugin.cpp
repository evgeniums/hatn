/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/** @file db/dbplugin.cpp
  *
  *   Base class for database plugins.
  *
  */

/****************************************************************************/

#include <iostream>

#include <hatn/db/dbplugin.h>

HATN_DB_NAMESPACE_BEGIN

/*********************** DbPlugin **************************/

//---------------------------------------------------------------

DbPlugin::DbPlugin(
        const common::PluginInfo* pluginInfo
    ) noexcept : common::Plugin(pluginInfo)
{
}

//---------------------------------------------------------------

HATN_DB_NAMESPACE_END
