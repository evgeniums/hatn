/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file grpcclient/grpctransport.сpp
  *
  */

#include <cstdlib>
#include <mutex>
#include <string>

#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>

#ifdef _WIN32
namespace {
    inline int hatn_setenv(const char* name, const char* value, int overwrite) {
        if (!overwrite && std::getenv(name)) return 0;
        return _putenv_s(name, value);
    }
}
#else
#  define hatn_setenv ::setenv
#endif

#include <chrono>

#include <grpcpp/grpcpp.h>
#include <grpcpp/generic/generic_stub.h>

#include <hatn/common/meta/enumint.h>
#include <hatn/network/networkerror.h>
#include <hatn/grpcclient/grpctransport.h>

#include "grpctransport_p.h"

#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>

HATN_GRPCCLIENT_NAMESPACE_BEGIN

namespace api=HATN_API_NAMESPACE;
namespace network=HATN_NETWORK_NAMESPACE;

namespace {

// Enable gRPC C-core tracing from the environment so ping/keepalive/connectivity
// behavior can be inspected without a rebuild. Opt-in via HATN_GRPC_TRACE:
//   HATN_GRPC_TRACE=1            -> a sensible default set (keepalive + connectivity)
//   HATN_GRPC_TRACE=http_keepalive,subchannel,tcp   -> exactly these tracers
// HATN_GRPC_VERBOSITY overrides the log level (default DEBUG). These must be set
// before the first channel is created, hence we run this once in the transport ctor.
// overwrite=0: an explicitly exported GRPC_TRACE/GRPC_VERBOSITY always wins.
void initGrpcTraceFromEnv()
{
#if 1
    const char* trace=std::getenv("HATN_GRPC_TRACE");
    if (trace==nullptr || trace[0]=='\0')
    {
        return;
    }

    std::string traceVal{trace};
#else
    std::string traceVal{"1"};
#endif
    if (traceVal=="1" || traceVal=="on" || traceVal=="default")
    {
        traceVal="http_keepalive,connectivity_state,subchannel";
    }

    hatn_setenv("GRPC_TRACE",traceVal.c_str(),0);

    const char* verbosity=std::getenv("HATN_GRPC_VERBOSITY");
    hatn_setenv("GRPC_VERBOSITY",(verbosity!=nullptr && verbosity[0]!='\0') ? verbosity : "DEBUG",0);

    std::cout << "gRPC tracing enabled: GRPC_TRACE=" << std::getenv("GRPC_TRACE")
              << " GRPC_VERBOSITY=" << std::getenv("GRPC_VERBOSITY") << std::endl;
}

// Build gRPC channel credentials from a Router's TLS/insecure settings.
std::shared_ptr<grpc::ChannelCredentials> buildRouterCreds(const Router* router)
{
    if (router->isInsecure())
    {
        return grpc::InsecureChannelCredentials();
    }

    grpc::SslCredentialsOptions sslOpts;
    sslOpts.pem_root_certs = router->serverCerts();
    if (!router->clientPrivKey().empty())
    {
        sslOpts.pem_cert_chain = router->clientCertChain();
        sslOpts.pem_private_key = router->clientPrivKey();
    }
    return grpc::SslCredentials(sslOpts);
}

} // anonymous namespace

/*****************************GrpcTransport*********************************/

//--------------------------------------------------------------------------

GrpcTransport::GrpcTransport(
    common::ThreadQWithTaskContext* thread,
    const common::pmr::AllocatorFactory* factory
    ) : pimpl(std::make_shared<detail::GrpcTransport_p>())
{
    // Enable gRPC tracing from the environment once, before any channel is created.
    static std::once_flag grpcTraceOnce;
    std::call_once(grpcTraceOnce,initGrpcTraceFromEnv);

    pimpl->transport=this;
    pimpl->thread=thread;
    pimpl->factory=factory;
}

//--------------------------------------------------------------------------

void GrpcTransport::setName(std::string name)
{
    pimpl->name=std::move(name);
}

//--------------------------------------------------------------------------

const std::string& GrpcTransport::name() const
{
    return pimpl->name;
}

//--------------------------------------------------------------------------

void GrpcTransport::setRouter(common::SharedPtr<Router> router)
{
    pimpl->router=std::move(router);
    if (pimpl->router)
    {
        HATN_CTX_DEBUG_RECORDS(1,"GrpcTransport::setRouter",{"insecure",pimpl->router->isInsecure()})
        initChannels();
    }
    else
    {
        closeChannels();
    }
}

//--------------------------------------------------------------------------

