/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file grpcclient/ipp/grpctransport.ipp
  *
  */

#ifndef HATNGRPCTRANSPORT_IPP
#define HATNGRPCTRANSPORT_IPP

#include <memory>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>

#include <grpcpp/grpcpp.h>
#include <grpcpp/generic/generic_stub.h>

#include <hatn/common/locker.h>
#include <hatn/common/containerutils.h>
#include <hatn/logcontext/contextlogger.h>
#include <hatn/api/apiliberror.h>
#include <hatn/api/client/clientresponse.h>
#include <hatn/clientserver/protomessagemap.h>

#include <hatn/grpcclient/grpcstream.h>
#include <hatn/grpcclient/grpctransport.h>

HATN_GRPCCLIENT_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

namespace detail {

struct RequestEntry {
    uintptr_t reqRawAddr;
    std::shared_ptr<grpc::ClientContext> grpcContext;

    RequestEntry(uintptr_t addr, std::shared_ptr<grpc::ClientContext> grpcContext)
        : reqRawAddr(addr), grpcContext(std::move(grpcContext))
    {}
};

using Requests = boost::multi_index::multi_index_container<
    RequestEntry,
    boost::multi_index::indexed_by<
        // First Index: Unique uintptr_t
        boost::multi_index::ordered_unique<
            boost::multi_index::tag<struct by_address>,
            boost::multi_index::member<RequestEntry, uintptr_t, &RequestEntry::reqRawAddr>
            >,
        // Second Index: Unique std::shared_ptr
        boost::multi_index::ordered_unique<
            boost::multi_index::tag<struct by_shared_ptr>,
            boost::multi_index::member<RequestEntry, std::shared_ptr<grpc::ClientContext>, &RequestEntry::grpcContext>
            >
        >
    >;

struct PriorityChannel
{
    std::shared_ptr<grpc::Channel> channel;
    std::shared_ptr<grpc::GenericStub> stub;
    common::MutexLock mutex;

    Requests pendingRequests;

    std::set<std::shared_ptr<GrpcStream>> streams;

    PriorityChannel()
    {}

    void init(const std::string& address,
              std::shared_ptr<grpc::ChannelCredentials> creds,
              const std::string& userAgent,
              const std::string& configJson
              );

    void close()
    {
        common::MutexScopedLock l{mutex};

        auto& ctxIdx = pendingRequests.get<by_shared_ptr>();
        for (const auto& entry : ctxIdx)
        {
            entry.grpcContext->TryCancel();
        }

        stub.reset();
        channel.reset();
        pendingRequests.clear();
    }

    template <typename RequestT>
    std::shared_ptr<grpc::ClientContext> addRequest(common::SharedPtr<RequestT> req)
    {
        auto ctx=std::make_shared<grpc::ClientContext>();

        {
            common::MutexScopedLock l{mutex};
            pendingRequests.emplace(reinterpret_cast<uintptr_t>(req.get()),ctx);
        }

        return ctx;
    }

    void cancelRequest(uintptr_t ptr)
    {
        common::MutexScopedLock l{mutex};

        auto& reqIndex = pendingRequests.get<by_address>();
        auto it = reqIndex.find(ptr);
        if(it != reqIndex.end())
        {
            (*(it->grpcContext)).TryCancel();
        }
    }

    template <typename RequestT>
    void cancelRequest(common::SharedPtr<RequestT> req)
    {
        cancelRequest(reinterpret_cast<uintptr_t>(req.get()));
    }

    template <typename RequestT>
    void removeRequest(common::SharedPtr<RequestT> req)
    {
        common::MutexScopedLock l{mutex};

        auto& reqIndex = pendingRequests.get<by_address>();
        reqIndex.erase(reinterpret_cast<uintptr_t>(req.get()));
    }

    void removeStream(std::shared_ptr<GrpcStream> stream)
    {
        common::MutexScopedLock l{mutex};
        streams.erase(stream);
    }
};

class GrpcTransport_p
{
public:

    GrpcTransport* transport;

    std::string name;

    common::SharedPtr<Router> router;
    common::ThreadQWithTaskContext* thread;
    const common::pmr::AllocatorFactory* factory;

    std::map<api::Priority,detail::PriorityChannel> channels;

    detail::PriorityChannel defaultChannel;
    std::map<std::string,std::string> typeMap;

    inline detail::PriorityChannel* channel(HATN_API_NAMESPACE::Priority p)
    {
        auto it=channels.find(p);
        if (it!=channels.end())
        {
            return &it->second;
        }
        return &defaultChannel;
    }

