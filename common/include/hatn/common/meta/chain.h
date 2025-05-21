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

HATN_NAMESPACE_BEGIN

namespace detail
{

template <typename HandlerT, typename NextHandlerT=void>
struct Node
{
    template <typename ...Args>
    void operator()(Args&& ...args)
    {
        handler(std::move(next),std::forward<Args>(args)...);
    }

    HandlerT handler;
    NextHandlerT next;
};

template <typename HandlerT>
struct Node<HandlerT>
{
    template <typename ...Args>
    void operator()(Args&& ...args)
    {
        handler(std::forward<Args>(args)...);
    }

    HandlerT handler;
};

struct makeNodeT
{
    template <typename HandlerT>
    auto operator() (HandlerT&& handler) const
    {
        return Node<std::decay_t<HandlerT>>{std::forward<HandlerT>(handler)};
    }

    template <typename HandlerT, typename NextHandlerT>
    auto operator() (HandlerT&& handler, NextHandlerT&& next) const
    {
        using first=std::decay_t<HandlerT>;
        using second=std::decay_t<NextHandlerT>;
        return Node<first,second>{std::forward<HandlerT>(handler),std::forward<NextHandlerT>(next)};
    }
};
constexpr makeNodeT makeNode{};

struct chainT
{
    template <typename ...HandlersT>
    auto operator() (HandlersT&& ...handlers) const
    {
        auto ts=boost::hana::make_tuple(std::forward<HandlersT>(handlers)...);
        auto last=boost::hana::back(ts);
        auto frontHandlers=boost::hana::drop_back(std::move(ts));
        return boost::hana::reverse_fold(
                std::move(frontHandlers),
                makeNode(std::move(last)),
                [](auto&& state, auto&& handler)
                {
                    return makeNode(std::move(handler),std::move(state));
                }
            );
    }
};

} // namespace detail

constexpr detail::chainT chain{};

HATN_NAMESPACE_END

#endif // HATNNODES_H
