/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file grpcclientt/grpcstream.h
  *
  */

/****************************************************************************/

#ifndef HATNGRPCGRPCSTREAM_H
#define HATNGRPCGRPCSTREAM_H

#include <grpcpp/support/client_callback.h>

#include <hatn/common/locker.h>

#include <hatn/api/priority.h>
#include <hatn/api/client/streamchannel.h>
#include <hatn/grpcclient/grpcclientdefs.h>

HATN_GRPCCLIENT_NAMESPACE_BEGIN

namespace api=HATN_API_NAMESPACE;
namespace clientapi=HATN_API_NAMESPACE::client;

namespace detail {
class GrpcTransport_p;
}

class HATN_GRPCCLIENT_EXPORT GrpcStream : public grpc::ClientBidiReactor<grpc::ByteBuffer, grpc::ByteBuffer>,
                                          public clientapi::StreamChannel::StreamChannel,
                                          public std::enable_shared_from_this<GrpcStream>
{
    public:

        GrpcStream(
            std::shared_ptr<detail::GrpcTransport_p> transport,
            api::Priority channelPriority,
            std::shared_ptr<grpc::ClientContext> context
        );

        ~GrpcStream();

        GrpcStream(const GrpcStream&)=delete;
        GrpcStream(GrpcStream&&)=delete;
        GrpcStream& operator=(const GrpcStream&)=delete;
        GrpcStream& operator=(GrpcStream&&)=delete;

        void OnWriteDone(bool ok) override;

        void OnReadDone(bool ok) override;

        void OnDone(const grpc::Status& s) override;

        void readNext(ReadCb callback) override;
        void writeNext(common::ByteArrayShared message, std::string messageType, WriteCb callback) override;

        void close(clientapi::StreamChannel::CloseCb callback={}) override;

    private:

        ReadCb m_readCallback;
        WriteCb m_writeCallback;
        CloseCb m_closeCallback;

        std::shared_ptr<detail::GrpcTransport_p> m_transport;
        api::Priority m_channelPriority;
        std::shared_ptr<grpc::ClientContext> m_context;

        grpc::ByteBuffer m_responseBuffer;
        std::atomic<bool> m_closed;

        common::MutexLock m_mutex;
};

HATN_GRPCCLIENT_NAMESPACE_END

#endif // HATNGRPCGRPCSTREAM_H

#if 0
class InitReactor : public grpc::ClientBidiReactor<grpc::ByteBuffer, grpc::ByteBuffer> {
 public:
  InitReactor(grpc::ByteBuffer init_msg) : init_payload_(std::move(init_msg)) {
    // 1. Queue the first message (initialization)
    StartWrite(&init_payload_);

    // 2. Queue the first read to listen for server responses
    StartRead(&response_buffer_);

    // 3. IMPORTANT: Start the actual RPC call
    // Without this, the queued StartWrite/StartRead won't execute.
    StartCall();
  }

  void OnWriteDone(bool ok) override {
    if (!ok) return;
    // The initial message was sent successfully.
    // You can now send more messages if needed using StartWrite().
  }

  void OnReadDone(bool ok) override {
    if (ok) {
      // Process the server's response to your initialization
      // ...
      StartRead(&response_buffer_); // Continue reading the stream
    }
  }

  void OnDone(const grpc::Status& s) override {
    delete this;
  }

 private:
  grpc::ByteBuffer init_payload_;
  grpc::ByteBuffer response_buffer_;
};

grpc::GenericStub stub(channel);
grpc::ClientContext context;
std::string method_name = "/your.package.ServiceName/MethodName";

// Create the reactor instance
auto* reactor = new MyGenericReactor();

// Start the streaming call
stub.async()->PrepareBidiStreamingCall(&context, method_name, reactor);
#endif
