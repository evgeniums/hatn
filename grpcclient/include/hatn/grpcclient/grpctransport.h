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

#include <hatn/common/threadwithqueue.h>

#include <hatn/base/configobject.h>
#include <hatn/dataunit/unitwrapper.h>
#include <hatn/dataunit/syntax.h>

#include <hatn/logcontext/context.h>

#include <hatn/api/api.h>
#include <hatn/api/apiconstants.h>
#include <hatn/api/priority.h>
#include <hatn/api/client/defaulttraits.h>

#include <hatn/grpcclient/grpcclientdefs.h>
#include <hatn/grpcclient/grpcrouter.h>

HATN_API_NAMESPACE_BEGIN
class Tenancy;
HATN_API_NAMESPACE_END

HATN_GRPCCLIENT_NAMESPACE_BEGIN

namespace common=HATN_COMMON_NAMESPACE;

HDU_UNIT(grpc_transport_config,
    HDU_FIELD(maximum_concurrent_calls,TYPE_UINT32,1,false,100)
    HDU_REPEATED_FIELD(priority_channels,TYPE_UINT8,2)
)

namespace detail {
class GrpcTransport_p;
}

class HATN_GRPCCLIENT_EXPORT GrpcTransport : public base::ConfigObject<grpc_transport_config::type>
{
    public:

        constexpr static const char* const ConfigSection="grpc";

        GrpcTransport(
            common::SharedPtr<Router> router,
            common::ThreadQWithTaskContext* thread=common::ThreadQWithTaskContext::current(),
            const common::pmr::AllocatorFactory* /*factory*/=common::pmr::AllocatorFactory::getDefault()
        );

        GrpcTransport(
                common::SharedPtr<Router> router,
                const common::pmr::AllocatorFactory* factory
            ) : GrpcTransport(std::move(router),common::ThreadQWithTaskContext::current(),factory)
        {}

        GrpcTransport(
            common::ThreadQWithTaskContext* thread=common::ThreadQWithTaskContext::current(),
            const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
            ) : GrpcTransport({},thread,factory)
        {
        }

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
            if (callback)
            {
                callback({});
            }
        }

        template <typename RequestT>
        void cancelRequest(
            common::SharedPtr<RequestT> req
        );

        bool canSend(HATN_API_NAMESPACE::Priority p) const;

        void setName(std::string name);

        const std::string& name() const;

        void setRouter(common::SharedPtr<Router> Router);

        common::SharedPtr<Router> router() const;

    private:

        void initChannels();
        void closeChannels();

        std::unique_ptr<detail::GrpcTransport_p> pimpl;
};

#if 0
#include <grpcpp/generic/generic_stub.h>
#include <grpcpp/grpcpp.h>

void CallGenericWithCallback(std::shared_ptr<grpc::Channel> channel,
                             const std::string& method_path,
                             const std::string& serialized_request) {
    // 1. Create the GenericStub
    grpc::GenericStub stub(channel);

    // 2. Prepare the request buffer
    grpc::Slice slice(serialized_request.data(), serialized_request.size());
    grpc::ByteBuffer request_buf(&slice, 1);

    // 3. Prepare response containers
    auto* response_buf = new grpc::ByteBuffer();
    auto* status = new grpc::Status();
    auto* context = new grpc::ClientContext();

    // 4. Initiate the Unary Call with a lambda callback
    stub.unary()->UnaryCall(context, method_path, &request_buf, response_buf,
        [response_buf, status, context](grpc::Status s) {
            *status = s;
            if (status->ok()) {
                // Success: Process response_buf here
                std::cout << "RPC Success!" << std::endl;
            } else {
                std::cerr << "RPC Failed: " << status->error_message() << std::endl;
            }

            // Cleanup allocated resources in the callback
            delete response_buf;
            delete status;
            delete context;
        });
}

#include <grpcpp/grpcpp.h>
#include <fstream>
#include <string>

// Helper to read file content into a string
std::string read_file(const std::string& filename) {
    std::ifstream file(filename);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

void run_client() {
    // 1. Define custom certificate paths
    std::string ca_cert = read_file("ca.crt");      // Root CA to verify server
    std::string client_cert = read_file("client.crt"); // Client's public cert
    std::string client_key = read_file("client.key");   // Client's private key

    // 2. Configure SSL options
    grpc::SslCredentialsOptions ssl_opts;
    ssl_opts.pem_root_certs = ca_cert; // Set custom Root CA

    // Optional: Add client certs for mutual TLS
    grpc::SslCredentialsOptions::PemKeyCertPair pkcp;
    pkcp.cert_chain = client_cert;
    pkcp.private_key = client_key;
    ssl_opts.pem_key_cert_pairs.push_back(pkcp);

    // 3. Create secure channel credentials
    auto channel_creds = grpc::SslCredentials(ssl_opts);

    // 4. Create the channel and stub
    auto channel = grpc::CreateChannel("localhost:50051", channel_creds);
    // YourService::Stub stub(channel);
}


#endif

HATN_GRPCCLIENT_NAMESPACE_END

#endif // HATNGRPCTRANSPORT_H
