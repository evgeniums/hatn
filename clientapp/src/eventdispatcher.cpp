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
    HATN_CTX_SCOPE_PUSH("event_category",event->category);
    HATN_CTX_SCOPE_PUSH("event",event->event);

    EventKey key{
        event->category,
        event->event
    };
    if (env)
    {
        key.setEnvId(env->name());
    }
    if (!event->topic.empty())
    {
        key.setTopic(event->topic);
        HATN_CTX_SCOPE_PUSH("event_topic",event->topic);
    }

    std::vector<EventHandler> handlers;
    {
        common::SharedLocker::SharedScope l{m_mutex};
        handlers=m_subscriptions->find(key);
    }

    if (handlers.empty())
    {
        HATN_CTX_DEBUG(1,"event subscribers not found")
    }

    for (auto&& handler : handlers)
    {
        if (handler)
        {
            HATN_CTX_DEBUG(1,"before event subscriber invoke")
            handler(env,ctx,event);
            HATN_CTX_DEBUG(1,"after event subscriber invoke")
        }
    }
}

//---------------------------------------------------------------

HATN_CLIENTAPP_NAMESPACE_END
