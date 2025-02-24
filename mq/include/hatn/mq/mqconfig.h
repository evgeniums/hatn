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
    common::SharedPtr<typename Traits::DownstreamPos> downstreamPos;

    lib::optional<typename Traits::MessageType> message;
    lib::optional<typename Traits::UpdateType> update;

    typename Traits::RefIdType refId;
    bool deleted;

    MqItem(typename Traits::MessageType msg,
           typename Traits::PosType pos,
           common::SharedPtr<typename Traits::DownstreamPos> downstreamPos=common::SharedPtr<typename Traits::DownstreamPos>{}
           )
        : pos(std::move(pos)),
          downstreamPos(std::move(downstreamPos)),
          message(std::move(msg)),
          deleted(false)
    {}

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
            return &refId;
        }
        return Traits::messageRefId(message.value());
    }
};

template <typename MessageT,
         typename UpdateT,
         typename IdT=du::ObjectId,
         typename RefIdT=IdT>
struct MessageTraitsBase
{
    using IdType=IdT;
    using RefIdType=RefIdT;
    using MessageType=MessageT;
    using UpdateType=UpdateT;

    static const auto* messageId(const MessageType&)
    {
        Assert(false,"Must be implemented in derived message traits");
        return nullptr;
    }

    static const auto* messageRefId(const MessageType&)
    {
        Assert(false,"Must be implemented in derived message traits");
        return nullptr;
    }
};

struct PosTraitsBase
{
    using Type=du::ObjectId;

    Type next()
    {
        return du::ObjectId::generateId();
    }
};

template <typename MessageTraitsT, typename PosTraits=PosTraitsBase>
class MqTraitsBase
{
    public:

        using SelfT=MqTraitsBase<MessageTraitsT,PosTraits>;

        using IdType=typename MessageTraitsT::IdType;
        using RefIdType=typename MessageTraitsT::RefIdType;
        using MessageType=typename MessageTraitsT::MessageType;
        using UpdateType=typename MessageTraitsT::UpdateType;

        using PosType=typename PosTraits::Type;

        using Item=MqItem<SelfT>;

        struct DownstreamPos
        {
            DownstreamId downstreamId;
            PosType pos;
        };

        using CreateCb=std::function<void (const common::SharedPtr<TaskContext>& ctx, Error ec, const RefIdType& refId, const common::SharedPtr<DownstreamPos>& downstreamPos, bool complete)>;
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

        struct Subscription
        {
            using Id=uint32_t;

            Id id=0;
            common::WeakPtr<TaskContext> ctx;
        };
        using SubscriptionT=Subscription;

        static const auto* messageId(const MessageType& msg)
        {
            return MessageTraitsT::messageId(msg);
        }

        static const auto* messageRefId(const MessageType& msg)
        {
            return MessageTraitsT::messageRefId(msg);
        }

        PosType nextPos()
        {
            return PosTraits::next();
        }
};

HATN_MQ_NAMESPACE_END

#endif // HATNMQCONFIG_H
