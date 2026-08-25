/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file grpcclient/grpctransport.h
  *
  */

/****************************************************************************/

#ifndef HATNGRPCTRANSPORT_H
#define HATNGRPCTRANSPORT_H

#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include <hatn/common/threadwithqueue.h>
#include <hatn/common/singleton.h>
#include <hatn/common/bytearray.h>

#include <hatn/base/configobject.h>
#include <hatn/dataunit/unitwrapper.h>
#include <hatn/dataunit/syntax.h>

#include <hatn/logcontext/context.h>

#include <hatn/api/api.h>
#include <hatn/api/apiconstants.h>
#include <hatn/api/priority.h>
#include <hatn/api/client/defaulttraits.h>
#include <hatn/api/client/clientresponse.h>
#include <hatn/api/client/streamchannel.h>

#include <hatn/grpcclient/grpcclientdefs.h>
#include <hatn/grpcclient/grpcrouter.h>

HATN_API_NAMESPACE_BEGIN
class Tenancy;
HATN_API_NAMESPACE_END

HATN_GRPCCLIENT_NAMESPACE_BEGIN

namespace common=HATN_COMMON_NAMESPACE;
namespace api=HATN_API_NAMESPACE;
namespace clientapi=HATN_API_NAMESPACE::client;

constexpr const uint32_t DefaultUnaryDeadlineTimeout=25;
// Ping-response timeout. Worst-case dead-socket detection = keep_alive_period +
// keep_alive_timeout. Shared by both platforms; tightening the timeout is nearly
// free since on a healthy link it only matters once a ping is already unanswered.
constexpr const uint32_t DefaultKeepAliveTimeout=5;

#if defined (BUILD_ANDROID) || defined (BUILD_IOS)

// Mobile: more conservative ping period — battery/radio wake.
// The open event-listener stream keeps pings alive, so withoutCalls=false is fine.
// Worst-case detection = 15 + 5 = 20 s.
constexpr const uint32_t DefaultKeepAlivePeriod=15;
constexpr const bool DefaultKeepAliveWithoutCalls=false;

constexpr const char* DefaultConfigJson = R"({
  "methodConfig": [
    {
      "name": [
        {
          "service": ""
        }
      ],
      "retryPolicy": {
        "maxAttempts": 5,
        "initialBackoff": "0.8s",
        "maxBackoff": "4s",
        "backoffMultiplier": 1.6,
        "retryableStatusCodes": [
          "UNAVAILABLE",
          "INTERNAL"
        ]
      },
      "waitForReady": true
    }
  ]
})";

#else

// Desktop: aggressive ping period for fast zombie-socket detection.
// Worst-case detection = 15 + 5 = 20 s.
constexpr const uint32_t DefaultKeepAlivePeriod=15;
constexpr const bool DefaultKeepAliveWithoutCalls = true;

constexpr const char* DefaultConfigJson = R"({
  "methodConfig": [
    {
      "name": [
        {
          "service": ""
        }
      ],
      "retryPolicy": {
        "maxAttempts": 4,
        "initialBackoff": "0.4s",
        "maxBackoff": "8s",
        "backoffMultiplier": 2.0,
        "retryableStatusCodes": [
          "UNAVAILABLE",
          "INTERNAL"
        ]
      },
      "waitForReady": true
    }
  ]
})";

#endif

// Allow unlimited keepalive pings on idle connections.
// gRPC default is 2, which stops pinging after 2 unanswered pings and defeats
// the shorter keep_alive_period on idle channels. 0 = no cap.
constexpr const uint32_t DefaultMaxPingsWithoutData=0;

// Cap reconnect backoff at 10 s instead of the gRPC default of 120 s so the
// channel retries quickly after a broken connection is detected.
constexpr const uint32_t DefaultMaxReconnectBackoffMs=10000;
constexpr const uint32_t DefaultInitialReconnectBackoffMs=1000;

// Minimum interval the client allows between keepalive pings sent on a connection
// with NO data frames flowing (e.g. a stuck stream after a WiFi<->VPN switch). This
// is exactly the situation of the event-listener stream: it half-closes its write
// side right after the subscribe message and never sends data again, so without
// this arg gRPC's own floor (tens of seconds, observed default ~300 s) overrides
// keep_alive_period and a zombie socket is only detected after that floor +
// keep_alive_timeout instead of keep_alive_period + keep_alive_timeout. Fed into
// GRPC_ARG_HTTP2_MIN_SENT_PING_INTERVAL_WITHOUT_DATA_MS in PriorityChannel::init().
// Keep at/below keep_alive_period so pings actually flow at the configured cadence.
constexpr const uint32_t DefaultMinSentPingIntervalWithoutDataMs=5000;

// Application-level stream heartbeat message type sent by the server inside
// stream_response wrappers on server-streaming calls. Liveness only: consumed and
// discarded by the app-level stream reader, never surfaced as a real event.
constexpr const char* StreamHeartbeatMessageType="hatn.stream.heartbeat";

