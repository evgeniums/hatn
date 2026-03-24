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
#include <hatn/api/client/clientrequest.h>
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

struct StreamEntry {
    uintptr_t reqRawAddr;
    std::shared_ptr<GrpcStream> stream;

    StreamEntry(uintptr_t addr, std::shared_ptr<GrpcStream> stream)
        : reqRawAddr(addr), stream(std::move(stream))
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

using Streams = boost::multi_index::multi_index_container<
    StreamEntry,
    boost::multi_index::indexed_by<
        // First Index: Unique uintptr_t
        boost::multi_index::ordered_unique<
            boost::multi_index::tag<struct by_address>,
            boost::multi_index::member<StreamEntry, uintptr_t, &StreamEntry::reqRawAddr>
            >,
        // Second Index: Unique std::shared_ptr
        boost::multi_index::ordered_unique<
            boost::multi_index::tag<struct by_shared_ptr>,
            boost::multi_index::member<StreamEntry, std::shared_ptr<GrpcStream>, &StreamEntry::stream>
            >
        >
    >;

struct PriorityChannel
{
    std::shared_ptr<grpc::Channel> channel;
    std::shared_ptr<grpc::GenericStub> stub;
    common::MutexLock mutex;

    Requests pendingRequests;
    Streams streams;

    PriorityChannel()
    {}

    void init(const GrpcTransport* cfg,
              const std::string& address,
              std::shared_ptr<grpc::ChannelCredentials> creds,
              const std::string& userAgent
              );

