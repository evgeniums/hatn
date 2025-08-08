/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/ipp/session.ipp
  *
  */

/****************************************************************************/

#ifndef HATNAPICLIENTSESSION_IPP
#define HATNAPICLIENTSESSION_IPP

#include <hatn/logcontext/contextlogger.h>

#include <hatn/dataunit/wirebufsolid.h>
#include <hatn/dataunit/visitors.h>

#include <hatn/api/authunit.h>
#include <hatn/api/requestunit.h>
#include <hatn/api/client/session.h>

HATN_API_NAMESPACE_BEGIN

namespace client {

//---------------------------------------------------------------

template <typename UnitT>
Error SessionAuth::serializeAuthHeader(lib::string_view protocol, uint32_t protocolVersion, common::SharedPtr<UnitT> content,
                                       const common::pmr::AllocatorFactory* factory
                                       )
{
    return Auth::serializeAuthHeader(protocol,protocolVersion,std::move(content),protocol::request::session_auth.id(),factory);
}

//---------------------------------------------------------------

template <typename Traits, typename NoAuthT>
Session<Traits,NoAuthT>::Session()
    : common::WithTraits<Traits>(this),
    m_id(du::ObjectId::generateIdStr()),
    m_valid(NoAuth),
    m_refreshing(false)
{
}

//---------------------------------------------------------------

template <typename Traits, typename NoAuthT>
template <typename ...TraitsArgs>
Session<Traits,NoAuthT>::Session(TraitsArgs&& ...traitsArgs)
    : common::WithTraits<Traits>(this,std::forward<TraitsArgs>(traitsArgs)...),
        m_id(du::ObjectId::generateIdStr()),
        m_valid(NoAuth),
        m_refreshing(false)
{
}

//---------------------------------------------------------------

template <typename Traits, typename NoAuthT>
template <typename ...TraitsArgs>
Session<Traits,NoAuthT>::Session(
        lib::string_view id,
        const common::pmr::AllocatorFactory* allocatorFactory,
        TraitsArgs&& ...traitsArgs)
        : common::pmr::WithFactory(allocatorFactory),
        common::WithTraits<Traits>(this,std::forward<TraitsArgs>(traitsArgs)...),
        m_id(id),
        m_valid(NoAuth),
        m_refreshing(false)
{
}

//---------------------------------------------------------------

template <typename Traits, typename NoAuthT>
template <typename ...TraitsArgs>
Session<Traits,NoAuthT>::Session(
    const std::string& id,
    TraitsArgs&& ...traitsArgs)
    : common::WithTraits<Traits>(this,std::forward<TraitsArgs>(traitsArgs)...),
    m_id(id),
    m_valid(NoAuth),
    m_refreshing(false)
{
}

//---------------------------------------------------------------

template <typename Traits, typename NoAuthT>
template <typename ContextT, typename ClientT>
void Session<Traits,NoAuthT>::refresh(common::SharedPtr<ContextT> ctx, RefreshCb callback, ClientT* client, Response resp)
{
    m_callbacks.emplace_back(std::move(callback));

    if (isRefreshing())
    {
        return;
    }
    setRefreshing(true);

    HATN_CTX_SCOPE_WITH_BARRIER("session::refresh")

    auto sessionCtx=this->sharedMainCtx();
    sessionCtx->resetParentCtx(ctx);
    this->traits().refresh(
        std::move(ctx),        
        [sessionCtx{std::move(sessionCtx)},this](auto ctx, const Error& ec)
        {
            sessionCtx->resetParentCtx();
            HATN_CTX_STACK_BARRIER_OFF("session::refresh")

            setRefreshing(false);
            setValid(!ec);
            if (ec)
            {
                resetAuthHeader();
            }

            for (auto&& it: m_callbacks)
            {
                it(ctx,ec);
            }
            m_callbacks.clear();
        },
        client,
        std::move(resp)
        );
}

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPICLIENTSESSION_IPP
