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
        using CreateCb=typename TraitsT::CreateCb;
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
                    CreateCb cb,
                    common::SharedPtr<DownstreamPos> downstreamPos
                    )
        {
            if (m_upstream!=nullptr)
            {
                auto self=this;
                auto cb1=[self,cb{std::move(cb)}](const common::SharedPtr<common::TaskContext>& ctx, Error ec, const common::SharedPtr<DownstreamPos>& downstreamPos, bool complete)
                {
                    //! @todo ensure that cb1 is invoked in origin thread
                    if (ec)
                    {
                        cb(ctx,ec,downstreamPos,complete);
                        return;
                    }
                    if (complete)
                    {
                        //! @todo read item from upstream
                        // self->traits().createAfterUpstream(ctx,item,cb);
                    }
                    else
                    {
                        cb(ctx,ec,downstreamPos,false);
                    }
                };
                m_upstream->create(ctx,msg,std::move(cb1),std::move(downstreamPos));
            }
            else
            {
                this->traits().create(std::move(ctx),std::move(msg),std::move(cb),std::move(downstreamPos));
            }
        }

    private:

        Upstream* m_upstream;
};

template <typename Traits>
class Mq<Traits> : public common::WithTraits<Traits>
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
                    common::SharedPtr<DownstreamPos> downstreamPos
                    )
        {
            this->traits().create(std::move(ctx),std::move(msg),std::move(cb),std::move(downstreamPos));
        }
};

HATN_MQ_NAMESPACE_END

#endif // HATNMQ_H
