/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file grpcclient/ipp/grpctransport.ipp
  *
  *  Public, gRPC-free template layer of GrpcTransport.
  *
  *  This header is instantiated inside consumer modules (whitemclient, tests).
  *  It must NOT reference any gRPC/abseil/protobuf type: it extracts grpc-free
  *  data from the typed request and hands off to the exported, non-template
  *  GrpcTransport::sendUnaryImpl / sendStreamImpl / cancelRequestImpl methods,
  *  which own every gRPC type and are compiled only into hatngrpcclient.dll.
  *  See grpctransport_p.h (DLL-private) for the gRPC implementation details.
  */

#ifndef HATNGRPCTRANSPORT_IPP
#define HATNGRPCTRANSPORT_IPP

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <hatn/common/containerutils.h>
#include <hatn/logcontext/contextlogger.h>
#include <hatn/dataunit/objectid.h>
#include <hatn/api/client/clientresponse.h>
#include <hatn/api/client/clientrequest.h>

#include <hatn/grpcclient/grpctransport.h>

HATN_GRPCCLIENT_NAMESPACE_BEGIN

namespace du=HATN_DATAUNIT_NAMESPACE;

//--------------------------------------------------------------------------

template <typename RequestT, typename CallbackT>
void GrpcTransport::sendRequest(
        common::SharedPtr<RequestT> req,
        CallbackT callback
    )
{
    HATN_CTX_SCOPE_WITH_BARRIER("grpctransport::sendrequest")

    const auto reqAddr=reinterpret_cast<uintptr_t>(req.get());
    const auto priority=req->priority();

    // construct full path of the method
    std::string method;
    if (req->service()->package().empty())
    {
        method=fmt::format("/{}/{}",req->service()->name(),req->method()->name());
    }
    else
    {
        method=fmt::format("/{}.{}/{}",req->service()->package(),req->service()->name(),req->method()->name());
    }
    HATN_CTX_DEBUG_RECORDS(1,"call method",{"method",method})

    // build request metadata (grpc-free): header names from config, values from request
    std::vector<std::pair<std::string,std::string>> metadata;

    // add authorization token
    if (!req->session().isNull())
    {
        auto token=req->session().sessionToken();
        if (!token.isNull())
        {
            std::string tokenBase64;
            common::ContainerUtils::rawToBase64(*token,tokenBase64);

            //! @todo optimization: store tag names in strings
            metadata.emplace_back("authorization",fmt::format("Bearer {}",tokenBase64));
            metadata.emplace_back(std::string{config().fieldValue(grpc_config::auth_tag_header)},std::string{req->session().tokenTag()});
        }
    }
    if (!req->topic().empty())
    {
        metadata.emplace_back(std::string{config().fieldValue(grpc_config::topic_header)},std::string{req->topic()});
    }
    if (req->tenancy() && !req->tenancy()->tenancyId().empty())
    {
        metadata.emplace_back(std::string{config().fieldValue(grpc_config::tenancy_header)},std::string{req->tenancy()->tenancyId()});
    }
    if (config().fieldValue(grpc_config::send_id_header))
    {
        metadata.emplace_back(std::string{config().fieldValue(grpc_config::id_header)},du::ObjectId::generateId().toString());
    }

    // add headers from method auth
    for (const auto& methodHeader : req->methodAuth().headers())
    {
        HATN_CTX_DEBUG_RECORDS(20,"sending gRPC header",{"header_name",methodHeader.first},{"header_value",methodHeader.second})
        metadata.emplace_back(methodHeader.first,methodHeader.second);
    }

    // serialize request body into a single buffer
    auto message=common::makeShared<common::ByteArray>();
    for (const auto& buf : req->message().chainBuffers())
    {
        message->append(buf.data(),buf.size());
    }

    if (req->requestType()==clientapi::RequestType::Unary)
    {
        HATN_CTX_DEBUG(1,"unary call")

        sendUnaryImpl(
            priority,
            reqAddr,
            std::move(method),
            std::move(metadata),
            std::move(message),
            [req,callback{std::move(callback)}](Result<clientapi::Response> response) mutable
            {
                if (response)
                {
                    callback(response.takeError());
                    return;
                }

                req->setResponse(response.takeValue());
                HATN_CTX_STACK_BARRIER_OFF("grpctransport::sendrequest")
                callback({});
            }
        );
    }
    else
    {
        HATN_CTX_DEBUG(1,"stream call")

        sendStreamImpl(
            priority,
            reqAddr,
            std::move(method),
            std::move(metadata),
            std::move(message),
            [req,callback{std::move(callback)}](const Error& ec, clientapi::Response response) mutable
            {
                if (ec)
                {
                    callback(ec);
                    return;
                }

                req->setResponse(std::move(response));
                HATN_CTX_STACK_BARRIER_OFF("grpctransport::sendrequest")
                callback({});
            }
        );
    }
}

//--------------------------------------------------------------------------

template <typename RequestT>
void GrpcTransport::cancelRequest(
        common::SharedPtr<RequestT> req
    )
{
    cancelRequestImpl(req->priority(),reinterpret_cast<uintptr_t>(req.get()));
}

//---------------------------------------------------------------

template <typename RequestT>
Error GrpcTransport::parseResponse(
        common::SharedPtr<RequestT> /*req*/
    )
{
    return OK;
}

//--------------------------------------------------------------------------

HATN_GRPCCLIENT_NAMESPACE_END

#endif // HATNGRPCTRANSPORT_IPP
