/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file mq/mqinmem.h
  *
  *
  */

/****************************************************************************/

#ifndef HATNMQINMEM_H
#define HATNMQINMEM_H

#include <functional>

#include <hatn/common/pmr/allocatorfactory.h>
#include <hatn/common/flatmap.h>
#include <hatn/common/cachelru.h>
#include <hatn/common/locker.h>
#include <hatn/common/withthread.h>

#include <hatn/mq/mqdef.h>
#include <hatn/mq/mqconfig.h>

HATN_MQ_NAMESPACE_BEGIN

template <typename MessageTraitsT, typename PosT=du::ObjectId>
class MqInmemTraits;

template <typename MessageTraitsT, typename PosT=du::ObjectId>
class MqInmem;

template <typename MessageTraitsT, typename PosT=du::ObjectId>
class MqInmemPool : public MqTraitsBase<MessageTraitsT,PosT>
{
    public:

        void openTopic(common::SharedPtr<TaskContext> ctx, lib::string_view topicId, TopicCb cb)
        {
            //! @todo wake up potential subscribers to subscribe
        }

        void closeTopic(common::SharedPtr<TaskContext> ctx, lib::string_view topicId, StatusCb cb)
        {
        }

        void findTopic(common::SharedPtr<TaskContext> ctx, lib::string_view topicId, TopicCb cb)
        {
        }

        Subscription subscribeToTopic(common::SharedPtr<TaskContext> ctx, lib::string_view topicId, SubscribeCb cb, StatusCb statusCb)
        {
        }

        void unsubscribeFromTopic(common::SharedPtr<TaskContext> ctx, lib::string_view topicId, Subscription& subscription, StatusCb statusCb)
        {
        }

        void reset()
        {
        }

    private:

        size_t m_topicsCapacity;
        uint32_t m_topicExpiration;

        size_t m_writeQueueCapacity;
        uint32_t m_writeQueueExpiration;

        size_t m_readContainerCapacity;
        size_t m_maxReadContainersCount;
        uint32_t m_readContainerExpiration;

        common::MutexLock m_mutex;

        common::CacheLruTtl<TopicId,std::shared_ptr<MqInmem<MessageTraitsT,PosT>>> m_topics;

        common::LruTtl<DeleterFn> m_topicReadMessageContainers;
        common::LruTtl<DeleterFn> m_topicReadQueueContainers;

        common::TaskWithContextThread* m_thread;

        template <typename MessageTraitsT1, typename PosT1=du::ObjectId>
        friend class MqInmemTraits;
};

template <typename MessageTraitsT, typename PosT=du::ObjectId>
class MqInmemTraits : public MqTraitsBase<MessageTraitsT,PosT>
{
    public:

        void create(common::SharedPtr<TaskContext> ctx, MessageType item, ItemCb cb, lib::optional<DownstreamPos> downstreamPos=lib::optional<DownstreamPos>{})
        {
        }

        void readFromId(common::SharedPtr<TaskContext> ctx, ItemCb cb, IdType from, IdType maxTo=IdType{}, size_t maxCount=1)
        {
        }

        void readToId(common::SharedPtr<TaskContext> ctx, ItemCb cb, IdType to, IdType minFrom=IdType{}, size_t maxCount=1)
        {
        }

        void findByRefId(common::SharedPtr<TaskContext> ctx, RefIdType refId, ItemCb cb)
        {
        }

        void update(common::SharedPtr<TaskContext> ctx, RefIdType refId, UpdateType data, ItemCb cb, lib::optional<DownstreamPos> downstreamPos=lib::optional<DownstreamPos>{})
        {
        }

        void remove(common::SharedPtr<TaskContext> ctx, RefIdType refId, RemoveCb cb, lib::optional<DownstreamPos> downstreamPos=lib::optional<DownstreamPos>{})
        {
        }

        void readFromPos(common::SharedPtr<TaskContext> ctx, ReadCb cb, PosType from, PosType maxTo=PosType{}, size_t maxCount=1)
        {
        }

        void readToPos(common::SharedPtr<TaskContext> ctx, ReadCb cb, PosType to, PosType minFrom=PosType{}, size_t maxCount=1)
        {
        }

        void getFirstPos(common::SharedPtr<TaskContext> ctx, PosCb cb)
        {
        }

        void getLastPos(common::SharedPtr<TaskContext> ctx, PosCb cb)
        {
        }

        void getLastDownstreamPos(common::SharedPtr<TaskContext> ctx, const DownstreamId& downstreamId, PosCb cb)
        {
        }

        void setLastDownstreamPos(common::SharedPtr<TaskContext> ctx, DownstreamPos downstreamPos)
        {
        }

        void removeDownstream(common::SharedPtr<TaskContext> ctx, const DownstreamId& downstreamId, StatusCb statusCb)
        {
        }

        Subscription subscribe(common::SharedPtr<TaskContext> ctx, SubscribeCb cb, StatusCb statusCb)
        {
        }

        void unsubscribe(common::SharedPtr<TaskContext> ctx, Subscription& subscription, StatusCb statusCb)
        {
        }

        const Item* next(const Item& item) const
        {
            return nullptr;
        }

        void reset()
        {
        }

    private:

        ItemQueueLruTtl<Item> m_writeQueue;

        PosType m_lastDownstreamPos;

        common::pmr::map<DownstreamId,PosType> m_lastDownstreamPos;

        PosType m_firstPos;
        PosType m_lastPos;

        common::pmr::map<IdInterval,ItemQueueInLruTtl<Item>,std::less<>> m_readMessages;
        common::pmr::map<PosInterval,ItemQueueInLruTtl<Item>,std::less<>> m_readQueues;

        common::pmr::map<Subscription::IdType,SubscribeCb> m_subscriptions;
        TopicId m_topic;

        MqInmemPool* m_pool;
        typename common::CacheLruTtl<TopicId,MqInmem<MessageTraitsT,PosT>>::Item *m_selfPoolItem;

        size_t m_inOpCount;
};

HATN_MQ_NAMESPACE_END

#endif // HATNMQINMEM_H
