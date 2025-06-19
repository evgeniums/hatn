/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientapp/notificationdispatcher.сpp
  *
  */

#include <hatn/clientapp/notificationdispatcher.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

//---------------------------------------------------------------

size_t NotificationSubscriptions::Index=0;

NotificationDispatcher::NotificationDispatcher() : m_subscriptions(std::make_shared<NotificationSubscriptions>())
{}

//---------------------------------------------------------------

void NotificationDispatcher::publish(
        common::SharedPtr<app::AppEnv> env,
        common::SharedPtr<Context> ctx,
        const std::string& service,
        const std::string& method,
        std::shared_ptr<Notification> notification
    )
{
    HATN_CTX_SCOPE("notifications::publish")
    HATN_CTX_PUSH_VAR("notification_service",service);
    HATN_CTX_PUSH_VAR("notification_method",method);

    NotificationKey key{
        service,
        method
    };
    if (env)
    {
        key.setEnvId(env->name());
    }
    if (notification)
    {
        key.setTopic(notification->topic);
        HATN_CTX_PUSH_VAR("notification_topic",notification->topic);
    }

    std::vector<NotificationHandler> handlers;
    {
        common::SharedLocker::SharedScope l{m_mutex};
        handlers=m_subscriptions->find(key);
    }

    for (auto&& handler : handlers)
    {
        if (handler)
        {
            handler(env,ctx,service,method,notification);
        }
    }
}

//---------------------------------------------------------------

HATN_CLIENTAPP_NAMESPACE_END
