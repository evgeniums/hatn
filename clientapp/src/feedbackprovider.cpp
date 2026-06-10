/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientapp/feedbackprovider.cpp
  */

#include <hatn/logcontext/contextlogger.h>

#include <hatn/clientapp/clientapp.h>
#include <hatn/clientapp/clientapperror.h>
#include <hatn/clientapp/feedbackprovider.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

FeedbackProvider::~FeedbackProvider() = default;

//--------------------------------------------------------------------------

void FeedbackProvider::sendFeedbackDirect(
    std::string /*dsn*/,
    std::string /*release*/,
    std::string /*environment*/,
    std::string /*text*/,
    std::function<void(const Error&)> callback
)
{
    callback(clientAppError(ClientAppError::FEEDBACK_PROVIDER_NOT_CONFIGURED));
}

//--------------------------------------------------------------------------

void FeedbackProviderRegistry::registerProvider(std::shared_ptr<FeedbackProvider> provider)
{
    m_providers[provider->name()] = std::move(provider);
}

//--------------------------------------------------------------------------

void FeedbackProviderRegistry::selectProvider(ClientApp* clientApp,
                                               const std::string& scheme,
                                               const std::string& section)
{
    auto it = m_providers.find(scheme);
    if (it == m_providers.end())
    {
        HATN_CTX_WARN_RECORDS("feedback provider not registered, ignoring",
            {"scheme", scheme})
        return;
    }

    auto ec = it->second->init(clientApp, section);
    if (ec)
    {
        HATN_CTX_ERROR_RECORDS(ec, "failed to init feedback provider",
            {"scheme", scheme})
        return;
    }

    m_active = it->second.get();
    HATN_CTX_INFO_RECORDS("feedback provider selected",
        {"scheme", scheme})
}

//--------------------------------------------------------------------------

//--------------------------------------------------------------------------

FeedbackProvider* FeedbackProviderRegistry::findProvider(const std::string& scheme) const noexcept
{
    auto it = m_providers.find(scheme);
    if (it == m_providers.end())
    {
        return nullptr;
    }
    return it->second.get();
}

//--------------------------------------------------------------------------

HATN_CLIENTAPP_NAMESPACE_END
