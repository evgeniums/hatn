/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/ipp/client.ipp
  *
  */

/****************************************************************************/

#ifndef HATNAPICLIENT_IPP
#define HATNAPICLIENT_IPP

#include <hatn/api/client/request.h>
#include <hatn/api/client/client.h>

HATN_API_NAMESPACE_BEGIN

namespace client {

//---------------------------------------------------------------

template <typename RouterTraits, typename SessionTraits, typename ContextT, typename RequestUnitT>
template <typename UnitT>
common::Result<typename Client<RouterTraits,SessionTraits,ContextT,RequestUnitT>::Req>
    Client<RouterTraits,SessionTraits,ContextT,RequestUnitT>::prepare(
        common::SharedPtr<Session<SessionTraits>> session,
        const Service& service,
        const Method& method,
        const UnitT& content
    )
{
    auto req=Req{std::move(session)};
    auto ec=req.makeUnit(service,method,content);
    HATN_CHECK_EC(ec)
    return req;
}

//---------------------------------------------------------------

template <typename RouterTraits, typename SessionTraits, typename ContextT, typename RequestUnitT>
template <typename UnitT>
Error Client<RouterTraits,SessionTraits,ContextT,RequestUnitT>::exec(
    common::SharedPtr<Context> ctx,
    common::SharedPtr<Session<SessionTraits>> session,
    const Service& service,
    const Method& method,
    const UnitT& content,
    RequestCb<Context> callback,
    lib::string_view  topic,
    Priority priority,
    uint32_t timeoutMs
    )
{
    auto req=Req{std::move(session),priority,timeoutMs};
    auto ec=req.makeUnit(service,method,content,topic);
    HATN_CHECK_EC(ec)
    doExec(std::move(ctx),std::move(req),std::move(callback));
    return OK;
}

//---------------------------------------------------------------

template <typename RouterTraits, typename SessionTraits, typename ContextT, typename RequestUnitT>
void Client<RouterTraits,SessionTraits,ContextT,RequestUnitT>::exec(
    common::SharedPtr<Context> ctx,
    Req req,
    RequestCb<Context> callback
    )
{
    doExec(std::move(ctx),std::move(req),std::move(callback));
}

//---------------------------------------------------------------

template <typename RouterTraits, typename SessionTraits, typename ContextT, typename RequestUnitT>
void Client<RouterTraits,SessionTraits,ContextT,RequestUnitT>::doExec(
        common::SharedPtr<Context> ctx,
        Req req,
        RequestCb<Context> callback,
        bool regenId
    )
{
    if (regenId)
    {
        req.regenId();
    }

    auto it=m_queues.find(req.priority());
    Assert(it!=m_queues.end(),"Unsupported API request priority");
    auto& queue=it->second;

    auto* item=queue.prepare();
    item->req=std::move(req);
    item->ctx=std::move(ctx);
    item->callback=std::move(callback);
    queue.pushItem(item);

    maybeReadQueue(queue);
}

//---------------------------------------------------------------

template <typename RouterTraits, typename SessionTraits, typename ContextT, typename RequestUnitT>
void Client<RouterTraits,SessionTraits,ContextT,RequestUnitT>::maybeReadQueue(Queue& queue)
{
    if (queue.empty())
    {
        return;
    }

    auto item=queue.pop();

    //! @todo Implement sending to connection pool
}

//---------------------------------------------------------------

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPICLIENT_IPP
