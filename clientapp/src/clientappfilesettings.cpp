/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientapp/clientappfilesettings.cpp
  *
  */

#include <boost/filesystem.hpp>

#include <hatn/common/fileutils.h>

#include <hatn/base/configtreejson.h>

#include <hatn/logcontext/postasync.h>

#include <hatn/app/app.h>

#include <hatn/clientapp/clientapperror.h>
#include <hatn/clientapp/clientapp.h>
#include <hatn/clientapp/clientappfilesettings.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

//---------------------------------------------------------------

ClientAppFileSettings::ClientAppFileSettings(ClientApp* app)
    : m_app(app)
{}

//---------------------------------------------------------------

Error ClientAppFileSettings::load(const std::string& filePath)
{
    m_filePath = filePath;
    m_configTree.reset();

    std::string backupPath = common::FileUtils::backupName(m_filePath.c_str());
    bool primaryExists = boost::filesystem::exists(m_filePath);
    bool backupExists  = boost::filesystem::exists(backupPath);

    if (!primaryExists && !backupExists)
    {
        return OK;
    }

    std::string buf;
    auto ec = common::FileUtils::loadFromFileWithBackup(buf, m_filePath.c_str());
    if (ec)
    {
        return ec;
    }

    HATN_BASE_NAMESPACE::ConfigTreeJson parser;
    ec = parser.parse(m_configTree, buf);
    if (ec)
    {
        m_configTree.reset();
        return ec;
    }

    return OK;
}

//---------------------------------------------------------------

Error ClientAppFileSettings::saveLocked()
{
    HATN_BASE_NAMESPACE::ConfigTreeJson serializer;
    auto r = serializer.serialize(m_configTree);
    if (r)
    {
        return r.takeError();
    }
    return common::FileUtils::saveToFileWithBackup(r.value(), m_filePath.c_str());
}

//---------------------------------------------------------------

void ClientAppFileSettings::publishChange(std::string section)
{
    auto event = std::make_shared<AppSettingsEvent>();
    event->event = std::move(section);
    m_app->eventDispatcher().publish(
        m_app->app().env(),
        HATN_LOGCONTEXT_NAMESPACE::makeLogCtx(),
        std::move(event)
    );
}

//---------------------------------------------------------------

Error ClientAppFileSettings::apply(Mutator mutator, std::string section)
{
    Error ec;
    {
        common::MutexScopedLock l{m_mutex};
        ec = mutator(m_configTree);
        if (ec)
        {
            return ec;
        }
        ec = saveLocked();
    }
    if (ec)
    {
        return ec;
    }
    publishChange(std::move(section));
    return OK;
}

//---------------------------------------------------------------

Error ClientAppFileSettings::save(std::string section)
{
    Error ec;
    {
        common::MutexScopedLock l{m_mutex};
        ec = saveLocked();
    }
    if (ec)
    {
        return ec;
    }
    publishChange(std::move(section));
    return OK;
}

//---------------------------------------------------------------

Result<std::string> ClientAppFileSettings::getAsString(const HATN_BASE_NAMESPACE::ConfigTreePath& path) const
{
    return getAs<std::string>(path);
}

Result<bool> ClientAppFileSettings::getAsBool(const HATN_BASE_NAMESPACE::ConfigTreePath& path) const
{
    return getAs<bool>(path);
}

Result<double> ClientAppFileSettings::getAsDouble(const HATN_BASE_NAMESPACE::ConfigTreePath& path) const
{
    return getAs<double>(path);
}

Result<int64_t> ClientAppFileSettings::getAsInt(const HATN_BASE_NAMESPACE::ConfigTreePath& path) const
{
    return getAs<int64_t>(path);
}

Result<uint64_t> ClientAppFileSettings::getAsUInt(const HATN_BASE_NAMESPACE::ConfigTreePath& path) const
{
    return getAs<uint64_t>(path);
}

//---------------------------------------------------------------

std::string ClientAppFileSettings::getAsString(const HATN_BASE_NAMESPACE::ConfigTreePath& path, common::Error& ec) const
{
    return getAs<std::string>(path, ec);
}

bool ClientAppFileSettings::getAsBool(const HATN_BASE_NAMESPACE::ConfigTreePath& path, common::Error& ec) const
{
    return getAs<bool>(path, ec);
}

double ClientAppFileSettings::getAsDouble(const HATN_BASE_NAMESPACE::ConfigTreePath& path, common::Error& ec) const
{
    return getAs<double>(path, ec);
}

int64_t ClientAppFileSettings::getAsInt(const HATN_BASE_NAMESPACE::ConfigTreePath& path, common::Error& ec) const
{
    return getAs<int64_t>(path, ec);
}

uint64_t ClientAppFileSettings::getAsUInt(const HATN_BASE_NAMESPACE::ConfigTreePath& path, common::Error& ec) const
{
    return getAs<uint64_t>(path, ec);
}

//---------------------------------------------------------------

Error ClientAppFileSettings::setJson(
    const HATN_BASE_NAMESPACE::ConfigTreePath& path,
    const std::string& json,
    std::string section
)
{
    // Parse into a temporary tree outside the lock so invalid JSON
    // can never corrupt the live tree state.
    HATN_BASE_NAMESPACE::ConfigTree tmp;
    HATN_BASE_NAMESPACE::ConfigTreeJson parser;
    auto ec = parser.parse(tmp, json);
    if (ec)
    {
        return ec;
    }

    // Atomically replace (remove old, merge new) and flush.
    return apply(
        [&](HATN_BASE_NAMESPACE::ConfigTree& tree)
        {
            tree.remove(path);
            return tree.merge(std::move(tmp), path);
        },
        std::move(section)
    );
}

//---------------------------------------------------------------

Result<std::string> ClientAppFileSettings::getJson(
    const HATN_BASE_NAMESPACE::ConfigTreePath& path
) const
{
    common::MutexScopedLock l{m_mutex};
    if (!m_configTree.isSet(path, true))
    {
        return clientAppError(ClientAppError::APPLICATION_SETTING_NOT_SET);
    }
    HATN_BASE_NAMESPACE::ConfigTreeJson serializer;
    return serializer.serialize(m_configTree, path);
}

//---------------------------------------------------------------

std::string ClientAppFileSettings::getJson(
    const HATN_BASE_NAMESPACE::ConfigTreePath& path,
    common::Error& ec
) const
{
    auto r = getJson(path);
    HATN_RESULT_EC(r, ec)
    return r.takeValue();
}

//---------------------------------------------------------------

HATN_CLIENTAPP_NAMESPACE_END