void GrpcTransport::closeChannels()
{
    for (auto&& it: pimpl->channels)
    {
        it.second.close();
    }
    pimpl->defaultChannel.close();
}

//--------------------------------------------------------------------------

void GrpcTransport::updateNetworkState(bool disconnected)
{
    for (auto&& it: pimpl->channels)
    {
        it.second.updateNetworkState(disconnected);
    }
    pimpl->defaultChannel.updateNetworkState(disconnected);
}

//--------------------------------------------------------------------------

void GrpcTransport::updateForegroundState()
{
    updateNetworkState(false);
}

//--------------------------------------------------------------------------

void GrpcTransport::reconnect()
{
    if (!pimpl->router || pimpl->router->hosts().empty())
    {
        return;
    }

    HATN_CTX_DEBUG(1,"GrpcTransport::reconnect: rebuilding channels after network medium change")

    std::string address=pimpl->router->hosts().at(0).asString();
    auto creds=buildRouterCreds(pimpl->router.get());

    auto serverName=pimpl->router->serverName();

    // Close and re-init each priority channel so zombie sockets are replaced immediately.
    for (auto&& it: pimpl->channels)
    {
        it.second.close();
        it.second.init(this,address,creds,name(),serverName);
    }
    pimpl->defaultChannel.close();
    pimpl->defaultChannel.init(this,address,creds,name(),serverName);
}

//--------------------------------------------------------------------------

common::SharedPtr<Router> GrpcTransport::router() const
{
    return pimpl->router;
}

//--------------------------------------------------------------------------

bool GrpcTransport::canSend(HATN_API_NAMESPACE::Priority p) const
{
    auto channel=pimpl->channel(p);

    {
        common::MutexScopedLock l{channel->mutex};
        return channel->pendingRequests.size() < config().fieldValue(grpc_config::maximum_concurrent_calls);
    }
}

//--------------------------------------------------------------------------

Error GrpcTransport::loadLogConfig(
    const HATN_BASE_NAMESPACE::ConfigTree& configTree,
    const std::string& configPath,
    HATN_BASE_NAMESPACE::config_object::LogRecords& records,
    const HATN_BASE_NAMESPACE::config_object::LogSettings& settings
    )
{
    if (pimpl->router)
    {
        HATN_CTX_WARN("GrpcTransport::loadLogConfig must be called before router is set")
    }
    base::ConfigTreePath p{configPath};
    p.append(ConfigSection);
    auto ec=base::ConfigObject<grpc_config::type>::loadLogConfig(configTree,p,records,settings);
    HATN_CHECK_EC(ec)

    if (name().empty())
    {
        setName(std::string{config().fieldValue(grpc_config::user_agent)});
    }

    return OK;
}

//--------------------------------------------------------------------------

void GrpcTransport::initChannels()
{
    // construct server address from router
    //! @todo Use own resolver, currently only one DNS name is supported
    std::string address;
    if (pimpl->router->hosts().empty())
    {
        return;
    }
    address=pimpl->router->hosts().at(0).asString();

    auto creds=buildRouterCreds(pimpl->router.get());
    if (pimpl->router->isInsecure())
    {
        HATN_CTX_DEBUG(1,"use insecure channel for gRPC transport")
    }
    else
    {
        HATN_CTX_DEBUG(1,"use TLS channel for gRPC transport")
    }

    auto serverName=pimpl->router->serverName();

    // init priority channels
    auto maxPriotity=static_cast<uint8_t>(api::Priority::Highest);
    for (size_t i=0;i<config().field(grpc_config::priority_channels).count();i++)
    {
        auto p=config().field(grpc_config::priority_channels).at(i);
        if (p<maxPriotity)
        {
            auto priority=static_cast<api::Priority>(p);
            auto it=pimpl->channels.emplace(std::piecewise_construct,std::forward_as_tuple(priority),
                                    std::forward_as_tuple());
            it.first->second.init(this,address,creds,name(),serverName);
        }
    }

    // init default priority channel
    pimpl->defaultChannel.init(this,address,creds,name(),serverName);
}

//--------------------------------------------------------------------------

void GrpcTransport::addMessageTypeMap(
        std::string pb,
        std::string du
    )
{
    pimpl->typeMap[std::move(pb)]=std::move(du);
}

//--------------------------------------------------------------------------

