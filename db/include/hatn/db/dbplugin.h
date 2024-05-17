/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/** @file db/dbplugin.h
  *
  *   Base class for database plugins
  *
  */

/****************************************************************************/

#ifndef HATNDBPLUGIN_H
#define HATNDBPLUGIN_H

#include <hatn/common/plugin.h>

#include <hatn/db/db.h>

HATN_DB_NAMESPACE_BEGIN

//! Base class for database plugins.
class HATN_DB_EXPORT DbPlugin : public common::Plugin
{
    public:

        //! Db plugin type
        constexpr static const char* Type="com.github.evgeniums.hatn.dbplugin";

        //! Ctor
        explicit DbPlugin(
            const common::PluginInfo* pluginInfo
        ) noexcept;

        virtual Error init()
        {
            return OK;
        }

        virtual Error cleanup()
        {
            return OK;
        }
};

HATN_DB_NAMESPACE_END

#endif // HATNDBPLUGIN_H
