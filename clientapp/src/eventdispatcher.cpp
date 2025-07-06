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

size_t EventSubscriptions::doRemove(size_t index, EventSubscriptions* subscriptions)
{
    size_t removedCount=subscriptions->handlers.erase(index);
    if (removedCount!=0)
    {
        return removedCount;
    }

    for (auto& it: subscriptions->keyHandlers)
    {
        size_t removedCount=doRemove(index,it.second.get());
        if (removedCount!=0)
        {
            if (it.second->handlers.empty() && it.second->keyHandlers.empty())
            {
                subscriptions->keyHandlers.erase(it.first);
            }
            return removedCount;
        }
    }

    return 0;
}

size_t EventSubscriptions::doInsert(EventKey key, EventHandler handler, size_t selectorIndex)
{
    HATN_CTX_SCOPE("eventsubcriptions::doinsert")

    HATN_CTX_DEBUG_RECORDS(1,"",{"selector_index",selectorIndex},{"keyselectorssize",key.selectors().size()},{"keyissubnull",key.isSubNull(selectorIndex)},
                           {"selector0",*key.selectors().at(0)},
                          {"selector1",*key.selectors().at(1)}
    )

    if (selectorIndex==key.selectors().size() || key.isSubNull(selectorIndex))
    {
        HATN_CTX_DEBUG_RECORDS(1,"handlers inserted")
        auto idx=Index++;
        handlers.emplace(idx,std::move(handler));
        return idx;
    }

    const auto* selector=key.selectors().at(selectorIndex);
    auto it=keyHandlers.find(*selector);
    if (it!=keyHandlers.end())
    {
        return it->second->doInsert(std::move(key),std::move(handler),selectorIndex+1);
    }

    auto subs=std::make_shared<EventSubscriptions>();
    auto subsPtr=subs.get();
    keyHandlers.emplace(std::move(*selector),std::move(subs));

    return subsPtr->doInsert(std::move(key),std::move(handler),selectorIndex+1);
}

//---------------------------------------------------------------

void EventSubscriptions::doFind(const EventKey& key,std::vector<EventHandler>& result, size_t selectorIndex) const
{
    HATN_CTX_SCOPE("eventsubcriptions::dofind")

    HATN_CTX_DEBUG_RECORDS(1,"",{"selector_index",selectorIndex},{"handlersempty",handlers.empty()})

    if (!handlers.empty())
    {
        for (const auto& handler:handlers)
        {
            result.push_back(handler.second);
        }
    }

    if (selectorIndex==key.selectors().size())
    {
        return;
    }

    const auto* selector=key.selectors().at(selectorIndex);
    auto it=keyHandlers.find(*selector);
    if (it!=keyHandlers.end())
    {
        it->second->doFind(key,result,selectorIndex+1);
    }
}

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

size_t EventDispatcher::subscribe(
    EventHandler handler,
    EventKey key
    )
{
    HATN_CTX_SCOPE("eventdispatcher::subscribe")
    HATN_CTX_SCOPE_PUSH("event_category",key.category());
    HATN_CTX_SCOPE_PUSH("event",key.event());

    size_t id=0;
    {
        common::SharedLocker::ExclusiveScope l{m_mutex};
        id=m_subscriptions->insert(std::move(key),std::move(handler));
    }
    HATN_CTX_SCOPE_PUSH("subscription_id",id);
    return id;
}

//---------------------------------------------------------------

void EventDispatcher::unsubscribe(
    size_t id
    )
{
    HATN_CTX_SCOPE("eventdispatcher::unsubscribe")
    HATN_CTX_SCOPE_PUSH("subscription_id",id);

    {
        common::SharedLocker::ExclusiveScope l{m_mutex};
        m_subscriptions->remove(id);
    }
}

//---------------------------------------------------------------

HATN_CLIENTAPP_NAMESPACE_END