void detail::PriorityChannel::init(const GrpcTransport* cfg,
                                   const std::string& address,
                                   std::shared_ptr<grpc::ChannelCredentials> creds,
                                   const std::string& userAgent,
                                   const std::string& serverName)
{
    std::string configJson{cfg->config().fieldValue(grpc_config::config_json)};

    grpc::ChannelArguments args;
    args.SetUserAgentPrefix(userAgent);
    if (!configJson.empty())
    {
        args.SetServiceConfigJSON(configJson);
    }

    // Keepalive: send pings every keep_alive_period seconds to detect dead connections.
    args.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, cfg->config().fieldValue(grpc_config::keep_alive_period) * 1000);
    // Close if no ping response within keep_alive_timeout seconds.
    args.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, cfg->config().fieldValue(grpc_config::keep_alive_timeout) * 1000);
    // Allow pings even when there are no active calls (desktop only; mobile keeps false for battery).
    args.SetInt(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, cfg->config().fieldValue(grpc_config::keep_alive_without_calls) ? 1 : 0);
    // Allow unlimited pings on idle connections. gRPC default is 2, which stops pinging
    // after 2 unanswered pings and defeats the shorter keep_alive_period on idle channels.
    args.SetInt(GRPC_ARG_HTTP2_MAX_PINGS_WITHOUT_DATA, cfg->config().fieldValue(grpc_config::max_pings_without_data));
    // Cap reconnect backoff so the channel retries quickly after detecting a broken link.
    args.SetInt(GRPC_ARG_INITIAL_RECONNECT_BACKOFF_MS, cfg->config().fieldValue(grpc_config::initial_reconnect_backoff_ms));
    args.SetInt(GRPC_ARG_MAX_RECONNECT_BACKOFF_MS, cfg->config().fieldValue(grpc_config::max_reconnect_backoff_ms));

    if (!serverName.empty())
    {
        args.SetSslTargetNameOverride(serverName);
    }

    channel = grpc::CreateCustomChannel(address,creds,args);
    stub= std::make_shared<grpc::GenericStub>(channel);
}

//--------------------------------------------------------------------------

Result<clientapi::Response> detail::GrpcTransport_p::handleResponse(
        std::shared_ptr<grpc::ClientContext> context,
        grpc::Status status,
        const grpc::ByteBuffer& responseBuf,
        bool streamMessage,
        std::string messageType,
        common::ByteArrayShared messageData
    ) const
{
    clientapi::Response resp;
    std::string appStatus;
    std::string grpcCode;

    if (messageData.isNull())
    {
        messageData=common::makeShared<common::ByteArray>();
        std::vector<grpc::Slice> slices;
        if (responseBuf.Dump(&slices).ok())
        {
            // copy response data
            for (const auto& slice : slices)
            {
                messageData->append(reinterpret_cast<const char*>(slice.begin()),slice.size());
            }
        }
    }

    if (!streamMessage)
    {
        auto handleHeaders=[&,this](const std::multimap<grpc::string_ref, grpc::string_ref>& metadata, std::string /*type*/)
        {
            auto respType=findHeader(metadata,transport->config().fieldValue(grpc_config::message_type_header));
            if (!respType.empty())
            {
                auto mappedRespType=mapMessageType(respType);
                resp.setMessageType(mappedRespType);
            }
            auto id=findHeader(metadata,transport->config().fieldValue(grpc_config::id_header));
            if (!id.empty())
            {
                resp.setId(id);
            }
            auto status=findHeader(metadata,transport->config().fieldValue(grpc_config::status_header));
            if (!status.empty())
            {
                appStatus=status;
            }
            auto code=findHeader(metadata,transport->config().fieldValue(grpc_config::grpc_code_header));
            if (!code.empty())
            {
                grpcCode=code;
            }
#if 0
            // dump response headers
            std::cout << "------HEADERS " << type << "---------" << std::endl;
            for (auto it = metadata.begin(); it != metadata.end(); ++it) {
                // Convert string_ref to std::string for printing
                std::cout << std::string(it->first.data(), it->first.size()) << ": "
                          << std::string(it->second.data(), it->second.size()) << std::endl;
            }
            std::cout << "-----------------" << std::endl;
#endif
        };

        handleHeaders(context->GetServerInitialMetadata(),"initial");
        handleHeaders(context->GetServerTrailingMetadata(),"trailing");
    }
    else
    {
        resp.setMessageType(std::move(messageType));
    }
    resp.setMessageData(messageData);

    // check status
    if (status.ok())
    {
        resp.setStatus(api::protocol::ResponseStatus::Success);
    }
    else
    {
        auto code=status.error_code();
        if (!grpcCode.empty())
        {
            int tmp = 0;
            auto [ptr, ec] = std::from_chars(grpcCode.data(), grpcCode.data() + grpcCode.size(), tmp);
            if (ec == std::errc())
            {
                code=static_cast<grpc::StatusCode>(tmp);
            }
        }

        resp.setStatus(apiStatus(code));

        if (resp.status()==HATN_API_NAMESPACE::ApiResponseStatus::AuthError)
        {
            // auth error not API error, it can be processed by client session
            return resp;
        }

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

            return ec;
        }

        auto apiError=makeError(
            code,
            std::move(appStatus),
            transport->config(),
            context.get(),
            resp.messageType(),
            messageData
        );
        resp.setError(std::move(apiError));
    }

    return resp;
}

