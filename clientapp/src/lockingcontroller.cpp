/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientapp/lockingcontroller.сpp
  *
  */

#include <hatn/logcontext/context.h>

#include <hatn/app/app.h>

#include <hatn/clientapp/clientapp.h>
#include <hatn/clientapp/clientappsettings.h>
#include <hatn/clientapp/lockingcontroller.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

//---------------------------------------------------------------

LockingController::LockingController(ClientApp* app)
    : m_app(app),
      m_lastActivity(common::DateTime::currentUtc()),
      m_locked(false),
      m_background(false),
      m_activityTimer(app->app().appThread()),
      m_autoLockPeriod(DefaultAutoLockPeriod),
      m_autoLockMode(AutoLockMode::Disabled),
      m_passphraseThrottlePeriod(DefaultPassphraseThrottlePeriod),
      m_passphraseThrottleDelay(DefaultPassphraseThrottleDelay),
      m_passphraseThrottleTries(DefaultPassphraseThrottleTolerateTries),
      m_passphraseCheckPeriod(DefaultPassphraseCheckPeriod),
      m_appSettingsSubscription(0),
      m_lockingSettingsSubscription(0),
      m_passhraseThrottleSettingsSubscription(0),
      m_passhraseSettingsSubscription(0)
{
    m_activityTimer.setAutoAsyncGuardEnabled(false);
}

//---------------------------------------------------------------

void LockingController::start()
{
    // set default settings

    HATN_BASE_NAMESPACE::ConfigTreePath lockingSection{SettingsLockingSection};
    HATN_BASE_NAMESPACE::ConfigTreePath passphraseSection{SettingsPassphraseSection};
    HATN_BASE_NAMESPACE::ConfigTreePath passphraseThrottleSection{SettingsPassphraseThrottleSection};

    m_app->appSettings()->lock();

    m_app->appSettings()->configTree().setDefaultEx(lockingSection.copyAppend("period"),DefaultAutoLockPeriod);
    m_app->appSettings()->configTree().setDefaultEx(lockingSection.copyAppend("mode"),static_cast<int>(AutoLockMode::Disabled));

    m_app->appSettings()->configTree().setDefaultEx(passphraseThrottleSection.copyAppend("period"),DefaultPassphraseThrottlePeriod);
    m_app->appSettings()->configTree().setDefaultEx(passphraseThrottleSection.copyAppend("delay"),DefaultPassphraseThrottleDelay);
    m_app->appSettings()->configTree().setDefaultEx(passphraseThrottleSection.copyAppend("tolerate_tries"),DefaultPassphraseThrottleTolerateTries);

    m_app->appSettings()->configTree().setDefaultEx(passphraseSection.copyAppend("check_period"),DefaultPassphraseCheckPeriod);

    m_app->appSettings()->unlock();

    // start activity timer
    auto activityHandler=[self=shared_from_this(),this](common::TimerTypes::Status status)
    {
        if (status==common::TimerTypes::Status::Cancel)
        {
            return;
        }

        checkLockedByLastActivity();
    };
    m_activityTimer.setPeriodUs(ActivityTimerPeriod * 1000 * 1000);
    m_activityTimer.start(std::move(activityHandler));

    // subscribe to app settings events
    m_appSettingsSubscription=m_app->eventDispatcher().subscribe(
        [self=shared_from_this(),this](auto,auto,auto)
        {
            updateLockingSettings();
            updatePassphraseThrottleSettings();
        },
        EventKey{AppSettingsEvent::Category}
    );
    m_lockingSettingsSubscription=m_app->eventDispatcher().subscribe(
        [self=shared_from_this(),this](auto,auto,auto)
        {
            updateLockingSettings();
        },
        EventKey{AppSettingsEvent::Category,SettingsLockingSection}
    );
    m_passhraseThrottleSettingsSubscription=m_app->eventDispatcher().subscribe(
        [self=shared_from_this(),this](auto,auto,auto)
        {
            updatePassphraseThrottleSettings();
        },
        EventKey{AppSettingsEvent::Category,SettingsPassphraseThrottleSection}
    );
    m_passhraseSettingsSubscription=m_app->eventDispatcher().subscribe(
        [self=shared_from_this(),this](auto,auto,auto)
        {
            updatePassphraseSettings();
        },
        EventKey{AppSettingsEvent::Category,SettingsPassphraseSection}
    );
}

