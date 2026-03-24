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
#include <hatn/common/singleton.h>

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

constexpr const uint32_t DefaultUnaryDeadlineTimeout=25;
constexpr const uint32_t DefaultKeepAlivePeriod=60; // 60 seconds
constexpr const uint32_t DefaultKeepAliveTimeout=20; // 20 seconds

constexpr const char* DefaultConfigJson = R"({
  "methodConfig": [{
    "name": [{"service": ""}],
    "retryPolicy": {
      "maxAttempts": 5,
      "initialBackoff": "1s",
      "maxBackoff": "10s",
      "backoffMultiplier": 2,
      "retryableStatusCodes": ["UNAVAILABLE", "INTERNAL"]
    }
  }]
})";

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
    HDU_FIELD(error_response_type,TYPE_STRING,15,false,"grpc_api_server.Error")
    HDU_FIELD(keep_alive_period,TYPE_UINT32,16,false,DefaultKeepAlivePeriod)
    HDU_FIELD(keep_alive_timeout,TYPE_UINT32,17,false,DefaultKeepAliveTimeout)
    HDU_FIELD(keep_alive_without_calls,TYPE_BOOL,18,false,true)
    HDU_FIELD(grpc_code_header,TYPE_STRING,19,false,"x-grpc-code")
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

    private:

        void initChannels();
        void closeChannels();

        std::shared_ptr<detail::GrpcTransport_p> pimpl;
};

HATN_GRPCCLIENT_NAMESPACE_END

#endif // HATNGRPCTRANSPORT_H
