/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientapp/eventdispatcher.сpp
  *
  */

#include <hatn/clientapp/eventdispatcher.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

//---------------------------------------------------------------

size_t EventSubscriptions::Index=0;

EventDispatcher::EventDispatcher() : m_subscriptions(std::make_shared<EventSubscriptions>())
{}

//---------------------------------------------------------------

void EventDispatcher::publish(
        common::SharedPtr<app::AppEnv> env,
        common::SharedPtr<Context> ctx,
        std::shared_ptr<Event> event
    )
{
    HATN_CTX_SCOPE("eventdispatcher::publish")
    HATN_CTX_PUSH_VAR("category",event->category);
    HATN_CTX_PUSH_VAR("event",event->event);

    EventKey key{
        event->category,
        event->event
    };
    if (env)
    {
        key.setEnvId(env->name());
    }
    key.setTopic(event->topic);
    HATN_CTX_PUSH_VAR("event_topic",event->topic);

    std::vector<EventHandler> handlers;
    {
        common::SharedLocker::SharedScope l{m_mutex};
        handlers=m_subscriptions->find(key);
    }

    for (auto&& handler : handlers)
    {
        if (handler)
        {
            handler(env,ctx,event);
        }
    }
}

//---------------------------------------------------------------

HATN_CLIENTAPP_NAMESPACE_END
