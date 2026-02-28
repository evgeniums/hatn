/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/ipp/rawtransport.ipp
  *
  */

/****************************************************************************/

#ifndef HATNAPIRAWTRANSPORT_IPP
#define HATNAPIRAWTRANSPORT_IPP

#include <hatn/common/meta/chain.h>

#include <hatn/base/configobject.h>

#include <hatn/dataunit/unitwrapper.h>
#include <hatn/dataunit/syntax.h>

#include <hatn/api/connectionpool.h>
#include <hatn/api/client/rawtransport.h>

HATN_API_NAMESPACE_BEGIN

namespace client {

//---------------------------------------------------------------

template <typename RouterT, typename Traits>
template <typename RequestT, typename CallbackT>
void RawTransport<RouterT,Traits>::sendRequest(
        common::SharedPtr<RequestT> req,
        CallbackT callback
    )
{
    auto send=[this](auto&& recv, common::SharedPtr<RequestT> req, CallbackT callback)
    {
        auto reqPtr=req.get();
        m_connectionPool.send(
            reqPtr->taskCtx,
            reqPtr->priority(),
            reqPtr->spanBuffers(),
            [recv=std::move(recv),req=std::move(req),callback=std::move(callback)](const Error& ec, auto connection) mutable
            {
                if (ec)
                {
                    callback(ec);
                    return;
                }

                // receive response
                recv(std::move(req),std::move(callback),connection);
            }
        );
    };

    auto recv=[this](common::SharedPtr<RequestT> req, CallbackT callback, auto connection)
    {
        auto reqPtr=req.get();

        m_connectionPool.recv(
            reqPtr->taskCtx,
            std::move(connection),
            reqPtr->responseData,
            std::move(callback)
        );
    };

    auto chain=hatn::chain(
        std::move(send),
        std::move(recv)
    );
    chain(std::move(req),std::move(callback));
}

//---------------------------------------------------------------

template <typename RouterT, typename Traits>
template <typename RequestT>
Error RawTransport<RouterT,Traits>::serializeRequest(
        common::SharedPtr<RequestT> req,
        lib::string_view topic,
        const Tenancy& tenancy
    )
{
    return req->serialize(topic,tenancy);
}

//---------------------------------------------------------------

template <typename RouterT, typename Traits>
Error RawTransport<RouterT,Traits>::loadLogConfig(
    const HATN_BASE_NAMESPACE::ConfigTree& configTree,
    const std::string& configPath,
    HATN_BASE_NAMESPACE::config_object::LogRecords& records,
    const HATN_BASE_NAMESPACE::config_object::LogSettings& settings
    )
{
    auto ec=base::ConfigObject<raw_transport_config::type>::loadLogConfig(configTree,configPath,records,settings);
    HATN_CHECK_EC(ec)
    m_connectionPool.setMaxConnectionsPerPriority(config().fieldValue(raw_transport_config::max_pool_priority_connections));

    return OK;
}

//---------------------------------------------------------------

template <typename RouterT, typename Traits>
bool RawTransport<RouterT,Traits>::canSend(Priority p) const
{
    return m_connectionPool.canSend(p);
}

//---------------------------------------------------------------

template <typename RouterT, typename Traits>
template <typename ContextT, typename CallbackT>
void RawTransport<RouterT,Traits>::close(
    common::SharedPtr<ContextT> ctx,
    CallbackT callback
    )
{
    m_connectionPool.close(std::move(ctx),std::move(callback));
}

//---------------------------------------------------------------

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPIRAWTRANSPORT_IPP