// Heartbeat period (seconds) the client requests from the server for streaming
// calls, sent as the stream_heartbeat_header request metadata. The server sends
// heartbeats at min(its own configured period, this value), so this value alone
// bounds how stale a client-side stall watchdog can safely be. 0 disables the
// request entirely (client gets no heartbeats, e.g. talking to an older server).
constexpr const uint32_t DefaultStreamHeartbeatPeriod=20;

// Safety net for mobile backgrounding: if a channel somehow ends up with no active
// calls (e.g. the event-listener stream was closed) but the app-level suspend() was
// not invoked for some reason, gRPC's own idle-timeout will still drop the socket
// after this many ms of inactivity instead of leaving it (and its keepalive pings)
// alive indefinitely. 0 disables the arg entirely (desktop default). Set comfortably
// above keep_alive_period so it never fights normal keepalive traffic.
#if defined (BUILD_ANDROID) || defined (BUILD_IOS)
constexpr const uint32_t DefaultClientIdleTimeoutMs=60000;
#else
constexpr const uint32_t DefaultClientIdleTimeoutMs=0;
#endif

HDU_UNIT(grpc_config,
    HDU_FIELD(maximum_concurrent_calls,TYPE_UINT32,1,false,100)
    HDU_REPEATED_FIELD(priority_channels,TYPE_UINT8,2)
    HDU_FIELD(user_agent,TYPE_STRING,3,false,"hatngrpcclient")
    HDU_FIELD(status_header,TYPE_STRING,4,false,"x-hatn-status")
    HDU_FIELD(id_header,TYPE_STRING,5,false,"x-hatn-id")
    HDU_FIELD(message_type_header,TYPE_STRING,6,false,"x-hatn-mtype")
    HDU_FIELD(error_family_header,TYPE_STRING,7,false,"x-hatn-efamily")
    HDU_FIELD(error_description_header,TYPE_STRING,8,false,"x-hatn-edescription")
    HDU_FIELD(topic_header,TYPE_STRING,9,false,"x-hatn-topic")
    HDU_FIELD(tenancy_header,TYPE_STRING,10,false,"x-hatn-tenancy")
    HDU_FIELD(auth_tag_header,TYPE_STRING,11,false,"x-hatn-atag")
    HDU_FIELD(config_json,TYPE_STRING,12,false,DefaultConfigJson)
    HDU_FIELD(unary_deadline_timeout,TYPE_UINT32,13,false,DefaultUnaryDeadlineTimeout)
    HDU_FIELD(min_sent_ping_interval_without_data_ms,TYPE_UINT32,14,false,DefaultMinSentPingIntervalWithoutDataMs)
    HDU_FIELD(error_response_type,TYPE_STRING,15,false,"grpc_api_server.Error")
    HDU_FIELD(keep_alive_period,TYPE_UINT32,16,false,DefaultKeepAlivePeriod)
    HDU_FIELD(keep_alive_timeout,TYPE_UINT32,17,false,DefaultKeepAliveTimeout)
    HDU_FIELD(keep_alive_without_calls,TYPE_BOOL,18,false,DefaultKeepAliveWithoutCalls)
    HDU_FIELD(grpc_code_header,TYPE_STRING,19,false,"x-grpc-code")
    HDU_FIELD(send_id_header,TYPE_BOOL,20,false,true)
    HDU_FIELD(max_pings_without_data,TYPE_UINT32,21,false,DefaultMaxPingsWithoutData)
    HDU_FIELD(max_reconnect_backoff_ms,TYPE_UINT32,22,false,DefaultMaxReconnectBackoffMs)
    HDU_FIELD(initial_reconnect_backoff_ms,TYPE_UINT32,23,false,DefaultInitialReconnectBackoffMs)
    HDU_FIELD(client_idle_timeout_ms,TYPE_UINT32,24,false,DefaultClientIdleTimeoutMs)
    HDU_FIELD(stream_heartbeat_header,TYPE_STRING,25,false,"x-hatn-stream-hb")
    HDU_FIELD(stream_heartbeat_period,TYPE_UINT32,26,false,DefaultStreamHeartbeatPeriod)
    // See whitemdesktop/docs/error-contract.md. error_details_header existed on the wire (evgo
    // always sends x-hatn-edetails) but this config had no field name for it until now.
    HDU_FIELD(error_details_header,TYPE_STRING,27,false,"x-hatn-edetails")
    HDU_FIELD(error_disposition_header,TYPE_STRING,28,false,"x-hatn-edisposition")
    HDU_FIELD(error_retry_after_header,TYPE_STRING,29,false,"x-hatn-eretry-after")
)

namespace detail {
class GrpcTransport_p;
}

class HATN_GRPCCLIENT_EXPORT GrpcTransport : public base::ConfigObject<grpc_config::type>
{
    public:

        constexpr static const char* const ConfigSection="grpc";

