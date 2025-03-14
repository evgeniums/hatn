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
    auto& req=request->template get<RequestT>();

    auto cb=[callback(std::move(callback)),&req](common::SharedPtr<api::server::RequestContext<RequestT>> request)
    {
        //! @todo Log status?
        callback(std::move(request));
    };

    auto handleMessage=[request{std::move(request)},cb{std::move(cb)},this,&req](ServiceMethodStatus status, auto handler, auto msg, const auto& validator)
    {
        // check status
        if (status>ServiceMethodStatus::MessageNotRequred)
        {
            switch (status)
            {
                case(ServiceMethodStatus::UnknownMethod): req.response.setStatus(protocol::ResponseStatus::UnknownMethod); break;
                case(ServiceMethodStatus::UnknownMessageType): req.response.setStatus(protocol::ResponseStatus::UnknownMessageType); break;
                case(ServiceMethodStatus::MessageMissing): req.response.setStatus(protocol::ResponseStatus::MessageMissing); break;

                default:
                    req.response.setStatus(protocol::ResponseStatus::InternalServerError);
                    break;
            }

            cb(std::move(request));
            return;
        }

        // if message not required invoke handler without message
        if (status==ServiceMethodStatus::MessageNotRequred)
        {
            handler(std::move(request),std::move(msg),std::move(cb));
            return;
        }

        // build message object
        using msgType=typename std::decay_t<decltype(msg)>::element_type;
        msg=req.template get<AllocatorFactory>().factory()->template createObject<msgType>();

        //  parse message
        const auto& messageField=request->unit.field(protocol::request::message);
        du::WireBufSolidShared buf{messageField.skippedNotParsedContent()};
        if (!du::io::deserialize(*msg,buf))
        {
            req.response.setStatus(protocol::ResponseStatus::FormatError);
            cb(std::move(request));
            return;
        }

        // validate message
        auto validationStatus=validator.apply(*msg);
        if (!validationStatus)
        {
            //! @todo construct locale aware validation report

            req.response.setStatus(protocol::ResponseStatus::ValidationError);
            cb(std::move(request));
            return;
        }

        // handle message in service
        handler(std::move(request),std::move(cb));
    };

    // prepare handler for request
    prepareHandler(&req,
                   req.unit.field(protocol::request::method).value(),
                   req.unit.field(protocol::request::message).isSet(),
                   req.unit.field(protocol::request::message_type).value(),
                   std::move(handleMessage)
                   );
}

//---------------------------------------------------------------

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPISERVERSERVICE_IPP
