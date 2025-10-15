/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientapp/clientappsettings.h
  *
  * Application settings that can be reqritten by app in run time.
  */

/****************************************************************************/

#ifndef HATNCLIENTAPPSETTINGS_H
#define HATNCLIENTAPPSETTINGS_H

#include <hatn/common/taskcontext.h>
#include <hatn/common/locker.h>

#include <hatn/base/configtree.h>

#include <hatn/clientapp/clientappdefs.h>
#include <hatn/clientapp/eventdispatcher.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

class ClientApp;

struct AppSettingsEvent : public Event
{
    public:

        constexpr static const char* Category="appsettings";

        AppSettingsEvent()
        {
            category=Category;
        }
};

class HATN_CLIENTAPP_EXPORT ClientAppSettings : public std::enable_shared_from_this<ClientAppSettings>
{
    public:

        constexpr static const char* DbObjectName="appsettings";
        constexpr static const char* AppSection="app";

        using Context=common::TaskContext;
        using Callback=std::function<void (common::SharedPtr<Context>, const Error&)>;

        ClientAppSettings(ClientApp* app);

        virtual ~ClientAppSettings()=default;
        ClientAppSettings(const ClientAppSettings&)=delete;
        ClientAppSettings(ClientAppSettings&&)=delete;
        ClientAppSettings& operator=(const ClientAppSettings&)=delete;
        ClientAppSettings& operator=(ClientAppSettings&&)=delete;

        const HATN_BASE_NAMESPACE::ConfigTree& configTree() const
        {
            return m_configTree;
        }

        HATN_BASE_NAMESPACE::ConfigTree& configTree()
        {
            return m_configTree;
        }

        Error load();

        virtual void flush(
            common::SharedPtr<Context> ctx,
            Callback callback,
            std::string section=""
        );

        void lock()
        {
            m_mutex.lock();
        }

        void unlock()
        {
            m_mutex.unlock();
        }

        common::MutexLock& mutex()
        {
            return m_mutex;
        }

    private:

        ClientApp* m_app;
        HATN_BASE_NAMESPACE::ConfigTree m_configTree;
        common::MutexLock m_mutex;
};

HATN_CLIENTAPP_NAMESPACE_END

#endif // HATNCLIENTAPPSETTINGS_H
