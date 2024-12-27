/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file mq/mq.h
  *
  *
  */

/****************************************************************************/

#ifndef HATNMQ_H
#define HATNMQ_H

#include <functional>

#include <hatn/common/stdwrappers.h>
#include <hatn/common/objecttraits.h>

#include <hatn/mq/mqdef.h>

HATN_MQ_NAMESPACE_BEGIN

template <typename Traits, typename =void>
class Mq : public common::WithTraits<Traits>
{
    using BaseT=common::WithTraits<Traits>;
    using TraitsT=typename BaseT::TraitsT;
    using SubscriptionT=typename TraitsT::SubscriptionT;

    using PosType=typename TraitsT::PosType;
    using IdType=typename TraitsT::IdType;
    using RefIdType=typename TraitsT::RefIdType;
    using MessageType=typename TraitsT::MessageType;
    using UpdateType=typename TraitsT::UpdateType;

    struct Item
    {
        lib::optional<PosType> pos;
        lib::optional<PosType> downstreamPos;

        lib::optional<MessageType> message;
        lib::optional<UpdateType> update;

        RefIdType refId;
        bool deleted=false;

        const auto* messageId() const
        {
            if (!message)
            {
                return nullptr;
            }
            return TraitsT::messageId(message.value());
        }

        const auto* messageRefId() const
        {
            if (!message)
            {
                return nullptr;
            }
            return TraitsT::messageRefId(message.value());
        }
    };

    using ItemCb=std::function<void (Error ec, const Item& item)>;
    using ReadCb=std::function<void (Error ec, const Item& item, size_t count)>;
    using PosCb=std::function<void (Error ec, const PosType& pos)>;
    using RemoveCb=std::function<void (Error ec, const RefIdType& refId)>;

    using SubscribeCb=std::function<void (const PosType& firstPos,
                                          const PosType& lastPos,
                                          const RefIdType& firstRefId,
                                          const RefIdType& lastRefId,
                                          size_t count)>;

    using BaseT::BaseT;

    void create(MessageType item, ItemCb cb, PosType downstreamPos=PosType{})
    {
        this->traits().create(std::move(item),std::move(cb),std::move(downstreamPos));
    }

    void readById(IdType id, ItemCb cb)
    {
        this->traits().readById(std::move(id),std::move(cb));
    }

    void readByRefId(RefIdType refId, ItemCb cb)
    {
        this->traits().readByRefId(std::move(refId),std::move(cb));
    }

    void update(RefIdType refId, UpdateType data, ItemCb cb, PosType downstreamPos=PosType{})
    {
        this->traits().update(std::move(refId),std::move(data),std::move(cb),std::move(downstreamPos));
    }

    void remove(RefIdType refId, RemoveCb cb, PosType downstreamPos=PosType{})
    {
        this->traits().remove(std::move(refId),std::move(cb),std::move(downstreamPos));
    }

    void readFromPos(ReadCb cb, PosType from=PosType{}, size_t maxCount=1)
    {
        this->traits().readFrom(std::move(cb),std::move(from),maxCount);
    }

    void readToPos(ReadCb cb, PosType to=PosType{}, size_t maxCount=1)
    {
        this->traits().readTo(std::move(cb),std::move(to),maxCount);
    }

    void findDownstreamPos(PosType downstreamPos, ReadCb cb)
    {
        this->traits().findDownstreamPos(std::move(downstreamPos),std::move(cb));
    }

    void getFirstPos(PosCb cb) const
    {
        this->traits().getFirstPos(std::move(cb));
    }

    void getLastPos(PosCb cb) const
    {
        this->traits().getLastPos(std::move(cb));
    }

    const Item* next(const Item& item) const
    {
        this->traits().next(item);
    }

    SubscriptionT subscribe(SubscribeCb cb)
    {
        this->traits().subscribe(std::move(cb));
    }

    void unsubscribe(SubscriptionT& subscription)
    {
        this->traits().unsubscribe(subscription);
    }

    void reset()
    {
        this->traits().reset();
    }
};

#if 0

using ItemId=du::ObjectId;

template <typename MessageTraits, template <typename> class Traits, typename UpstreamT>
class Mq<MessageTraits,Traits,UpstreamT> : public Mq<MessageTraits,Traits>
{
    public:

        using MqBase=Mq<MessageTraits,Traits>;
        using Self=Mq<MessageTraits,Traits,UpstreamT>;

        using ItemT=typename MqBase::ItemT;
        using PosT=typename MqBase::PosType;
        using IdT=typename MqBase::IdType;
        using MessageT=typename MqBase::MessageType;

        using PushCb=typename MqBase::PushCb;
        using ReadCb=typename MqBase::ReadCb;
        using PosCb=typename MqBase::PosCb;

        template <typename ...Args>
        Mq<MessageTraits,Traits,UpstreamT>::Mq(
                std::shared_ptr<UpstreamT> upstream,
                Args&&... args
            ) : MqBase(std::forward<Args>(args)...),
                m_upstream(std::move(upstream))
        {}

        void push(MessageT item, PushCb cb)
        {
            if (m_upstream)
            {
                m_upstream->push(std::move(item),
                         [this,cb{std::move(cb)}](Error ec, const ItemT& item1)
                         {
                             if (ec)
                             {
                                cb(ec,item1);
                             }
                             else
                             {
                                 MqBase::push(item1,std::move(cb));
                             }
                         }
                    );
            }
            else
            {
                MqBase::push(std::move(item),std::move(cb));
            }
        }

        void readFrom(ReadCb cb, PosT from=PosT{}, size_t maxCount=1)
        {
            //! @todo read from traits
            //! is does not fit then read from upstream and update in traits then invoke callback
        }

        void readBefore(ReadCb cb, PosT to=PosT{}, size_t maxCount=1)
        {
            //! @todo read from traits
            //! is does not fit then read from upstream and update in traits then invoke callback
        }

        void getFirstPos(PosCb cb) const
        {
            //! @todo read from traits
            //! is not ready then read from upstream and update in traits then invoke callback
        }

        void getLastPos(PosCb cb) const
        {
            //! @todo same as getFirstPos()
        }

    private:

        std::shared_ptr<UpstreamT> m_upstream;
};
#endif

HATN_MQ_NAMESPACE_END

#endif // HATNMQ_H
