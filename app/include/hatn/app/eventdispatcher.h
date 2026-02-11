/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file app/eventdispatcher.h
  *
  */

/****************************************************************************/

#ifndef HATNEVENTDISPATCHER_H
#define HATNEVENTDISPATCHER_H

#include <memory>
#include <functional>

#include <hatn/common/sharedlocker.h>
#include <hatn/dataunit/unitwrapper.h>
#include <hatn/app/appdefs.h>

HATN_APP_NAMESPACE_BEGIN

constexpr static const char* EventCreate="create";
constexpr static const char* EventUpdate="update";
constexpr static const char* EventRemove="remove";
constexpr static const char* EventRename="rename";
constexpr static const char* EventClose="close";
constexpr static const char* EventOpen="open";
constexpr static const char* EventMove="move";

class AppEnv;
using Context=common::TaskContext;

class EventKey
{
    public:

        constexpr static const size_t SelectorsCount=5;
        using Selectors=std::array<const std::string*,SelectorsCount>;

        EventKey(
                std::string category={},
                std::string event={},
                std::string envId={},
                std::string topic={},
                std::string oid={}
            ) : m_category(std::move(category)),
                m_event(std::move(event)),
                m_envId(std::move(envId)),
                m_topic(std::move(topic)),
                m_oid(std::move(oid))
        {
            initSelectors();
        }

        ~EventKey()=default;

        EventKey(EventKey&& other)
        {
            m_category=std::move(other.m_category);
            m_event=std::move(other.m_event);
            m_envId=std::move(other.m_envId);
            m_topic=std::move(other.m_topic);
            m_oid=std::move(other.m_oid);
            initSelectors();
        }

        EventKey(const EventKey& other)
        {
            m_category=other.m_category;
            m_event=other.m_event;
            m_envId=other.m_envId;
            m_topic=other.m_topic;
            m_oid=other.m_oid;
            initSelectors();
        }

        EventKey operator=(EventKey&& other)
        {
            if (&other==this)
            {
                return *this;
            }

            m_category=std::move(other.m_category);
            m_event=std::move(other.m_event);
            m_envId=std::move(other.m_envId);
            m_topic=std::move(other.m_topic);
            m_oid=std::move(other.m_oid);
            initSelectors();
            return *this;
        }

        EventKey& operator=(const EventKey& other)
        {
            if (&other==this)
            {
                return *this;
            }

            m_category=other.m_category;
            m_event=other.m_event;
            m_envId=other.m_envId;
            m_topic=other.m_topic;
            m_oid=other.m_oid;
            initSelectors();
            return *this;
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

        const std::string& oid() const noexcept
        {
            return m_oid;
        }

        void setCategory(std::string category)
        {
            m_category=std::move(category);
        }

        void setEvent(std::string event)
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

        void setOid(std::string oid)
        {
            m_oid=std::move(oid);
        }

        const Selectors& selectors() const
        {
            return m_selectors;
        }

    private:

        void initSelectors()
        {
            m_selectors[0]=&m_category;
            m_selectors[1]=&m_event;
            m_selectors[2]=&m_envId;
            m_selectors[3]=&m_topic;
            m_selectors[4]=&m_oid;
        }

        std::string m_category;
        std::string m_event;
        std::string m_envId;
        std::string m_topic;
        std::string m_oid;

        Selectors m_selectors;
};

struct Event
{
    std::string category;
    std::string event;

    std::string topic;
    std::string oid;
    std::string messageTypeName;
    std::string genericParameter;
    du::UnitWrapper message;
    std::vector<common::ByteArrayShared> buffers;
};

using EventHandler=std::function<void (common::SharedPtr<AppEnv> env,
                                              common::SharedPtr<Context> ctx,
                                              std::shared_ptr<Event> event)>;

class HATN_APP_EXPORT EventSubscriptions
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
        std::map<std::string,std::shared_ptr<EventSubscriptions>,std::less<>> keyHandlers;

        size_t doRemove(size_t index, EventSubscriptions* subscriptions);

        size_t doInsert(EventKey key, EventHandler handler, size_t selectorIndex=0);

        void doFind(const EventKey& key,std::vector<EventHandler>& result, size_t selectorIndex=0) const;
};

class HATN_APP_EXPORT EventDispatcher
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
        );

        void unsubscribe(
            size_t id
        );

    private:

        std::shared_ptr<EventSubscriptions> m_subscriptions;
        common::SharedLocker m_mutex;
};

HATN_APP_NAMESPACE_END

#endif // HATNEVENTDISPATCHER_H
