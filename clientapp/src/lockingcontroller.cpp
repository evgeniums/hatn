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
      m_pincodeTolerateTries(DefaultPincodeTolerateTries),
      m_pincodeResetErrorsPeriod(DefaultPincodeResetErrorPeriod),
      m_passphraseThrottlePeriod(DefaultPassphraseThrottlePeriod),
      m_passphraseThrottleDelay(DefaultPassphraseThrottleDelay),
      m_passphraseThrottleTries(DefaultPassphraseThrottleTolerateTries),
      m_passphraseCheckPeriod(DefaultPassphraseCheckPeriod),
      m_appSettingsSubscription(0),
      m_lockingSettingsSubscription(0)
{
    m_activityTimer.setAutoAsyncGuardEnabled(false);
}

//---------------------------------------------------------------

void LockingController::init()
{
    common::MutexScopedLock l(m_mutex);

    // load passphrase throttling settings from app config
    try
    {
        HATN_BASE_NAMESPACE::ConfigTreePath passphraseThrottleSection{SettingsPassphraseThrottleSection};
        auto phThrottlePeriodKey=passphraseThrottleSection.copyAppend("period");
        if (m_app->app().configTree().isSet(phThrottlePeriodKey,true))
        {
            m_passphraseThrottlePeriod=m_app->app().configTree().getEx(phThrottlePeriodKey).asEx<uint32_t>();
        }
        auto phThrottleDelayKey=passphraseThrottleSection.copyAppend("delay");
        if (m_app->app().configTree().isSet(phThrottleDelayKey,true))
        {
            m_passphraseThrottleDelay=m_app->app().configTree().getEx(phThrottleDelayKey).asEx<uint32_t>();
        }
        auto phThrottleTolerateTriesKey=passphraseThrottleSection.copyAppend("tolerate_tries");
        if (m_app->app().configTree().isSet(phThrottleTolerateTriesKey,true))
        {
            m_passphraseThrottleTries=m_app->app().configTree().getEx(phThrottleTolerateTriesKey).asEx<uint32_t>();
        }
    }
    catch(...)
    {
    }
}

//---------------------------------------------------------------

void LockingController::start()
{
    // set default settings

    HATN_BASE_NAMESPACE::ConfigTreePath lockingSection{SettingsLockingSection};
    HATN_BASE_NAMESPACE::ConfigTreePath passphraseSection{SettingsPassphraseSection};    

    m_app->appSettings()->lock();

    m_app->appSettings()->configTree().setDefaultEx(lockingSection.copyAppend("period"),DefaultAutoLockPeriod);
    m_app->appSettings()->configTree().setDefaultEx(lockingSection.copyAppend("mode"),static_cast<int>(AutoLockMode::Disabled));
    m_app->appSettings()->configTree().setDefaultEx(lockingSection.copyAppend("tolerate_tries"),DefaultPincodeTolerateTries);
    m_app->appSettings()->configTree().setDefaultEx(lockingSection.copyAppend("reset_error_period"),DefaultPincodeResetErrorPeriod);

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
            updatePassphraseSettings();
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
        uint32_t period,
        uint32_t tolerateTries,
        uint32_t resetErrorPeriod
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
            auto r3=m_app->appSettings()->configTree().set(lockingSection.copyAppend("tolerate_tries"),tolerateTries);
            if (r3)
            {
                return;
            }
            auto r4=m_app->appSettings()->configTree().set(lockingSection.copyAppend("reset_error_period"),resetErrorPeriod);
            if (r4)
            {
                return;
            }
        }
    }
    m_app->appSettings()->flush(std::move(ctx),std::move(callback),SettingsLockingSection);
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
    uint32_t tolerateTries;
    uint32_t resetErrorPeriod;

    {
        common::MutexScopedLock l(m_app->appSettings()->mutex());

        try
        {
            period=m_app->appSettings()->configTree().getEx(lockingSection.copyAppend("period")).asEx<uint32_t>();
            int modeInt=m_app->appSettings()->configTree().getEx(lockingSection.copyAppend("mode")).asEx<int>();
            mode=static_cast<AutoLockMode>(modeInt);
            tolerateTries=m_app->appSettings()->configTree().getEx(lockingSection.copyAppend("tolerate_tries")).asEx<uint32_t>();
            resetErrorPeriod=m_app->appSettings()->configTree().getEx(lockingSection.copyAppend("reset_error_period")).asEx<uint32_t>();
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
        m_pincodeTolerateTries=tolerateTries;
        m_pincodeResetErrorsPeriod=resetErrorPeriod;
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
