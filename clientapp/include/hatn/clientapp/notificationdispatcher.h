/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientapp/notificationdispatcher.h
  *
  */

/****************************************************************************/

#ifndef HATNCLIENTNOTIFICATIONDISPATCHER_H
#define HATNCLIENTNOTIFICATIONDISPATCHER_H

#include <memory>
#include <map>
#include <functional>

#include <hatn/common/sharedlocker.h>

#include <hatn/dataunit/unitwrapper.h>

#include <hatn/clientapp/clientappdefs.h>
#include <hatn/clientapp/clientapp.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

class NotificationKey
{
    public:

        NotificationKey(
                std::string service={},
                std::string method={},
                std::string envId={},
                std::string topic={}
            ) : m_service(std::move(service)),
                m_method(std::move(method)),
                m_envId(std::move(envId)),
                m_topic(std::move(topic))
        {
            m_selectors[0]=&m_service;
            m_selectors[1]=&m_method;
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

        const std::string& service() const noexcept
        {
            return m_service;
        }

        const std::string& method() const noexcept
        {
            return m_method;
        }

        const std::string& envId() const noexcept
        {
            return m_envId;
        }

        const std::string& topic() const noexcept
        {
            return m_topic;
        }

        void setService(std::string service)
        {
            m_service=std::move(service);
        }

        void setMethod(std::string method)
        {
            m_method=std::move(method);
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

        std::string m_service;
        std::string m_method;
        std::string m_envId;
        std::string m_topic;

        std::array<const std::string*,4> m_selectors;
};

struct Notification
{
    std::string topic;
    std::string messageTypeName;
    du::UnitWrapper message;
    std::vector<common::ByteArrayShared> buffers;
};

using NotificationHandler=std::function<void (common::SharedPtr<app::AppEnv> env,
                                              common::SharedPtr<Context> ctx,
                                              const std::string& service,
                                              const std::string& method,
                                              std::shared_ptr<Notification> notification)>;

class HATN_CLIENTAPP_EXPORT NotificationSubscriptions
{
    public:

        std::vector<NotificationHandler> find(const NotificationKey& key) const
        {
            std::vector<NotificationHandler> result;
            doFind(key,result);
            return result;
        }

        size_t insert(NotificationKey key, NotificationHandler handler)
        {
            return doInsert(std::move(key),std::move(handler));
        }

        void remove(size_t index)
        {
            doRemove(index,this);
        }

    private:

        static size_t Index;

        std::map<size_t,NotificationHandler> handlers;
        std::map<std::string,std::shared_ptr<NotificationSubscriptions>> keyHandlers;

        size_t doRemove(size_t index, NotificationSubscriptions* subscriptions)
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

        size_t doInsert(NotificationKey key, NotificationHandler handler, size_t selectorIndex=0)
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

            auto subs=std::make_shared<NotificationSubscriptions>();
            auto subsPtr=subs.get();
            keyHandlers.emplace(std::move(*selector),std::move(subs));

            return subsPtr->doInsert(std::move(key),std::move(handler),selectorIndex+1);
        }

        void doFind(const NotificationKey& key,std::vector<NotificationHandler>& result, size_t selectorIndex=0) const
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

class HATN_CLIENTAPP_EXPORT NotificationDispatcher
{
    public:

        NotificationDispatcher();

        void publish(
            common::SharedPtr<app::AppEnv> env,
            common::SharedPtr<Context> ctx,
            const std::string& service,
            const std::string& method,
            std::shared_ptr<Notification> notification
        );

        size_t subscribe(
            NotificationHandler handler,
            NotificationKey key={}
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

        std::shared_ptr<NotificationSubscriptions> m_subscriptions;
        common::SharedLocker m_mutex;
};

HATN_CLIENTAPP_NAMESPACE_END

#endif // HATNCLIENTNOTIFICATIONDISPATCHER_H
