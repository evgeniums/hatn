/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientapp/logsprovider.cpp
  */

#include <hatn/logcontext/contextlogger.h>

#include <hatn/clientapp/clientapp.h>
#include <hatn/clientapp/logsprovider.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

LogsProvider::~LogsProvider() = default;

//--------------------------------------------------------------------------

void LogsProviderRegistry::registerProvider(std::shared_ptr<LogsProvider> provider)
{
    m_providers[provider->name()] = std::move(provider);
}

//--------------------------------------------------------------------------

void LogsProviderRegistry::selectProvider(ClientApp* clientApp,
                                           const std::string& scheme,
                                           const std::string& section)
{
    auto it = m_providers.find(scheme);
    if (it == m_providers.end())
    {
        HATN_CTX_WARN_RECORDS("logs provider not registered, ignoring",
            {"scheme", scheme})
        return;
    }

    auto ec = it->second->init(clientApp, section);
    if (ec)
    {
        HATN_CTX_ERROR_RECORDS(ec, "failed to init logs provider",
            {"scheme", scheme})
        return;
    }

    m_active = it->second.get();
    HATN_CTX_INFO_RECORDS("logs provider selected",
        {"scheme", scheme})
}

//--------------------------------------------------------------------------

HATN_CLIENTAPP_NAMESPACE_END
