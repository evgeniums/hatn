/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientapp/eventdispatcher.h
  *
  */

/****************************************************************************/

#ifndef HATNCLIENTEVENTDISPATCHER_H
#define HATNCLIENTEVENTDISPATCHER_H

#include <memory>
#include <map>
#include <functional>

#include <hatn/common/sharedlocker.h>

#include <hatn/dataunit/unitwrapper.h>

#include <hatn/clientapp/clientappdefs.h>
#include <hatn/clientapp/clientapp.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

class EventKey
{
    public:

        EventKey(
                std::string category={},
                std::string event={},
                std::string envId={},
                std::string topic={}
            ) : m_category(std::move(category)),
                m_event(std::move(event)),
                m_envId(std::move(envId)),
                m_topic(std::move(topic))
        {
            m_selectors[0]=&m_category;
            m_selectors[1]=&m_event;
            m_selectors[2]=&m_envId;
            m_selectors[3]=&m_topic;
        }

        bool isNull() const noexcept
        {
            for (const auto& selector : m_selectors)
            {
                if (!selector->empty())
                {
                    return false;
                }
            }
            return true;
        }

        bool isSubNull(size_t index) const noexcept
        {
            for (size_t i=index;i<m_selectors.size();i++)
            {
                if (!m_selectors[i]->empty())
                {
                    return false;
                }
            }
            return true;
        }

        const std::string& category() const noexcept
        {
            return m_category;
        }

        const std::string& event() const noexcept
        {
            return m_event;
        }

        const std::string& envId() const noexcept
        {
            return m_envId;
        }

        const std::string& topic() const noexcept
        {
            return m_topic;
        }

        void setcategory(std::string category)
        {
            m_category=std::move(category);
        }

        void setevent(std::string event)
        {
            m_event=std::move(event);
        }

        void setEnvId(std::string envId)
        {
            m_envId=std::move(envId);
        }

        void setTopic(std::string topic)
        {
            m_topic=std::move(topic);
        }

        const std::array<const std::string*,4>& selectors() const
        {
            return m_selectors;
        }


    private:

        std::string m_category;
        std::string m_event;
        std::string m_envId;
        std::string m_topic;

        std::array<const std::string*,4> m_selectors;
};

struct Event
{
    std::string category;
    std::string event;

    std::string topic;
    std::string messageTypeName;
    du::UnitWrapper message;
    std::vector<common::ByteArrayShared> buffers;
};

using EventHandler=std::function<void (common::SharedPtr<app::AppEnv> env,
                                              common::SharedPtr<Context> ctx,
                                              std::shared_ptr<Event> event)>;

class HATN_CLIENTAPP_EXPORT EventSubscriptions
{
    public:

        std::vector<EventHandler> find(const EventKey& key) const
        {
            std::vector<EventHandler> result;
            doFind(key,result);
            return result;
        }

        size_t insert(EventKey key, EventHandler handler)
        {
            return doInsert(std::move(key),std::move(handler));
        }

        void remove(size_t index)
        {
            doRemove(index,this);
        }

    private:

        static size_t Index;

        std::map<size_t,EventHandler> handlers;
        std::map<std::string,std::shared_ptr<EventSubscriptions>> keyHandlers;

        size_t doRemove(size_t index, EventSubscriptions* subscriptions)
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

        size_t doInsert(EventKey key, EventHandler handler, size_t selectorIndex=0)
        {
            if (selectorIndex==key.selectors().size() || key.isSubNull(selectorIndex))
            {
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

        void doFind(const EventKey& key,std::vector<EventHandler>& result, size_t selectorIndex=0) const
        {
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
};

class HATN_CLIENTAPP_EXPORT EventDispatcher
{
    public:

        EventDispatcher();

        void publish(
            common::SharedPtr<app::AppEnv> env,
            common::SharedPtr<Context> ctx,
            std::shared_ptr<Event> event
        );

        size_t subscribe(
            EventHandler handler,
            EventKey key={}
        )
        {
            common::SharedLocker::ExclusiveScope l{m_mutex};
            return m_subscriptions->insert(std::move(key),std::move(handler));
        }

        void unsubscribe(
            size_t id
        )
        {
            common::SharedLocker::ExclusiveScope l{m_mutex};
            m_subscriptions->remove(id);
        }

    private:

        std::shared_ptr<EventSubscriptions> m_subscriptions;
        common::SharedLocker m_mutex;
};

HATN_CLIENTAPP_NAMESPACE_END

#endif // HATNCLIENTEVENTDISPATCHER_H
