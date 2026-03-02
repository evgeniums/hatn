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

#include <grpcpp/grpcpp.h>
#include <grpcpp/generic/generic_stub.h>

#include <hatn/common/meta/enumint.h>
#include <hatn/grpcclient/grpctransport.h>
#include <hatn/grpcclient/ipp/grpctransport.ipp>

#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>

HATN_GRPCCLIENT_NAMESPACE_BEGIN

namespace api=HATN_API_NAMESPACE;


//--------------------------------------------------------------------------

GrpcTransport::GrpcTransport(
    common::ThreadQWithTaskContext* thread,
    const common::pmr::AllocatorFactory* factory
    ) : pimpl(std::make_unique<detail::GrpcTransport_p>())
{
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

    // construct grpc channel credentials
    std::shared_ptr<grpc::ChannelCredentials> creds;
    if (pimpl->router->isInsecure())
    {
        creds=grpc::InsecureChannelCredentials();
    }
    else
    {
        grpc::SslCredentialsOptions sslOpts;
        sslOpts.pem_root_certs = pimpl->router->serverCerts();

        if (!pimpl->router->clientPrivKey().empty())
        {
            sslOpts.pem_cert_chain = pimpl->router->clientCertChain();
            sslOpts.pem_private_key = pimpl->router->clientPrivKey();
        }

        creds=grpc::SslCredentials(sslOpts);
    }

    std::string configJson{config().fieldValue(grpc_config::config_json)};

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
            it.first->second.init(address,creds,name(),configJson);
        }
    }

    // init default priority channel
    pimpl->defaultChannel.init(address,creds,name(),configJson);
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

void detail::PriorityChannel::init(const std::string& address,
                                   std::shared_ptr<grpc::ChannelCredentials> creds,
                                   const std::string& userAgent,
                                   const std::string& configJson)
{
    grpc::ChannelArguments args;
    args.SetUserAgentPrefix(userAgent);    
    args.SetServiceConfigJSON(configJson);

    channel = grpc::CreateCustomChannel(address,creds,args);
    stub= std::make_shared<grpc::GenericStub>(channel);
}

//--------------------------------------------------------------------------

HATN_GRPCCLIENT_NAMESPACE_END
