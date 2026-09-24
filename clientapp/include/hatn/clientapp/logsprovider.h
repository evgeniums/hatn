/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientapp/logsprovider.h
  *
  * Abstract logs provider and its registry.
  *
  * A logs provider delivers application log files to an external destination
  * (e.g. Sentry attachments). It receives a list of filesystem paths — the
  * live client core log first, followed by any additional paths supplied by
  * the caller (e.g. platform-specific iOS/Android logs). The provider reads,
  * packages, and uploads each file itself. If no provider is configured bridge
  * callers must return ClientAppError::LOGS_PROVIDER_NOT_CONFIGURED.
  */

/****************************************************************************/

#ifndef HATNLOGSPROVIDER_H
#define HATNLOGSPROVIDER_H

#include <string>
#include <functional>
#include <vector>
#include <map>
#include <memory>

#include <hatn/common/error.h>
#include <hatn/common/taskcontext.h>

#include <hatn/clientapp/clientappdefs.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

class ClientApp;

class HATN_CLIENTAPP_EXPORT LogsProvider
{
    public:

        virtual ~LogsProvider();

        virtual const char* name() const noexcept = 0;

        virtual Error init(ClientApp* clientApp, const std::string& section) = 0;

        virtual void sendLogs(
            common::SharedPtr<common::TaskContext> ctx,
            std::string comments,
            std::vector<std::string> filePaths,
            std::function<void(const Error&)> callback
        ) = 0;
};

class HATN_CLIENTAPP_EXPORT LogsProviderRegistry
{
    public:

        void registerProvider(std::shared_ptr<LogsProvider> provider);

        void selectProvider(ClientApp* clientApp,
                            const std::string& scheme,
                            const std::string& section);

        LogsProvider* activeProvider() const noexcept
        {
            return m_active;
        }

        void reset() noexcept
        {
            m_active = nullptr;
        }

    private:

        std::map<std::string, std::shared_ptr<LogsProvider>> m_providers;
        LogsProvider* m_active = nullptr;
};

HATN_CLIENTAPP_NAMESPACE_END

#endif // HATNLOGSPROVIDER_H