//---------------------------------------------------------------

void LockingController::close()
{
    m_activityTimer.stop();

    m_app->eventDispatcher().unsubscribe(m_appSettingsSubscription);
    m_app->eventDispatcher().unsubscribe(m_lockingSettingsSubscription);
    m_app->eventDispatcher().unsubscribe(m_passhraseThrottleSettingsSubscription);
    m_app->eventDispatcher().unsubscribe(m_passhraseSettingsSubscription);
}

//---------------------------------------------------------------

bool LockingController::checkLockedByLastActivity()
{
    if (isLocked())
    {
        return true;
    }

    if (autoLockMode()==AutoLockMode::WhenInactive)
    {
        auto now=common::DateTime::currentUtc();
        m_mutex.lock();
        auto expire=m_lastActivity;
        m_mutex.unlock();
        expire.addSeconds(autoLockPeriod());
        if (now.after(expire))
        {
            lock();
            return true;
        }
    }

    return false;
}

//---------------------------------------------------------------

void LockingController::lock()
{
    m_mutex.lock();
    m_locked=true;
    m_mutex.unlock();

    m_app->eventDispatcher().publish(
        m_app->app().env(),
        HATN_LOGCONTEXT_NAMESPACE::makeLogCtx(),
        std::make_shared<LockEvent>()
    );
}

//---------------------------------------------------------------

void LockingController::unlock()
{
    m_mutex.lock();
    m_locked=false;
    m_mutex.unlock();

    m_app->eventDispatcher().publish(
        m_app->app().env(),
        HATN_LOGCONTEXT_NAMESPACE::makeLogCtx(),
        std::make_shared<UnlockEvent>()
    );
}

//---------------------------------------------------------------

void LockingController::setBackground()
{
    m_mutex.lock();
    m_background=true;
    m_mutex.unlock();

    if (autoLockMode()==AutoLockMode::Background)
    {
        lock();
    }
}

//---------------------------------------------------------------

void LockingController::setForeground()
{
    m_mutex.lock();
    m_background=false;
    m_mutex.unlock();

    updateLastActivity();
}

//---------------------------------------------------------------

void LockingController::updateLastActivity()
{
    if (isLocked())
    {
        return;
    }

    m_mutex.lock();
    m_lastActivity.loadCurrentUtc();
    m_mutex.unlock();

    auto lastActivityEvent=std::make_shared<LastActivityEvent>();
    //! @todo Load last activity time to event's message

    m_app->eventDispatcher().publish(
        m_app->app().env(),
        HATN_LOGCONTEXT_NAMESPACE::makeLogCtx(),
        std::move(lastActivityEvent)
    );
}

//---------------------------------------------------------------

void LockingController::setAutoLockSettings(
        common::SharedPtr<ClientAppSettings::Context> ctx,
        ClientAppSettings::Callback callback,
        AutoLockMode mode,
        uint32_t period
    )
{
    {
        common::MutexScopedLock l{m_app->appSettings()->mutex()};
        HATN_BASE_NAMESPACE::ConfigTreePath lockingSection{SettingsLockingSection};
        auto r1=m_app->appSettings()->configTree().set(lockingSection.copyAppend("period"),period);
        if (!r1)
        {
            auto r2=m_app->appSettings()->configTree().set(lockingSection.copyAppend("mode"),static_cast<int>(mode));
            if (r2)
            {
                return;
            }
        }
    }
    m_app->appSettings()->flush(std::move(ctx),std::move(callback),SettingsLockingSection);
}

//---------------------------------------------------------------

