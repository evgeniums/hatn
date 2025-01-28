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

#include <hatn/common/error.h>
#include <hatn/common/stdwrappers.h>
#include <hatn/common/objecttraits.h>
#include <hatn/common/sharedptr.h>
#include <hatn/common/taskcontext.h>

#include <hatn/mq/mqdef.h>

HATN_MQ_NAMESPACE_BEGIN

template <typename Traits, typename Upstream=void>
class Mq : public common::WithTraits<Traits>
{
    public:

        using BaseT=common::WithTraits<Traits>;
        using TraitsT=typename BaseT::TraitsT;

        using MessageType=typename TraitsT::MessageType;
        using IdType=typename TraitsT::IdType;
        using PosType=typename TraitsT::PosType;
        using Item=typename TraitsT::Item;
        using ItemCb=typename TraitsT::ItemCb;
        using DownstreamPos=typename TraitsT::DownstreamPos;

        using BaseT::BaseT;

        void create(common::SharedPtr<common::TaskContext> ctx,
                    MessageType msg,
                    ItemCb cb,
                    lib::optional<DownstreamPos> downstreamPos=lib::optional<DownstreamPos>{}
                    )
        {
            this->traits().create(std::move(ctx),std::move(msg),std::move(cb),std::move(downstreamPos));
        }
};

template <typename Traits, typename Upstream>
class Mq<Traits,Upstream> : public common::WithTraits<Traits>
{
    public:

        using BaseT=common::WithTraits<Traits>;
        using TraitsT=typename BaseT::TraitsT;

        using MessageType=typename TraitsT::MessageType;
        using IdType=typename TraitsT::IdType;
        using PosType=typename TraitsT::PosType;
        using Item=typename TraitsT::Item;
        using ItemCb=typename TraitsT::ItemCb;
        using DownstreamPos=typename TraitsT::DownstreamPos;

        template <typename ...Args>
        Mq(
            Upstream* upstream,
            Args&& ...traitsArgs)
                : BaseT(std::forward<Args>(traitsArgs)...),
                  m_upstream(upstream)
        {}

        void create(common::SharedPtr<common::TaskContext> ctx,
                    MessageType msg,
                    ItemCb cb,
                    lib::optional<DownstreamPos> downstreamPos=lib::optional<DownstreamPos>{}
                    )
        {
            if (m_upstream!=nullptr)
            {
                auto self=this;
                auto cb1=[self,ctx,msg,cb,downstreamPos](const common::SharedPtr<common::TaskContext>& ctx, Error ec, const Item& item, bool complete)
                {
                    //! @todo Post back to origin thread
                    if (ec)
                    {
                        cb(ctx,ec,item,complete);
                        return;
                    }
                    if (complete)
                    {
                        self->traits().createAfterUpstream(ctx,item,cb);
                    }
                    else
                    {
                        cb(ctx,ec,item,false);
                    }
                };
                m_upstream->create(ctx,item,std::move(cb1),downstreamPos);
            }
            else
            {
                this->traits().create(std::move(ctx),std::move(msg),std::move(cb),std::move(downstreamPos));
            }
        }

    private:

        Upstream* m_upstream;
};


HATN_MQ_NAMESPACE_END

#endif // HATNMQ_H
