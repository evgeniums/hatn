/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientapp/crashreporter.h
  *
  * Abstract crash reporter and its registry.
  *
  * A crash reporter captures native crashes and uploads them to an external
  * destination (e.g. Sentry with crashpad on desktop, inproc on mobile).
  * init() is called once at app startup; close() must be called before the
  * process exits to flush pending envelopes.
  *
  * On desktop release builds only: the concrete provider calls sentry_init /
  * sentry_close. On mobile the platform SDK handles upload; the C layer only
  * initialises the in-process handler.
  */

/****************************************************************************/

#ifndef HATNCRASHEDREPORTER_H
#define HATNCRASHEDREPORTER_H

#include <string>
#include <map>
#include <memory>

#include <hatn/common/error.h>

#include <hatn/clientapp/clientappdefs.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

class ClientApp;

class HATN_CLIENTAPP_EXPORT CrashReporter
{
    public:

        virtual ~CrashReporter();

        virtual const char* name() const noexcept = 0;

        virtual Error init(ClientApp* clientApp, const std::string& section) = 0;

        virtual void setUser(std::string userId, std::string userName) = 0;

        virtual void clearUser() = 0;

        virtual void close() = 0;
};

class HATN_CLIENTAPP_EXPORT CrashReporterRegistry
{
    public:

        void registerProvider(std::shared_ptr<CrashReporter> reporter);

        void selectProvider(ClientApp* clientApp,
                            const std::string& scheme,
                            const std::string& section);

        CrashReporter* activeProvider() const noexcept
        {
            return m_active;
        }

        void reset() noexcept
        {
            m_active = nullptr;
        }

    private:

        std::map<std::string, std::shared_ptr<CrashReporter>> m_reporters;
        CrashReporter* m_active = nullptr;
};

HATN_CLIENTAPP_NAMESPACE_END

#endif // HATNCRASHEDREPORTER_H
