/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientapp/feedbackprovider.h
  *
  * Abstract feedback provider and its registry.
  *
  * A feedback provider delivers user-submitted feedback text to an external
  * destination (e.g. Sentry user feedback, a webhook, etc.). The active
  * provider is selected at startup from config; if none is configured the
  * registry returns nullptr and bridge callers must return
  * ClientAppError::FEEDBACK_PROVIDER_NOT_CONFIGURED.
  */

/****************************************************************************/

#ifndef HATNFEEDBACKPROVIDER_H
#define HATNFEEDBACKPROVIDER_H

#include <string>
#include <functional>
#include <map>
#include <memory>

#include <hatn/common/error.h>
#include <hatn/common/taskcontext.h>

#include <hatn/clientapp/clientappdefs.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

class ClientApp;

class HATN_CLIENTAPP_EXPORT FeedbackProvider
{
    public:

        virtual ~FeedbackProvider();

        virtual const char* name() const noexcept = 0;

        virtual Error init(ClientApp* clientApp, const std::string& section) = 0;

        virtual void sendFeedback(
            common::SharedPtr<common::TaskContext> ctx,
            std::string recipientId,
            std::string text,
            std::function<void(const Error&)> callback
        ) = 0;

        // Account-level feedback: send using the provided provider config directly,
        // bypassing the app-level init'd instance. Default returns NOT_CONFIGURED;
        // override in providers that support per-call DSN (e.g. via HTTP API).
        virtual void sendFeedbackDirect(
            std::string dsn,
            std::string release,
            std::string environment,
            std::string text,
            std::function<void(const Error&)> callback
        );
};

class HATN_CLIENTAPP_EXPORT FeedbackProviderRegistry
{
    public:

        void registerProvider(std::shared_ptr<FeedbackProvider> provider);

        void selectProvider(ClientApp* clientApp,
                            const std::string& scheme,
                            const std::string& section);

        // Returns the provider registered under `scheme`, or nullptr if not registered.
        // Does not require the provider to be init'd; used for account-level sendFeedbackDirect.
        FeedbackProvider* findProvider(const std::string& scheme) const noexcept;

        FeedbackProvider* activeProvider() const noexcept
        {
            return m_active;
        }

        void reset() noexcept
        {
            m_active = nullptr;
        }

    private:

        std::map<std::string, std::shared_ptr<FeedbackProvider>> m_providers;
        FeedbackProvider* m_active = nullptr;
};

HATN_CLIENTAPP_NAMESPACE_END

#endif // HATNFEEDBACKPROVIDER_H
