/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/ipp/serverservice.ipp
  *
  */

/****************************************************************************/

#ifndef HATNAPISERVERSERVICE_IPP
#define HATNAPISERVERSERVICE_IPP

#include <hatn/dataunit/wirebufsolid.h>
#include <hatn/dataunit/visitors.h>

#include <hatn/api/server/serverservice.h>

HATN_API_NAMESPACE_BEGIN

namespace server {

//---------------------------------------------------------------

template <typename Traits>
template <typename RequestT>
void ServerService<Traits>::handleRequest(
        common::SharedPtr<api::server::RequestContext<RequestT>> request,
        api::server::RouteCb<RequestT> callback
    ) const
{
#if 0
    // check method
    if (request->unit.field(api::request::method).value()!=lib::string_view{ApiRequestMethod})
    {
        //! @todo report invalid service method
        return;
    }

    // check message type
    if (request->unit.field(api::request::message_type).value()!=lib::string_view{ApiRequestMessageType})
    {
        //! @todo report invalid message type
        return;
    }

    // check if message is set
    if (!request->unit.field(api::request::message).isSet())
    {
        //! @todo report message not set
        return;
    }

    // parse request content
    auto msg=request->env->template get<api::server::AllocatorFactory>().factory()->template createObject<Message>();
    const auto& messageField=request->unit.field(api::request::message);
    du::WireBufSolidShared buf{messageField.skippedNotParsedContent()};
    Error ec;
    du::io::deserialize(*msg,buf,ec);
    if (ec)
    {
        //! @todo report message parsing failed
        return;
    }

    // fill db message fields

    // parse object

    // check it producer_pos outdated

    // save object and mq message in db

    // notify that mq message received

    // invoke callback
#endif
}

//---------------------------------------------------------------

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPISERVERSERVICE_IPP
