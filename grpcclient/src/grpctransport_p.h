/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file grpcclient/src/grpctransport_p.h
  *
  *  DLL-private implementation header for GrpcTransport.
  *
  *  Holds every gRPC-dependent type (PriorityChannel, GrpcTransport_p, the
  *  request/stream registries and the response helpers). It is included ONLY by
  *  the hatngrpcclient sources (grpctransport.cpp, grpcstream.cpp) so that gRPC,
  *  abseil and protobuf are fully encapsulated inside hatngrpcclient.dll and never
  *  leak into consumers. See grpctransport.ipp (grpc-free) for the public template
  *  layer that hands off to the exported GrpcTransport::*Impl methods.
  */

/****************************************************************************/

#ifndef HATNGRPCTRANSPORT_P_H
#define HATNGRPCTRANSPORT_P_H

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

#include <hatn/grpcclient/grpctransport.h>

#include "grpcstream.h"

HATN_GRPCCLIENT_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

namespace detail {

class GrpcTransport_p;

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
    mutable common::MutexLock mutex;

    Requests pendingRequests;
    Streams streams;

    bool disconnected=false;

    PriorityChannel()
    {}

    void init(const GrpcTransport* cfg,
              const std::string& address,
              std::shared_ptr<grpc::ChannelCredentials> creds,
              const std::string& userAgent,
              const std::string& serverName
              );

    bool isDisconnected() const
    {
        common::MutexScopedLock l{mutex};
        return disconnected;
    }

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

    void updateNetworkState(bool disconnect)
    {
        if (disconnect)
        {
            std::vector<std::shared_ptr<grpc::ClientContext>> contexts;

            {
                common::MutexScopedLock l{mutex};
                if (disconnected)
                {
                    return;
                }

                disconnect=true;
                auto& ctxIdx = pendingRequests.get<by_shared_ptr>();
                for (const auto& entry : ctxIdx)
                {
                    contexts.emplace_back(entry.grpcContext);
                }
            }

            for (const auto& ctx : contexts)
            {
                ctx->TryCancel();
            }

            closeAllStreams();

            {
                common::MutexScopedLock l{mutex};
                pendingRequests.clear();
            }

            return;
        }

        {
            common::MutexScopedLock l{mutex};

            disconnected=false;
            channel->GetState(true);
        }
    }

    std::shared_ptr<grpc::ClientContext> addRequest(uintptr_t reqAddr)
    {
        auto ctx=std::make_shared<grpc::ClientContext>();

        {
            common::MutexScopedLock l{mutex};
            pendingRequests.emplace(reqAddr,ctx);
        }

        return ctx;
    }

    std::shared_ptr<GrpcStream> addStream(
                                    uintptr_t reqAddr,
                                    api::Priority priority,
                                    std::shared_ptr<detail::GrpcTransport_p> transport
                                )
    {
        auto ctx=std::make_shared<grpc::ClientContext>();
        auto stream=std::make_shared<GrpcStream>(transport,priority,std::move(ctx));

        {
            common::MutexScopedLock l{mutex};
            streams.emplace(reqAddr,stream);
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

    void removeRequest(uintptr_t reqPtr)
    {
        common::MutexScopedLock l{mutex};

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
        std::vector<std::shared_ptr<GrpcStream>> st;
        mutex.lock();
        for (auto&& it: streams.get<by_shared_ptr>())
        {
            st.push_back(it.stream);
        }
        mutex.unlock();

        for (auto& it : st)
        {
            it->close();
        }

        mutex.lock();
        streams.clear();
        mutex.unlock();
    }

    void resetState()
    {
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

} // namespace detail

HATN_GRPCCLIENT_NAMESPACE_END

#endif // HATNGRPCTRANSPORT_P_H
