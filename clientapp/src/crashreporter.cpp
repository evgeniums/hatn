/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientapp/crashreporter.cpp
  */

#include <hatn/logcontext/contextlogger.h>

#include <hatn/clientapp/clientapp.h>
#include <hatn/clientapp/crashreporter.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

CrashReporter::~CrashReporter() = default;

//--------------------------------------------------------------------------

void CrashReporterRegistry::registerProvider(std::shared_ptr<CrashReporter> reporter)
{
    m_reporters[reporter->name()] = std::move(reporter);
}

//--------------------------------------------------------------------------

void CrashReporterRegistry::selectProvider(ClientApp* clientApp,
                                            const std::string& scheme,
                                            const std::string& section)
{
    auto it = m_reporters.find(scheme);
    if (it == m_reporters.end())
    {
        HATN_CTX_WARN_RECORDS("crash reporter not registered, ignoring",
            {"scheme", scheme})
        return;
    }

    auto ec = it->second->init(clientApp, section);
    if (ec)
    {
        HATN_CTX_ERROR_RECORDS(ec, "failed to init crash reporter",
            {"scheme", scheme})
        return;
    }

    m_active = it->second.get();
    HATN_CTX_INFO_RECORDS("crash reporter selected",
        {"scheme", scheme})
}

//--------------------------------------------------------------------------

HATN_CLIENTAPP_NAMESPACE_END