        GrpcTransport(
            common::ThreadQWithTaskContext* thread=common::ThreadQWithTaskContext::current(),
            const common::pmr::AllocatorFactory* /*factory*/=common::pmr::AllocatorFactory::getDefault()
        );

        GrpcTransport(
                const common::pmr::AllocatorFactory* factory
            ) : GrpcTransport(common::ThreadQWithTaskContext::current(),factory)
        {}

        Error loadLogConfig(
            const HATN_BASE_NAMESPACE::ConfigTree& configTree,
            const std::string& configPath,
            HATN_BASE_NAMESPACE::config_object::LogRecords& records,
            const HATN_BASE_NAMESPACE::config_object::LogSettings& settings
        );

        template <typename RequestT>
        Error serializeRequest(
            common::SharedPtr<RequestT> req,
            lib::string_view topic,
            const HATN_API_NAMESPACE::Tenancy& tenancy
        )
        {
            req->setTopicAndTenancy(topic,tenancy);
            return OK;
        }

        template <typename RequestT>
        Error serializeRequest(
            common::SharedPtr<RequestT>
        )
        {
            return OK;
        }

        template <typename RequestT>
        Error parseResponse(
            common::SharedPtr<RequestT> req
        );

        template <typename RequestT, typename CallbackT>
        void sendRequest(
            common::SharedPtr<RequestT> req,
            CallbackT callback
        );

        template <typename ContextT, typename CallbackT>
        void close(
            common::SharedPtr<ContextT>,
            CallbackT callback
        )
        {
            closeChannels();
            callback({});
        }

        template <typename RequestT>
        void cancelRequest(
            common::SharedPtr<RequestT> req
        );

        bool canSend(HATN_API_NAMESPACE::Priority p) const;

        void setName(lib::string_view name)
        {
            setName(std::string{name});
        }

        void setName(std::string name);

        const std::string& name() const;

        void setRouter(common::SharedPtr<Router> Router);

        common::SharedPtr<Router> router() const;

        void addMessageTypeMap(
            std::string pb,
            std::string du
        );

        void updateNetworkState(bool disconnected);

        void updateForegroundState();

        // Hard reconnect: tears down all channels (cancelling in-flight RPCs/streams) and
        // re-creates them on the current network interface. Call when the transport medium
        // changes (WiFi <-> cellular) so zombie sockets are replaced immediately instead of
        // waiting for keepalive timeout to fire.
        void reconnect();

        // Mobile background/foreground lifecycle: unlike updateNetworkState(true), which only
        // cancels in-flight RPCs/streams and marks the channel disconnected while leaving the
        // underlying grpc::Channel/stub (and its socket + keepalive pings) alive, suspend()
        // destroys the channel objects outright (same effect as closeChannels()), which is the
        // only way to actually drop the TCP socket. resume() rebuilds them (same as
        // initChannels()), symmetric with setRouter()'s init/close pairing. Call suspend() when
        // the app enters background and resume() when it returns to foreground.
        void suspend();
        void resume();

        // ---- gRPC-encapsulation boundary -----------------------------------
        // These non-template methods own every gRPC type and are compiled only
        // into hatngrpcclient.dll (exported via the class-level export macro).
        // The header template sendRequest()/cancelRequest() (see grpctransport.ipp)
        // extract grpc-free data from the request and call into these, so gRPC,
        // abseil and protobuf never leak into consumer modules. reqAddr is
        // reinterpret_cast<uintptr_t>(req.get()) and keys the pending-request /
        // stream registries. onResponse/onMessage are constructed in the caller
        // (capturing req + the user callback) and run on the transport thread.

        void sendUnaryImpl(
            api::Priority priority,
            uintptr_t reqAddr,
            std::string method,
            std::vector<std::pair<std::string,std::string>> metadata,
            common::ByteArrayShared message,
            std::function<void(Result<clientapi::Response>)> onResponse
        );

        void sendStreamImpl(
            api::Priority priority,
            uintptr_t reqAddr,
            std::string method,
            std::vector<std::pair<std::string,std::string>> metadata,
            common::ByteArrayShared message,
            clientapi::StreamChannel::ReadCb onMessage
        );

        void cancelRequestImpl(
            api::Priority priority,
            uintptr_t reqAddr
        );

    private:

        // Creates the per-priority PriorityChannel map entries (structure only, no gRPC
        // channel objects yet). Called once from setRouter(), before any traffic exists.
        // Must never be called again afterwards: the map is read without synchronization
        // from gRPC reactor/callback threads (see GrpcTransport_p::channel()), so mutating
        // its structure (e.g. from resume()) concurrently with those reads is undefined
        // behavior. Existing PriorityChannel entries are never erased, only closed, so
        // "populate once, then only touch existing entries" is sufficient.
        void populateChannels();
        void initChannels();
        void closeChannels();

        std::shared_ptr<detail::GrpcTransport_p> pimpl;
};

HATN_GRPCCLIENT_NAMESPACE_END

#endif // HATNGRPCTRANSPORT_H
