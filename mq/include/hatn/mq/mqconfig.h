/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file mq/mqconfig.h
  */

/****************************************************************************/

#ifndef HATNMQCONFIG_H
#define HATNMQCONFIG_H

#include <functional>

#include <hatn/common/stdwrappers.h>
#include <hatn/common/allocatoronstack.h>
#include <hatn/common/taskcontext.h>

#include <hatn/dataunit/objectid.h>

#include <hatn/mq/mqdef.h>

HATN_MQ_NAMESPACE_BEGIN

using DownstreamId=common::StringOnStackT<du::ObjectId::Length>;
using TaskContext=common::TaskContext;
using StatusCb=std::function<void (const common::SharedPtr<TaskContext>& ctx, Error ec)>;
using TopicId=common::StringOnStackT<du::ObjectId::Length>;

template <typename Traits>
struct MqItem
{
    lib::optional<typename Traits::PosType> pos;
    lib::optional<typename Traits::DownstreamPos> downstreamPos;

    lib::optional<typename Traits::MessageType> message;
    lib::optional<typename Traits::UpdateType> update;

    typename Traits::RefIdType refId;
    bool deleted=false;

    const auto* messageId() const
    {
        if (!message)
        {
            return nullptr;
        }
        return Traits::messageId(message.value());
    }

    const auto* messageRefId() const
    {
        if (!message)
        {
            return nullptr;
        }
        return Traits::messageRefId(message.value());
    }
};

template <typename Traits>
struct MqConfig
{
    using PosType=typename Traits::PosType;
    using IdType=typename Traits::IdType;
    using RefIdType=typename Traits::RefIdType;
    using MessageType=typename Traits::MessageType;
    using UpdateType=typename Traits::UpdateType;

    using Item=MqItem<Traits>;

    using ItemCb=std::function<void (const common::SharedPtr<TaskContext>& ctx, Error ec, const Item& item)>;
    using ReadCb=std::function<void (const common::SharedPtr<TaskContext>& ctx, Error ec, const Item& item, size_t count)>;
    using PosCb=std::function<void (const common::SharedPtr<TaskContext>& ctx, Error ec, const PosType& pos)>;
    using RemoveCb=std::function<void (const common::SharedPtr<TaskContext>& ctx, Error ec, const RefIdType& refId)>;

    using SubscribeCb=std::function<void (const common::SharedPtr<TaskContext>& ctx,
                                           lib::string_view topic,
                                           const PosType& firstPos,
                                           const PosType& lastPos,
                                           const RefIdType& firstRefId,
                                           const RefIdType& lastRefId,
                                           size_t count)>;
};

template <typename MessageT,
         typename UpdateT,
         typename PosT=du::ObjectId,
         typename IdT=du::ObjectId,
         typename RefIdT=IdT>
struct MessageTraits
{
    using IdType=IdT;
    using RefIdType=RefIdT;
    using MessageType=MessageT;
    using UpdateType=UpdateT;

    const auto* messageId() const
    {
        Assert(false,"Must be implemented in derived message traits");
        return nullptr;
    }

    const auto* messageRefId() const
    {
        Assert(false,"Must be implemented in derived message traits");
        return nullptr;
    }
};

template <typename MessageTraitsT, typename PosT=du::ObjectId>
class MqTraitsBase : public MessageTraitsT, public MqConfig<MqTraitsBase<MessageTraitsT,PosT>>
{
    public:

        struct Subscription
        {
            using IdType=uint32_t;

            IdType id=0;
            common::WeakPtr<TaskContext> ctx;
        };
        using SubscriptionT=Subscription;

        struct DownstreamPos
        {
            DownstreamId downstreamId;
            PosT pos;
        };
};

HATN_MQ_NAMESPACE_END

#endif // HATNMQCONFIG_H
