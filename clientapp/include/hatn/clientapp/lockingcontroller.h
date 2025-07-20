/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientapp/lockingcontroller.h
  *
  */

/****************************************************************************/

#ifndef HATNCLIENTLOCKINGCONTROLLER_H
#define HATNCLIENTLOCKINGCONTROLLER_H

#include <hatn/common/locker.h>
#include <hatn/common/datetime.h>
#include <hatn/common/asiotimer.h>

#include <hatn/clientapp/clientappdefs.h>
#include <hatn/clientapp/eventdispatcher.h>
#include <hatn/clientapp/clientappsettings.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

class ClientApp;

enum class AutoLockMode : int
{
    Disabled,
    Background,
    WhenInactive
};

class HATN_CLIENTAPP_EXPORT LockingController : public std::enable_shared_from_this<LockingController>
{
    public:

        constexpr static const char* EventCategory="locking";

        constexpr static const char* SettingsLockingSection="clientapp.locking";
        constexpr static const char* SettingsPassphraseThrottleSection="clientapp.passphrase_throttle";

        constexpr static const uint32_t ActivityTimerPeriod=1;

        constexpr static const uint32_t DefaultAutoLockPeriod=60;

        constexpr static const uint32_t DefaultPassphraseThrottlePeriod=60;
        constexpr static const uint32_t DefaultPassphraseThrottleDelay=5;

        LockingController(ClientApp* app);

        void start();
        void close();

        void lock();

        void unlock();

        void updateLastActivity();

        void setBackground();
        void setForeground();

        bool isLocked() const
        {
            common::MutexScopedLock l(m_mutex);
            return m_locked;
        }

        bool isBackground() const
        {
            common::MutexScopedLock l(m_mutex);
            return m_background;
        }

        uint32_t autoLockPeriod() const noexcept
        {
            common::MutexScopedLock l(m_mutex);
            return m_autoLockPeriod;
        }

        AutoLockMode autoLockMode() const noexcept
        {
            common::MutexScopedLock l(m_mutex);
            return m_autoLockMode;
        }

        uint32_t passphraseThrottlePeriod() const
        {
            common::MutexScopedLock l(m_mutex);
            return m_passphraseThrottlePeriod;
        }

        uint32_t passphraseThrottleDelay() const
        {
            common::MutexScopedLock l(m_mutex);
            return m_passphraseThrottleDelay;
        }

        void setAutoLockSettings(
            common::SharedPtr<ClientAppSettings::Context> ctx,
            ClientAppSettings::Callback callback,
            AutoLockMode mode,
            uint32_t period=DefaultAutoLockPeriod
        );

        void setPassphraseThrottleSettings(
            common::SharedPtr<ClientAppSettings::Context> ctx,
            ClientAppSettings::Callback callback,
            uint32_t period,
            uint32_t delay
        );

        void setLocked(bool enable)
        {
            if (enable)
            {
                lock();
            }
            else
            {
                unlock();
            }
        }

        bool checkLockedByLastActivity();

    private:

        ClientApp* m_app;
        common::DateTime m_lastActivity;
        bool m_locked;
        bool m_background;

        common::AsioDeadlineTimer m_activityTimer;

        uint32_t m_autoLockPeriod;
        AutoLockMode m_autoLockMode;

        uint32_t m_passphraseThrottlePeriod;
        uint32_t m_passphraseThrottleDelay;

        size_t m_appSettingsSubscription;
        size_t m_lockingSettingsSubscription;
        size_t m_passhraseThrottleSettingsSubscription;

        mutable common::MutexLock m_mutex;

        void updateLockingSettings();
        void updatePassphraseThrottleSettings();
};

class LockEvent : public Event
{
    public:

        constexpr static const char* Name="lock";

        LockEvent()
        {
            category=LockingController::EventCategory;
            event=Name;
        }
};

class UnlockEvent : public Event
{
    public:

        constexpr static const char* Name="unlock";

        UnlockEvent()
        {
            category=LockingController::EventCategory;
            event=Name;
        }
};

class LastActivityEvent : public Event
{
    public:

        constexpr static const char* Name="last_activity";

        LastActivityEvent()
        {
            category=LockingController::EventCategory;
            event=Name;
        }
};

HATN_CLIENTAPP_NAMESPACE_END

#endif // HATNCLIENTAPPSETTINGS_H
