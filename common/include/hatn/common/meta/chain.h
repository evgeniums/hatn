/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/meta/nodes.h
  *
  */

/****************************************************************************/

#ifndef HATNNODES_H
#define HATNNODES_H

#include <boost/hana.hpp>

#include <hatn/common/common.h>

#define HATN_CHAIN_NAMESPACE_BEGIN namespace hatn { namespace chain {
#define HATN_CHAIN_NAMESPACE_END }}

#define HATN_CHAIN_NAMESPACE hatn::common
#define HATN_CHAIN_USING using namespace hatn::chain;
#define HATN_CHAIN_NS chain

HATN_CHAIN_NAMESPACE_BEGIN

template <typename HandlerT, typename NextHandlerT=void>
struct Node
{
    template <typename ...Args>
    void operator()(Args&& ...args) const
    {
        if constexpr (std::is_same_v<NextHandlerT,void>)
        {
            handler(std::forward<Args>(args)...);
        }
        else
        {
            handler(next,std::forward<Args>(args)...);
        }
    }

    HandlerT handler;
    NextHandlerT* next=nullptr;
};

struct makeNodeT
{
    template <typename HandlerT>
    auto operator() (HandlerT&& handler) const
    {
        return Node<std::decay_t<HandlerT>>{std::forward<HandlerT>(handler),nullptr};
    }

    template <typename HandlerT, typename NextHandlerT>
    auto operator() (HandlerT&& handler, NextHandlerT) const
    {
        return Node<std::decay_t<HandlerT>,typename NextHandlerT::type>{std::forward<HandlerT>(handler),nullptr};
    }
};
constexpr makeNodeT makeNode{};

template <typename HandlerT>
struct nodeBuilderT
{
    auto operator() () const
    {
        return makeNode(handler);
    }

    template <typename NextHandlerT>
    auto operator() (NextHandlerT&&) const
    {
        using NextHandler=std::decay_t<NextHandlerT>;
        return makeNode(handler,boost::hana::type_c<NextHandler>);
    }

    HandlerT handler;
};

struct makeNodeBuilderT
{
    template <typename HandlerT>
    auto operator() (HandlerT&& handler) const
    {
        return nodeBuilderT<std::decay_t<HandlerT>>{std::forward<HandlerT>(handler)};
    }
};
constexpr makeNodeBuilderT node{};

struct nodesT
{
    template <typename ...NodeBuildersT>
    auto operator() (NodeBuildersT&& ...nodeBuilders) const
    {
        auto builders=boost::hana::make_tuple(std::forward<NodeBuildersT>(nodeBuilders)...);
        auto nodes=boost::hana::reverse_fold(
            std::move(builders),
            boost::hana::make_tuple(),
            [](auto&& state, auto&& builder)
            {
                return boost::hana::eval_if(
                    boost::hana::equal(boost::hana::size(state),boost::hana::size_c<0>),
                    [&](auto _)
                    {
                        return boost::hana::make_tuple(_(builder)());
                    },
                    [&](auto _)
                    {
                        return boost::hana::prepend(_(state),_(builder(boost::hana::front(_(state)))));
                    }
                );
            }
        );
        return nodes;
    }
};
constexpr nodesT nodes{};

struct linkT
{
    template <typename Ts>
    auto& operator() (Ts& nodes) const
    {
        void* last=nullptr;
        boost::hana::reverse_fold(
            nodes,
            last,
            [](auto&& state, auto& node)
            {
                node.next=state;
                return &node;
            }
            );
        return boost::hana::front(nodes);
    }
};
constexpr linkT link{};

HATN_CHAIN_NAMESPACE_END

#endif // HATNNODES_H
