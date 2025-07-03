/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/ipp/clientwithauth.ipp
  */

/****************************************************************************/

#ifndef HATNCLIENTWITHAUTH_IPP
#define HATNCLIENTWITHAUTH_IPP

#include <hatn/logcontext/contextlogger.h>
#include <hatn/logcontext/logconfigrecords.h>

#include <hatn/clientserver/clientwithauth.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

template <typename RouterT, typename RequestContextT, typename MessageBufT, typename RequestUnitT, typename ...AuthProtocols>
void ClientWithAuthT<RouterT,RequestContextT,MessageBufT,RequestUnitT,AuthProtocols...>::setName(lib::string_view name)
{
    m_client->setName(name);
    session()->setId(name);
}

//--------------------------------------------------------------------------

template <typename RouterT, typename RequestContextT, typename MessageBufT, typename RequestUnitT, typename ...AuthProtocols>
lib::string_view ClientWithAuthT<RouterT,RequestContextT,MessageBufT,RequestUnitT,AuthProtocols...>::name() const
{
    return session()->id();
}

//--------------------------------------------------------------------------

template <typename RouterT, typename RequestContextT, typename MessageBufT, typename RequestUnitT, typename ...AuthProtocols>
Error ClientWithAuthT<RouterT,RequestContextT,MessageBufT,RequestUnitT,AuthProtocols...>::exec(
        common::SharedPtr<RequestContext> ctx,
        Callback callback,
        const api::Service& service,
        const api::Method& method,
        MessageType message,
        lib::string_view topic,
        api::Priority priority,
        uint32_t timeoutMs,
        clientapi::MethodAuth methodAuth
    )
{
    return m_client->exec(std::move(ctx),std::move(callback),m_sessionWrapper,service,method,std::move(message),topic,priority,timeoutMs,std::move(methodAuth));
}

//--------------------------------------------------------------------------

template <typename RouterT, typename RequestContextT, typename MessageBufT, typename RequestUnitT, typename ...AuthProtocols>
Error ClientWithAuthT<RouterT,RequestContextT,MessageBufT,RequestUnitT,AuthProtocols...>::loadLogConfig(
        const HATN_BASE_NAMESPACE::ConfigTree& configTree,
        const std::string& configPath
    )
{
    HATN_CTX_SCOPE_PUSH("client_name",name())
    auto ec=hatn::loadLogConfig(_TR("configuration of client with authentication"),*m_client,configTree,configPath);
    HATN_CHECK_EC(ec)
    return hatn::loadLogConfig(_TR("configuration of client with authentication"),*session(),configTree,HATN_BASE_NAMESPACE::ConfigTreePath{configPath}.copyAppend("session"));
}

//--------------------------------------------------------------------------

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTWITHAUTH_IPP