    void close()
    {
        mutex.lock();
        std::vector<std::shared_ptr<grpc::ClientContext>> contexts;
        auto& ctxIdx = pendingRequests.get<by_shared_ptr>();
        for (const auto& entry : ctxIdx)
        {
            contexts.emplace_back(entry.grpcContext);
        }
        mutex.unlock();

        for (const auto& ctx : contexts)
        {
            ctx->TryCancel();
        }

        closeAllStreams();

        {
            common::MutexScopedLock l{mutex};
            stub.reset();
            channel.reset();
            pendingRequests.clear();
        }
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

    template <typename RequestT>
    std::shared_ptr<GrpcStream> addStream(
                                    common::SharedPtr<RequestT> req,
                                    std::shared_ptr<detail::GrpcTransport_p> transport
                                )
    {
        auto ctx=std::make_shared<grpc::ClientContext>();
        auto stream=std::make_shared<GrpcStream>(transport,req->priority(),std::move(ctx));

        {
            common::MutexScopedLock l{mutex};
            streams.emplace(reinterpret_cast<uintptr_t>(req.get()),stream);
        }

        return stream;
    }

    void cancelRequest(uintptr_t ptr)
    {
        std::shared_ptr<grpc::ClientContext> ctx;
        {
            common::MutexScopedLock l{mutex};

            auto& reqIndex = pendingRequests.get<by_address>();
            auto it = reqIndex.find(ptr);
            if(it != reqIndex.end())
            {
                ctx=it->grpcContext;
            }
        }
        if (ctx)
        {
            ctx->TryCancel();
        }

        std::shared_ptr<GrpcStream> stream;
        {
            common::MutexScopedLock l{mutex};

            auto& streamIndex = streams.get<by_address>();
            auto it = streamIndex.find(ptr);
            if(it != streamIndex.end())
            {
                stream=it->stream;
            }
        }
        if (stream)
        {
            stream->close();
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

        auto reqPtr=reinterpret_cast<uintptr_t>(req.get());

        auto& reqIndex = pendingRequests.get<by_address>();
        reqIndex.erase(reqPtr);

        auto& streamIndex = streams.get<by_address>();
        streamIndex.erase(reqPtr);
    }

    void removeStream(std::shared_ptr<GrpcStream> stream)
    {
        common::MutexScopedLock l{mutex};
        auto& streamIndex = streams.get<by_shared_ptr>();
        streamIndex.erase(stream);
    }

    void closeAllStreams()
    {
        mutex.lock();
        auto& st=streams.get<by_shared_ptr>();
        mutex.unlock();

        for (auto& it : st)
        {
            it.stream->close();
        }

        mutex.lock();
        streams.clear();
        mutex.unlock();
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

    static inline HATN_API_NAMESPACE::ApiResponseStatus apiStatus(grpc::StatusCode grpcCode);

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
        const grpc::ByteBuffer& respBuffer,
        bool streamMessage=false,
        std::string messageType={},
        common::ByteArrayShared messageBuf={}
    ) const;

    void removeStream(api::Priority p, std::shared_ptr<GrpcStream> stream)
    {
        auto ch=channel(p);
        ch->removeStream(std::move(stream));
    }
};

#if 0

const (
    // OK is returned on success.
    OK Code = 0

    // Canceled indicates the operation was canceled (typically by the caller).
    //
    // The gRPC framework will generate this error code when cancellation
    // is requested.
    Canceled Code = 1

    // Unknown error. An example of where this error may be returned is
    // if a Status value received from another address space belongs to
    // an error-space that is not known in this address space. Also
    // errors raised by APIs that do not return enough error information
    // may be converted to this error.
    //
    // The gRPC framework will generate this error code in the above two
    // mentioned cases.
    Unknown Code = 2

    // InvalidArgument indicates client specified an invalid argument.
    // Note that this differs from FailedPrecondition. It indicates arguments
    // that are problematic regardless of the state of the system
    // (e.g., a malformed file name).
    //
    // This error code will not be generated by the gRPC framework.
    InvalidArgument Code = 3

    // DeadlineExceeded means operation expired before completion.
    // For operations that change the state of the system, this error may be
    // returned even if the operation has completed successfully. For
    // example, a successful response from a server could have been delayed
    // long enough for the deadline to expire.
    //
    // The gRPC framework will generate this error code when the deadline is
    // exceeded.
    DeadlineExceeded Code = 4

    // NotFound means some requested entity (e.g., file or directory) was
    // not found.
    //
    // This error code will not be generated by the gRPC framework.
    NotFound Code = 5

    // AlreadyExists means an attempt to create an entity failed because one
    // already exists.
    //
    // This error code will not be generated by the gRPC framework.
    AlreadyExists Code = 6

    // PermissionDenied indicates the caller does not have permission to
    // execute the specified operation. It must not be used for rejections
    // caused by exhausting some resource (use ResourceExhausted
    // instead for those errors). It must not be
    // used if the caller cannot be identified (use Unauthenticated
    // instead for those errors).
    //
    // This error code will not be generated by the gRPC core framework,
    // but expect authentication middleware to use it.
    PermissionDenied Code = 7

    // ResourceExhausted indicates some resource has been exhausted, perhaps
    // a per-user quota, or perhaps the entire file system is out of space.
    //
    // This error code will be generated by the gRPC framework in
    // out-of-memory and server overload situations, or when a message is
    // larger than the configured maximum size.
    ResourceExhausted Code = 8

    // FailedPrecondition indicates operation was rejected because the
    // system is not in a state required for the operation's execution.
    // For example, directory to be deleted may be non-empty, an rmdir
    // operation is applied to a non-directory, etc.
    //
    // A litmus test that may help a service implementor in deciding
    // between FailedPrecondition, Aborted, and Unavailable:
    //  (a) Use Unavailable if the client can retry just the failing call.
    //  (b) Use Aborted if the client should retry at a higher-level
    //      (e.g., restarting a read-modify-write sequence).
    //  (c) Use FailedPrecondition if the client should not retry until
    //      the system state has been explicitly fixed. E.g., if an "rmdir"
    //      fails because the directory is non-empty, FailedPrecondition
    //      should be returned since the client should not retry unless
    //      they have first fixed up the directory by deleting files from it.
    //  (d) Use FailedPrecondition if the client performs conditional
    //      REST Get/Update/Delete on a resource and the resource on the
    //      server does not match the condition. E.g., conflicting
    //      read-modify-write on the same resource.
    //
    // This error code will not be generated by the gRPC framework.
    FailedPrecondition Code = 9

    // Aborted indicates the operation was aborted, typically due to a
    // concurrency issue like sequencer check failures, transaction aborts,
    // etc.
    //
    // See litmus test above for deciding between FailedPrecondition,
    // Aborted, and Unavailable.
    //
    // This error code will not be generated by the gRPC framework.
    Aborted Code = 10

    // OutOfRange means operation was attempted past the valid range.
    // E.g., seeking or reading past end of file.
    //
    // Unlike InvalidArgument, this error indicates a problem that may
    // be fixed if the system state changes. For example, a 32-bit file
    // system will generate InvalidArgument if asked to read at an
    // offset that is not in the range [0,2^32-1], but it will generate
    // OutOfRange if asked to read from an offset past the current
    // file size.
    //
    // There is a fair bit of overlap between FailedPrecondition and
    // OutOfRange. We recommend using OutOfRange (the more specific
    // error) when it applies so that callers who are iterating through
    // a space can easily look for an OutOfRange error to detect when
    // they are done.
    //
    // This error code will not be generated by the gRPC framework.
    OutOfRange Code = 11

    // Unimplemented indicates operation is not implemented or not
    // supported/enabled in this service.
    //
    // This error code will be generated by the gRPC framework. Most
    // commonly, you will see this error code when a method implementation
    // is missing on the server. It can also be generated for unknown
    // compression algorithms or a disagreement as to whether an RPC should
    // be streaming.
    Unimplemented Code = 12

    // Internal errors. Means some invariants expected by underlying
    // system has been broken. If you see one of these errors,
    // something is very broken.
    //
    // This error code will be generated by the gRPC framework in several
    // internal error conditions.
    Internal Code = 13

    // Unavailable indicates the service is currently unavailable.
    // This is a most likely a transient condition and may be corrected
    // by retrying with a backoff. Note that it is not always safe to retry
    // non-idempotent operations.
    //
    // See litmus test above for deciding between FailedPrecondition,
    // Aborted, and Unavailable.
    //
    // This error code will be generated by the gRPC framework during
    // abrupt shutdown of a server process or network connection.
    Unavailable Code = 14

    // DataLoss indicates unrecoverable data loss or corruption.
    //
    // This error code will not be generated by the gRPC framework.
    DataLoss Code = 15

    // Unauthenticated indicates the request does not have valid
    // authentication credentials for the operation.
    //
    // The gRPC framework will generate this error code when the
    // authentication metadata is invalid or a Credentials callback fails,
    // but also expect authentication middleware to generate it.
    Unauthenticated Code = 16

    _maxCode = 17
)
#endif

inline HATN_API_NAMESPACE::ApiResponseStatus GrpcTransport_p::apiStatus(grpc::StatusCode grpcCode)
{
    switch (grpcCode)
    {
    case grpc::OK: return HATN_API_NAMESPACE::ApiResponseStatus::Success;
    case grpc::CANCELLED:
    case grpc::UNKNOWN:
    case grpc::INVALID_ARGUMENT: return HATN_API_NAMESPACE::ApiResponseStatus::FormatError;
    case grpc::DEADLINE_EXCEEDED:
    case grpc::NOT_FOUND:
    case grpc::ALREADY_EXISTS:
    case grpc::PERMISSION_DENIED: return HATN_API_NAMESPACE::ApiResponseStatus::Forbidden;
    case grpc::UNAUTHENTICATED: return HATN_API_NAMESPACE::ApiResponseStatus::AuthError;
    case grpc::RESOURCE_EXHAUSTED: return HATN_API_NAMESPACE::ApiResponseStatus::RetryLater;
    case grpc::FAILED_PRECONDITION:
    case grpc::ABORTED:
    case grpc::OUT_OF_RANGE:
    case grpc::UNIMPLEMENTED: return HATN_API_NAMESPACE::ApiResponseStatus::UnknownMethod;
    case grpc::INTERNAL: return HATN_API_NAMESPACE::ApiResponseStatus::InternalServerError;
    case grpc::UNAVAILABLE:
    case grpc::DATA_LOSS:
    case grpc::DO_NOT_USE:
        break;
    }

    return HATN_API_NAMESPACE::ApiResponseStatus::Generic;
}

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
    std::shared_ptr<grpc::ClientContext> context;
    std::shared_ptr<GrpcStream> stream;
    if (req->requestType()==clientapi::RequestType::Unary)
    {
        context = channel->addRequest(req);
    }
    else
    {
        // create stream for non-unary calls
        stream=channel->addStream(req,pimpl);
        context=stream->context();
    }

    // setup deadline
    if (config().fieldValue(grpc_config::unary_deadline_timeout)!=0 && req->requestType()==clientapi::RequestType::Unary)
    {
        context->set_wait_for_ready(true);
        std::chrono::system_clock::time_point deadline =
            std::chrono::system_clock::now() + std::chrono::seconds(config().fieldValue(grpc_config::unary_deadline_timeout));
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

    grpc::GenericStub* stub=channel->stub.get();
    grpc::StubOptions opt;
    if (req->requestType()==clientapi::RequestType::Unary)
    {
        HATN_CTX_DEBUG(1,"unary call")

        // prepare callback
        auto cb=[req,channel,responseBuf,callback,context,bufs,this](grpc::Status status)
        {
            auto response = pimpl->handleResponse(
                context,
                status,
                *responseBuf
                );
            if (response)
            {
                channel->removeRequest(req);
                callback(response.takeError());
                return;
            }

            req->setResponse(response.takeValue());
            channel->removeRequest(req);
            HATN_CTX_STACK_BARRIER_OFF("grpctransport::sendrequest")
            callback({});
        };

        // invoke a call
        stub->UnaryCall(
            context.get(),
            method,
            opt,
            &requestBuf,
            responseBuf.get(),
            cb
        );
    }
    else
    {
        HATN_CTX_DEBUG(1,"stream call")

        auto cb=[req,channel,callback,context,bufs,stream,this](const Error& ec, clientapi::Response response)
        {
            if (ec)
            {
                channel->removeRequest(req);
                callback(ec);
                return;
            }

            req->setResponse(std::move(response));

            HATN_CTX_STACK_BARRIER_OFF("grpctransport::sendrequest")
            callback({});
        };

        stub->PrepareBidiStreamingCall(context.get(), method, opt, stream.get());
        stream->startStream(&requestBuf,cb);
    }
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
