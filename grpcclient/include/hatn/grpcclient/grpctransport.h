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
#include <hatn/api/responseunit.h>
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
    HDU_FIELD(user_agent,TYPE_STRING,3,false,"hatngrpcclient")
)

namespace detail {
class GrpcTransport_p;
}

class HATN_GRPCCLIENT_EXPORT GrpcTransport : public base::ConfigObject<grpc_transport_config::type>
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
        common::Result<common::SharedPtr<HATN_API_NAMESPACE::ResponseManaged>> parseResponse(
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

HATN_GRPCCLIENT_NAMESPACE_END

#endif // HATNGRPCTRANSPORT_H
