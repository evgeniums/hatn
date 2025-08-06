/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientapp/methodnetworkstatus.сpp
  *
  */

#include <hatn/dataunit/syntax.h>

#include <hatn/clientapp/methodnetworkstatus.h>
#include <hatn/clientapp/systemservice.h>

#include <hatn/dataunit/ipp/syntax.ipp>

HATN_CLIENTAPP_NAMESPACE_BEGIN

HDU_UNIT(network_status,
    HDU_FIELD(connected,TYPE_BOOL,1)
)

//---------------------------------------------------------------

void MethodNetworkStatus::exec(
        common::SharedPtr<HATN_APP_NAMESPACE::AppEnv> env,
        common::SharedPtr<Context> ctx,
        Request request,
        Callback callback
    )
{
    HATN_CTX_SCOPE("networkstatus::exec")

    auto msg=request.message.as<network_status::managed>();

    HATN_CTX_DEBUG_RECORDS(1,"network status updated",{"connected",msg->fieldValue(network_status::connected)})

    std::ignore=msg;
    std::ignore=ctx;
    std::ignore=env;
    //! @todo critical: publish network event
    callback(Error{},Response{});
}

//---------------------------------------------------------------

std::string MethodNetworkStatus::messageType() const
{
    return messageTypeT<network_status::conf>();
}

//---------------------------------------------------------------

MessageBuilderFn MethodNetworkStatus::messageBuilder() const
{
    return messageBuilderT<network_status::managed>();
}

//---------------------------------------------------------------

HATN_CLIENTAPP_NAMESPACE_END