    inline Error makeError(
        int grpcCode,
        std::string status,
        const grpc_config::type& config,
        const grpc::ClientContext* grpcCtx,
        std::string messageType,
        common::ByteArrayShared respData
    ) const;

    inline std::string findHeader(const std::multimap<grpc::string_ref,grpc::string_ref>& metadata, lib::string_view headerName) const
    {
        auto it = metadata.find(grpc::string_ref{headerName.data(),headerName.size()});
        if (it != metadata.end())
        {
            return std::string{it->second.data(),it->second.size()};
        }
        return std::string{};
    };

    inline std::string mapMessageType(const std::string& pb) const
    {
        auto it=typeMap.find(pb);
        if (it!=typeMap.end())
        {
            return it->second;
        }
        return HATN_CLIENT_SERVER_NAMESPACE::ProtoMessageMap::instance().findCppByProto(pb);
    }

    Result<clientapi::Response> handleResponse(
        std::shared_ptr<grpc::ClientContext> context,
        grpc::Status status,
        const grpc::ByteBuffer& respBuffer
    ) const;

    void removeStream(api::Priority p, std::shared_ptr<GrpcStream> stream)
    {
        auto ch=channel(p);
        ch->removeStream(std::move(stream));
    }
};

inline Error GrpcTransport_p::makeError(
        int grpcCode,
        std::string status,
        const grpc_config::type& config,
        const grpc::ClientContext* grpcCtx,
        std::string messageType,
        common::ByteArrayShared respData
    ) const
{
    int code=-grpcCode;
    const std::multimap<grpc::string_ref, grpc::string_ref>& metadata = grpcCtx->GetServerInitialMetadata();

    auto family=findHeader(metadata,config.fieldValue(grpc_config::error_family_header));
    auto description=findHeader(metadata,config.fieldValue(grpc_config::error_description_header));

    // make api error from response_error_message
    auto nativeError=std::make_shared<common::NativeError>(-1,&api::ApiLibErrorCategory::getCategory());
    common::ApiError apiError{code};
    nativeError->setApiError(std::move(apiError));
    nativeError->mutableApiError()->setDescription(description);
    nativeError->mutableApiError()->setFamily(family);
    nativeError->mutableApiError()->setStatus(status);
    if (!messageType.empty() && !respData->empty())
    {
        nativeError->mutableApiError()->setDataType(messageType);
        nativeError->mutableApiError()->setData(respData);
    }
    return Error{api::ApiLibError::SERVER_RESPONDED_WITH_ERROR,std::move(nativeError)};
}

}

//--------------------------------------------------------------------------

