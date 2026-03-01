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
#include <hatn/logcontext/contextlogger.h>
#include <hatn/api/apiliberror.h>

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

    PriorityChannel()
    {}

    void init(const std::string& address, std::shared_ptr<grpc::ChannelCredentials> creds, const std::string& userAgent);

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
};

class GrpcTransport_p
{
public:

    std::string name;

    common::SharedPtr<Router> router;
    common::ThreadQWithTaskContext* thread;
    const common::pmr::AllocatorFactory* factory;

    std::map<api::Priority,detail::PriorityChannel> channels;

    detail::PriorityChannel defaultChannel;

    detail::PriorityChannel* channel(HATN_API_NAMESPACE::Priority p)
    {
        auto it=channels.find(p);
        if (it!=channels.end())
        {
            return &it->second;
        }
        return &defaultChannel;
    }
};

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

    // add authorization token and other headers to context
    auto token=req->session().sessionToken();
    if (!token.isNull())
    {
        auto bearer=fmt::format("Bearer {}",token->toStdString());

        std::cout << "GrpcTransport::sendRequest bearer: " << bearer << std::endl;

        context->AddMetadata("authorization", bearer);
        context->AddMetadata("x-token-tag", req->session().tokenTag());
    }
    if (!req->topic().empty())
    {
        context->AddMetadata("x-topic", req->topic());
    }
    if (req->tenancy())
    {
        context->AddMetadata("x-tenancy", std::string{req->tenancy()->tenancyId()});
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

    // prepare buffers
    auto bufs=req->spanBuffers();
    std::vector<grpc::Slice> slices;
    slices.reserve(bufs.size());
    for (const auto& buf : bufs)
    {
        slices.emplace_back(buf.data(),buf.size());
    }
    grpc::ByteBuffer requestBuf(slices.data(),slices.size());
    auto responseBuf=std::make_shared<grpc::ByteBuffer>();

    // prepare callback
    auto cb=[req,channel,responseBuf,callback](grpc::Status status)
    {
        // check status
        if (!status.ok())
        {
            //! @todo Map error codes to API lib codes

            auto nativeErr=std::make_shared<common::NativeError>(
                    status.error_message(),
                    status.error_code(),
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

        // copy response data
        std::vector<grpc::Slice> slices;
        if (responseBuf->Dump(&slices).ok())
        {
            for (const auto& slice : slices)
            {
                req->responseData.appendBuffer(common::ConstDataBuf{reinterpret_cast<const char*>(slice.begin()), slice.size()});
            }
        }

        // done
        HATN_CTX_STACK_BARRIER_OFF("grpctransport::sendrequest")
        channel->removeRequest(req);
        callback({});
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

//--------------------------------------------------------------------------

HATN_GRPCCLIENT_NAMESPACE_END

#endif // HATNGRPCTRANSPORT_IPP
