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

#include <boost/icl/interval_map.hpp>

#include <hatn/common/pmr/allocatorfactory.h>
#include <hatn/common/flatmap.h>
#include <hatn/common/cachelruttl.h>
#include <hatn/common/locker.h>
#include <hatn/common/withthread.h>

#include <hatn/mq/mqdef.h>
#include <hatn/mq/mqconfig.h>

HATN_MQ_NAMESPACE_BEGIN

#if 0
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

        struct IdInterval
        {
            TopicId topic;
            IdType from;
            IdType to;
        };

        struct PosInterval
        {
            TopicId topic;
            PosType from;
            PosType to;
        };

        using ReadMessageContainers=boost::icl::interval_map<IdType,Item>;
        using ReadQueueContainers=boost::icl::interval_map<PosType,Item>;

        common::CacheLruTtl<IdInterval,ReadMessageContainers> m_topicReadMessageContainers;
        common::CacheLruTtl<PosInterval,ReadQueueContainers> m_topicReadQueueContainers;

        common::TaskWithContextThread* m_thread;

        template <typename MessageTraitsT1, typename PosT1=du::ObjectId>
        friend class MqInmemTraits;
};

#endif

template <typename MessageTraitsT, typename PosTraits=PosTraitsBase>
class MqInmemTraits : public MqTraitsBase<MessageTraitsT,PosTraits>
{
    public:

        using BaseT=MqTraitsBase<MessageTraitsT,PosTraits>;
        using MessageType=typename BaseT::MessageType;
        using IdType=typename BaseT::IdType;
        using PosType=typename PosTraits::Type;
        using Item=typename BaseT::Item;
        using ItemCb=typename BaseT::ItemCb;
        using DownstreamPos=typename BaseT::DownstreamPos;

        //! @todo Fix constructors
        MqInmemTraits(uint64_t ttl=1000) : m_writeQueue(ttl)
        {}

        void create(common::SharedPtr<TaskContext> ctx, MessageType msg, ItemCb cb, lib::optional<DownstreamPos> downstreamPos=lib::optional<DownstreamPos>{})
        {
            //! @todo Post to thread

            auto pos=this->nextPos();
            const auto& item=m_writeQueue.emplaceItem(pos,msg,pos,std::move(downstreamPos));

            //! @todo Update downstream pos

            //! @todo Post back to origin thread
            cb(ctx,ec,item,true);

            //! @todo Process subscriptions
        }

        void createAfterUpstream(common::SharedPtr<TaskContext> ctx, const Item& item, ItemCb cb)
        {
            //! @todo Post to thread

            const auto& newItem=m_writeQueue.pushItem(item.pos,item);

            //! @todo Update downstream pos

            //! @todo Post back to origin thread
            cb(ctx,ec,newItem,true);

            //! @todo Process subscriptions
        }

#if 0
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
#endif
    private:

        common::CacheLruTtl<PosType,Item> m_writeQueue;

#if 0
        common::pmr::map<DownstreamId,PosType> m_lastDownstreamPos;

        PosType m_firstPos;
        PosType m_lastPos;

        common::pmr::map<IdInterval,ItemQueueInLruTtl<Item>,std::less<>> m_readMessages;
        common::pmr::map<PosInterval,ItemQueueInLruTtl<Item>,std::less<>> m_readQueues;

        common::pmr::map<Subscription::IdType,SubscribeCb> m_subscriptions;
        TopicId m_topic;

        MqInmemPool* m_pool;
        typename common::CacheLruTtl<TopicId,MqInmem<MessageTraitsT,PosT>>::Item *m_selfPoolItem;
#endif
};

HATN_MQ_NAMESPACE_END

#endif // HATNMQINMEM_H