template <typename RequestT, typename CallbackT>
void GrpcTransport::sendRequest(
        common::SharedPtr<RequestT> req,
        CallbackT callback
    )
{
    HATN_CTX_SCOPE_WITH_BARRIER("grpctransport::sendrequest")

    // find channel for priority
    auto channel=pimpl->channel(req->priority());

    // create context
    auto context = channel->addRequest(req);
    if (config().fieldValue(grpc_config::deadline_timeout)!=0)
    {
        context->set_wait_for_ready(true);
        std::chrono::system_clock::time_point deadline =
            std::chrono::system_clock::now() + std::chrono::seconds(config().fieldValue(grpc_config::deadline_timeout));
        context->set_deadline(deadline);
    }

    // add authorization token and other headers to context
    if (!req->session().isNull())
    {
        auto token=req->session().sessionToken();
        if (!token.isNull())
        {
            std::string tokenBase64;
            common::ContainerUtils::rawToBase64(*token,tokenBase64);

            auto bearer=fmt::format("Bearer {}",tokenBase64);
#if 0
            std::cout << "GrpcTransport::sendRequest bearer: " << bearer << std::endl;
#endif
            //! @todo optimization: store tag names in strings
            context->AddMetadata("authorization", bearer);
            context->AddMetadata(std::string{config().fieldValue(grpc_config::auth_tag_header)}, req->session().tokenTag());
        }
    }
    if (!req->topic().empty())
    {
        context->AddMetadata(std::string{config().fieldValue(grpc_config::topic_header)}, req->topic());
    }
    if (req->tenancy() && !req->tenancy()->tenancyId().empty())
    {
        context->AddMetadata(std::string{config().fieldValue(grpc_config::tenancy_header)}, std::string{req->tenancy()->tenancyId()});
    }

    // add headers from method auth
    for (const auto& methodHeader : req->methodAuth().headers())
    {
        context->AddMetadata(methodHeader.first,methodHeader.second);
    }

    // prepare request

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

    common::ByteArray barr;

    // prepare buffers
    auto bufs=req->message().chainBuffers();
    std::vector<grpc::Slice> slices;
    slices.reserve(bufs.size());
    for (const auto& buf : bufs)
    {
        slices.emplace_back(buf.data(),buf.size());
        barr.append(buf.data(),buf.size());
    }
    grpc::ByteBuffer requestBuf(slices.data(),slices.size());
    auto responseBuf=std::make_shared<grpc::ByteBuffer>();

    // prepare callback
    auto cb=[req,channel,responseBuf,callback,context,bufs,this](grpc::Status status)
    {
#if 0
        const std::multimap<grpc::string_ref, grpc::string_ref>& metadata = context->GetServerInitialMetadata();

#if 1
        // dump response headers
        std::cout << "------HEADERS-----" << std::endl;
        for (auto it = metadata.begin(); it != metadata.end(); ++it) {
            // Convert string_ref to std::string for printing
            std::cout << std::string(it->first.data(), it->first.size()) << ": "
                      << std::string(it->second.data(), it->second.size()) << std::endl;
        }
std::cout << "-----------------" << std::endl;
#endif

        // copy response data
        common::ByteArrayShared respData=common::makeShared<common::ByteArray>();
        std::vector<grpc::Slice> slices;
        if (responseBuf->Dump(&slices).ok())
        {
            for (const auto& slice : slices)
            {
                respData->append(reinterpret_cast<const char*>(slice.begin()),slice.size());
            }
        }

        // parse response
        clientapi::Response resp;
        resp.setStatus(api::protocol::ResponseStatus::Success);
        resp.setMessageData(respData);
        auto respType=pimpl->findHeader(metadata,config().fieldValue(grpc_config::message_type_header));
        auto mappedRespType=pimpl->mapMessageType(respType);
        resp.setMessageType(mappedRespType);
        resp.setId(pimpl->findHeader(metadata,config().fieldValue(grpc_config::id_header)));

        // check status
        if (!status.ok())
        {
            if (status.error_code()==grpc::StatusCode::UNAUTHENTICATED)
            {
                resp.setStatus(api::protocol::ResponseStatus::AuthError);
            }
            else
            {
                //! @todo maybe map appStatus to response status
                resp.setStatus(api::protocol::ResponseStatus::Generic);
            }

            auto appStatus=pimpl->findHeader(metadata,config().fieldValue(grpc_config::status_header));
            if (appStatus.empty())
            {
                // possibly network errors
                auto nativeErr=std::make_shared<common::NativeError>(
                    status.error_message(),
                    -status.error_code(),
                    &api::ApiLibErrorCategory::getCategory()
                    );
                Error ec{
                    api::ApiLibError::TRANSPORT_REQUEST_FAILED,
                    std::move(nativeErr)
                };

                channel->removeRequest(req);
                callback(std::move(ec));
                return;
            }

            auto apiError=pimpl->makeError(
                    status.error_code(),
                    std::move(appStatus),
                    config(),
                    context.get(),
                    resp.messageType(),
                    respData
                );
            resp.setErrror(std::move(apiError));
        }

        // done                
        req->setResponse(std::move(resp));
        channel->removeRequest(req);
        HATN_CTX_STACK_BARRIER_OFF("grpctransport::sendrequest")
        callback({});
#else

        auto response =  pimpl->handleResponse(
                    context,
                    status,
                    *responseBuf
                );
        if (responseBuf)
        {
            channel->removeRequest(req);
            callback(response.takeError());
            return;
        }

        req->setResponse(response.takeValue());
        channel->removeRequest(req);
        HATN_CTX_STACK_BARRIER_OFF("grpctransport::sendrequest")
        callback({});
#endif
    };

    // invoke a call
    grpc::StubOptions opt;
    grpc::GenericStub* stub=channel->stub.get();
    stub->UnaryCall(
        context.get(),
        method,
        opt,
        &requestBuf,
        responseBuf.get(),
        cb
    );
}

//--------------------------------------------------------------------------

template <typename RequestT>
void GrpcTransport::cancelRequest(
        common::SharedPtr<RequestT> req
    )
{
    auto channel=pimpl->channel(req->priority());
    channel->cancelRequest(req);
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
