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

#include <hatn/clientapp/eventdispatcher.h>
#include <hatn/clientapp/methodnetworkstatus.h>
#include <hatn/clientapp/systemservice.h>

#include <hatn/dataunit/ipp/syntax.ipp>

HATN_CLIENTAPP_NAMESPACE_BEGIN

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

    HATN_CTX_DEBUG_RECORDS(1,"network status updated",{"network_status",msg->toString(true)})

    auto app=clientApp(env,ctx);
    if (app!=nullptr)
    {
        auto event=std::make_shared<Event>();
        event->category=EventCategory;

        if (!msg->fieldValue(network_status::connected))
        {
            event->event=EventDisconnect;
        }
        else
        {
            event->event=msg->fieldValue(network_status::event);
            if (event->event.empty())
            {
                event->event=EventConnect;
            }
        }

        app->eventDispatcher().publish(env,ctx,event);
    }

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