void LockingController::setPassphraseThrottleSettings(
        common::SharedPtr<ClientAppSettings::Context> ctx,
        ClientAppSettings::Callback callback,
        uint32_t period,
        uint32_t delay,
        uint32_t tolerateTries
    )
{
    {
        common::MutexScopedLock l{m_app->appSettings()->mutex()};
        HATN_BASE_NAMESPACE::ConfigTreePath passphraseSection{SettingsPassphraseThrottleSection};
        auto r1=m_app->appSettings()->configTree().set(passphraseSection.copyAppend("period"),period);
        if (!r1)
        {
            auto r2=m_app->appSettings()->configTree().set(passphraseSection.copyAppend("delay"),delay);
            if (r2)
            {
                return;
            }
            auto r3=m_app->appSettings()->configTree().set(passphraseSection.copyAppend("tolerate_tries"),tolerateTries);
            if (r3)
            {
                return;
            }
        }
    }
    m_app->appSettings()->flush(std::move(ctx),std::move(callback),SettingsPassphraseThrottleSection);
}

//---------------------------------------------------------------

void LockingController::setPassphraseCheckPeriod(
    common::SharedPtr<ClientAppSettings::Context> ctx,
    ClientAppSettings::Callback callback,
    uint32_t period
    )
{
    {
        common::MutexScopedLock l{m_app->appSettings()->mutex()};
        HATN_BASE_NAMESPACE::ConfigTreePath passphraseSection{SettingsPassphraseSection};
        auto r1=m_app->appSettings()->configTree().set(passphraseSection.copyAppend("check_period"),period);
        if (r1)
        {
            return;
        }
    }
    m_app->appSettings()->flush(std::move(ctx),std::move(callback),SettingsPassphraseSection);
}

//---------------------------------------------------------------

void LockingController::updateLockingSettings()
{
    HATN_BASE_NAMESPACE::ConfigTreePath lockingSection{SettingsLockingSection};
    AutoLockMode mode;
    uint32_t period;

    {
        common::MutexScopedLock l(m_app->appSettings()->mutex());

        try
        {
            period=m_app->appSettings()->configTree().getEx(lockingSection.copyAppend("period")).asEx<uint32_t>();
            int modeInt=m_app->appSettings()->configTree().getEx(lockingSection.copyAppend("mode")).asEx<int>();
            mode=static_cast<AutoLockMode>(modeInt);
        }
        catch (...)
        {
            return;
        }
    }

    {
        common::MutexScopedLock l(m_mutex);
        m_autoLockPeriod=period;
        m_autoLockMode=mode;
    }
}

//---------------------------------------------------------------

void LockingController::updatePassphraseThrottleSettings()
{
    HATN_BASE_NAMESPACE::ConfigTreePath passphraseSection{SettingsPassphraseThrottleSection};
    uint32_t period;
    uint32_t delay;
    uint32_t tolerateTries;

    {
        common::MutexScopedLock l(m_app->appSettings()->mutex());

        try
        {
            period=m_app->appSettings()->configTree().getEx(passphraseSection.copyAppend("period")).asEx<uint32_t>();
            delay=m_app->appSettings()->configTree().getEx(passphraseSection.copyAppend("delay")).asEx<uint32_t>();
            tolerateTries=m_app->appSettings()->configTree().getEx(passphraseSection.copyAppend("tolerate_tries")).asEx<uint32_t>();
        }
        catch (...)
        {
            return;
        }
    }

    {
        common::MutexScopedLock l(m_mutex);
        m_passphraseThrottlePeriod=period;
        m_passphraseThrottleDelay=delay;
        m_passphraseThrottleTries=tolerateTries;
    }
}

//---------------------------------------------------------------

void LockingController::updatePassphraseSettings()
{
    HATN_BASE_NAMESPACE::ConfigTreePath passphraseSection{SettingsPassphraseSection};
    uint32_t period;

    {
        common::MutexScopedLock l(m_app->appSettings()->mutex());

        try
        {
            period=m_app->appSettings()->configTree().getEx(passphraseSection.copyAppend("check_period")).asEx<uint32_t>();
        }
        catch (...)
        {
            return;
        }
    }

    {
        common::MutexScopedLock l(m_mutex);
        m_passphraseCheckPeriod=period;
    }
}

//---------------------------------------------------------------

HATN_CLIENTAPP_NAMESPACE_END