//--------------------------------------------------------------------------

void GrpcTransport::sendUnaryImpl(
        api::Priority priority,
        uintptr_t reqAddr,
        std::string method,
        std::vector<std::pair<std::string,std::string>> metadata,
        common::ByteArrayShared message,
        std::function<void(Result<clientapi::Response>)> onResponse
    )
{
    auto channel=pimpl->channel(priority);
    if (channel->isDisconnected())
    {
        HATN_CTX_SCOPE_ERROR("network is disconnected")
        onResponse(network::networkError(network::NetworkError::NETWORK_NOT_CONNECTED));
        return;
    }

    // create context and register pending request
    auto context=channel->addRequest(reqAddr);

    // setup deadline
    if (config().fieldValue(grpc_config::unary_deadline_timeout)!=0)
    {
        context->set_wait_for_ready(true);
        std::chrono::system_clock::time_point deadline =
            std::chrono::system_clock::now() + std::chrono::seconds(config().fieldValue(grpc_config::unary_deadline_timeout));
        context->set_deadline(deadline);
    }

    // add metadata to context
    for (const auto& h : metadata)
    {
        context->AddMetadata(h.first,h.second);
    }

    // prepare request and response buffers
    grpc::Slice slice(message->data(),message->size());
    grpc::ByteBuffer requestBuf(&slice,1);
    auto responseBuf=std::make_shared<grpc::ByteBuffer>();

    grpc::GenericStub* stub=channel->stub.get();
    grpc::StubOptions opt;

    // invoke unary call; the completion runs on a transport thread
    stub->UnaryCall(
        context.get(),
        method,
        opt,
        &requestBuf,
        responseBuf.get(),
        [pimpl=pimpl,priority,reqAddr,responseBuf,context,message,onResponse{std::move(onResponse)}](grpc::Status status) mutable
        {
            auto response=pimpl->handleResponse(
                context,
                status,
                *responseBuf
            );

            pimpl->channel(priority)->removeRequest(reqAddr);
            onResponse(std::move(response));
        }
    );
}

//--------------------------------------------------------------------------

void GrpcTransport::sendStreamImpl(
        api::Priority priority,
        uintptr_t reqAddr,
        std::string method,
        std::vector<std::pair<std::string,std::string>> metadata,
        common::ByteArrayShared message,
        clientapi::StreamChannel::ReadCb onMessage
    )
{
    auto channel=pimpl->channel(priority);
    if (channel->isDisconnected())
    {
        HATN_CTX_SCOPE_ERROR("network is disconnected")
        onMessage(network::networkError(network::NetworkError::NETWORK_NOT_CONNECTED),{});
        return;
    }

    // create stream (with its own context) and register it
    auto stream=channel->addStream(reqAddr,priority,pimpl);
    auto context=stream->context();

    // add metadata to context
    for (const auto& h : metadata)
    {
        context->AddMetadata(h.first,h.second);
    }

    // prepare request buffer
    grpc::Slice slice(message->data(),message->size());
    grpc::ByteBuffer requestBuf(&slice,1);

    grpc::GenericStub* stub=channel->stub.get();
    grpc::StubOptions opt;

    // wrap the grpc-free read callback so the pending request is dropped on error
    auto cb=[pimpl=pimpl,priority,reqAddr,onMessage{std::move(onMessage)}](const Error& ec, clientapi::Response response) mutable
    {
        if (ec)
        {
            pimpl->channel(priority)->removeRequest(reqAddr);
        }
        onMessage(ec,std::move(response));
    };

    stub->PrepareBidiStreamingCall(context.get(),method,opt,stream.get());
    stream->startStream(&requestBuf,std::move(cb));
}

//--------------------------------------------------------------------------

void GrpcTransport::cancelRequestImpl(api::Priority priority, uintptr_t reqAddr)
{
    pimpl->channel(priority)->cancelRequest(reqAddr);
}

//--------------------------------------------------------------------------

HATN_GRPCCLIENT_NAMESPACE_END
