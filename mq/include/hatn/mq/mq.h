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

#include <hatn/common/pmr/allocatorfactory.h>
#include <hatn/common/cachelru.h>
#include <hatn/common/objecttraits.h>

#include <hatn/dataunit/objectid.h>

#include <hatn/mq/mqdef.h>

HATN_MQ_NAMESPACE_BEGIN

using ItemId=du::ObjectId;

template <typename ValueTraits>
struct Item
{
    using PosType=typename ValueTraits::PosType;
    using IdType=typename ValueTraits::IdType;
    using ValueT=typename ValueTraits::ValueType;

    PosType pos;
    ValueT value;

    IdType refId;
    bool deleted;

    const auto& valueId() const
    {
        return ValueTraits::id(value);
    }

    const auto& valueRefId() const
    {
        return ValueTraits::refId(value);
    }
};

template <typename ValueTraits, template <typename> class Traits, typename =void>
class Mq : public common::WithTraits<Traits<ValueTraits>>
{
    using BaseT=common::WithTraits<Traits<ValueTraits>>;
    using TraitsT=typename BaseT::TraitsT;
    using SubscriptionT=typename TraitsT::SubscriptionT;

    using ItemType=typename TraitsT::ItemType;
    using PosType=typename TraitsT::PosType;
    using IdType=typename TraitsT::IdType;
    using ValueType=typename TraitsT::ValueType;
    using UpdateType=typename TraitsT::UpdateType;

    using ItemCb=std::function<void (Error ec, const ItemType& item)>;
    using ReadCb=std::function<void (Error ec, const ItemType& item, size_t count)>;
    using PosCb=std::function<void (Error ec, const PosType& pos)>;
    using RemoveCb=std::function<void (Error ec, const IdType& refId)>;

    using SubscribeCb=std::function<void (const PosType& firstPos,
                                          const PosType& lastPos,
                                          const PosType& firstRefId,
                                          const PosType& lastRefId,
                                          size_t count)>;

    using BaseT::BaseT;

    void create(ValueType item, ItemCb cb)
    {
        this->traits().create(std::move(item),std::move(cb));
    }

    void read(IdType refId, ItemCb cb)
    {
        this->traits().read(std::move(refId),std::move(cb));
    }

    void update(IdType refId, UpdateType data, ItemCb cb)
    {
        this->traits().update(std::move(refId),std::move(data),std::move(cb));
    }

    void remove(IdType refId, RemoveCb cb)
    {
        this->traits().remove(std::move(refId),std::move(cb));
    }

    void readFrom(ReadCb cb, PosType from=PosType{}, size_t maxCount=1)
    {
        this->traits().readFrom(std::move(cb),std::move(from),maxCount);
    }

    void readTo(ReadCb cb, PosType to=PosType{}, size_t maxCount=1)
    {
        this->traits().readTo(std::move(cb),std::move(to),maxCount);
    }

    void getFirstPos(PosCb cb) const
    {
        this->traits().getFirstPos(std::move(cb));
    }

    void getLastPos(PosCb cb) const
    {
        this->traits().getLastPos(std::move(cb));
    }

    const ItemType* next(const ItemType& item) const
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
template <typename ValueTraits, template <typename> class Traits, typename UpstreamT>
class Mq<ValueTraits,Traits,UpstreamT> : public Mq<ValueTraits,Traits>
{
    public:

        using MqBase=Mq<ValueTraits,Traits>;
        using Self=Mq<ValueTraits,Traits,UpstreamT>;

        using ItemT=typename MqBase::ItemT;
        using PosT=typename MqBase::PosType;
        using IdT=typename MqBase::IdType;
        using ValueT=typename MqBase::ValueType;

        using PushCb=typename MqBase::PushCb;
        using ReadCb=typename MqBase::ReadCb;
        using PosCb=typename MqBase::PosCb;

        template <typename ...Args>
        Mq<ValueTraits,Traits,UpstreamT>::Mq(
                std::shared_ptr<UpstreamT> upstream,
                Args&&... args
            ) : MqBase(std::forward<Args>(args)...),
                m_upstream(std::move(upstream))
        {}

        void push(ValueT item, PushCb cb)
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
