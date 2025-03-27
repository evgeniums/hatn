/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file mq/test/testproducer.cpp
  */

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include "hatn_test_config.h"
#include <hatn/test/multithreadfixture.h>

#include <hatn/logcontext/streamlogger.h>

#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>
#include <hatn/dataunit/ipp/objectid.ipp>

#include <hatn/api/api.h>

#include <hatn/api/client/plaintcpconnection.h>
#include <hatn/api/client/plaintcprouter.h>
#include <hatn/api/client/client.h>
#include <hatn/api/client/session.h>
#include <hatn/api/client/serviceclient.h>

#include <hatn/api/server/plaintcpserver.h>
#include <hatn/api/server/servicerouter.h>
#include <hatn/api/server/servicedispatcher.h>
#include <hatn/api/server/connectionsstore.h>
#include <hatn/api/server/server.h>
#include <hatn/api/server/env.h>

#include <hatn/api/ipp/client.ipp>
#include <hatn/api/ipp/clientrequest.ipp>
#include <hatn/api/ipp/auth.ipp>
#include <hatn/api/ipp/message.ipp>
#include <hatn/api/ipp/methodauth.ipp>
#include <hatn/api/ipp/serverresponse.ipp>
#include <hatn/api/ipp/serverservice.ipp>
#include <hatn/api/ipp/session.ipp>
#include <hatn/api/ipp/makeapierror.ipp>

#include <hatn/mq/producerclient.h>
#include <hatn/mq/producerservice.h>
#include <hatn/mq/apiclient.h>
#include <hatn/mq/scheduler.h>
#include <hatn/mq/notifier.h>

#include <hatn/mq/ipp/producerclient.ipp>

HATN_API_USING
HATN_COMMON_USING
HATN_LOGCONTEXT_USING
HATN_NETWORK_USING

namespace db=HATN_DB_NAMESPACE;
namespace mq=HATN_MQ_NAMESPACE;
namespace scheduler=HATN_SCHEDULER_NAMESPACE;

/********************** TestEnv **************************/

struct TestEnv : public HATN_TEST_NAMESPACE::MultiThreadFixture
{
    TestEnv()
    {
    }

    ~TestEnv()
    {
    }

    TestEnv(const TestEnv&)=delete;
    TestEnv(TestEnv&&) =delete;
    TestEnv& operator=(const TestEnv&)=delete;
    TestEnv& operator=(TestEnv&&) =delete;
};

constexpr const uint32_t TcpPort=53852;

using ClientType=client::Client<client::PlainTcpRouter,client::SessionWrapper<client::SessionNoAuthContext,client::SessionNoAuth>,LogCtxType>;
using ClientCtxType=client::ClientContext<ClientType>;
HATN_TASK_CONTEXT_DECLARE(ClientType)
HATN_TASK_CONTEXT_DEFINE(ClientType,ClientType)

using ClientWithAuthType=client::ClientWithAuth<client::SessionWrapper<client::SessionNoAuthContext,client::SessionNoAuth>,client::ClientContext<ClientType>,ClientType>;
using ClientWithAuthCtxType=client::ClientWithAuthContext<ClientWithAuthType>;

HATN_TASK_CONTEXT_DECLARE(ClientWithAuthType)
HATN_TASK_CONTEXT_DEFINE(ClientWithAuthType)

template <typename SessionWrapperT>
auto createClientWithAuth(SharedPtr<ClientCtxType> clientCtx, SessionWrapperT session)
{
    auto scl=client::makeClientWithAuthContext<ClientWithAuthCtxType>(std::move(session),std::move(clientCtx));
    return scl;
}

struct ContextBuilder
{
    auto makeContext() const
    {
        return makeLogCtx();
    }
};

class NotifierTraits
{
    public:

        template <typename ContextT, typename CallbackT, typename MessageT>
        void notify(
            SharedPtr<ContextT> ctx,
            CallbackT callback,
            lib::string_view topic,
            SharedPtr<MessageT> msg,
            mq::MessageStatus status,
            const std::string& errMsg
            )
        {
            if (status==mq::MessageStatus::Sent)
            {
                BOOST_TEST_MESSAGE(fmt::format("mq message was sent to topic \"{}\" pos {}",topic,msg->fieldValue(mq::message::pos).string()));
            }
            else
            {
                BOOST_TEST_MESSAGE(fmt::format("Failed to send mq message {} to topic \"{}\": {}",msg->fieldValue(mq::message::producer_pos).string(),topic,errMsg));
            }
            callback(std::move(ctx),Error{});
        }
};
using MqNotifier=mq::client::Notifier<NotifierTraits>;
using MqApiClient=mq::ApiClient<mq::ApiClientDefaultTraits<ClientWithAuthCtxType,ClientWithAuthType>>;
using MqProducerClientConfig=mq::client::ProducerClientConfig<MqApiClient,MqNotifier,scheduler::DefaultSchedulerConfig<ContextBuilder>>;
using MqProducerClient=mq::client::ProducerClient<MqProducerClientConfig>;
using MqClientScheduler=MqProducerClient::Scheduler;
using MqBackgroundWorker=MqClientScheduler::BackgroundWorker;

using WorkerV=scheduler::Worker<TaskLogContext>;
struct SchedulerWithWorkerConfig : public scheduler::DefaultSchedulerConfig<ContextBuilder>
{
    using Worker=WorkerV;
};
using MqProducerClientConfigW=mq::client::ProducerClientConfig<MqApiClient,MqNotifier,SchedulerWithWorkerConfig>;
using MqProducerClientW=mq::client::ProducerClient<MqProducerClientConfigW>;

HATN_TASK_CONTEXT_DECLARE(MqBackgroundWorker)
HATN_TASK_CONTEXT_DEFINE(MqBackgroundWorker,MqBackgroundWorker)

HATN_TASK_CONTEXT_DECLARE(MqClientScheduler)
HATN_TASK_CONTEXT_DEFINE(MqClientScheduler,MqClientScheduler)

HATN_TASK_CONTEXT_DECLARE(MqNotifier)
HATN_TASK_CONTEXT_DEFINE(MqNotifier,MqNotifier)

HATN_TASK_CONTEXT_DECLARE(MqApiClient)
HATN_TASK_CONTEXT_DEFINE(MqApiClient,MqApiClient)

HATN_TASK_CONTEXT_DECLARE(MqProducerClient)
HATN_TASK_CONTEXT_DEFINE(MqProducerClient,MqProducerClient)

HATN_TASK_CONTEXT_DECLARE(MqProducerClientW)
HATN_TASK_CONTEXT_DEFINE(MqProducerClientW,MqProducerClientW)

using MqProducerClientCtx=TaskContextType<MqProducerClient,MqApiClient,MqBackgroundWorker,MqNotifier,MqClientScheduler,LogContext>;

auto createClient(Thread* thread)
{
    auto resolver=std::make_shared<client::IpHostResolver>(thread);
    std::vector<client::IpHostName> hosts;
    hosts.resize(1);
    hosts[0].name="localhost";
    hosts[0].port=TcpPort;
    hosts[0].ipVersion=IpVersion::V4;

    auto router=makeShared<client::PlainTcpRouter>(
        hosts,
        resolver,
        thread
        );

    auto cfg=std::make_shared<client::ClientConfig>();

    auto cl=client::makeClientContext<client::ClientContext<ClientType>>(
        std::move(cfg),
        std::move(router),
        thread
        );
    return cl;
}

BOOST_AUTO_TEST_SUITE(TestProducer)

BOOST_AUTO_TEST_CASE(ConstructClient)
{
    MqProducerClient cl;
    MqProducerClientW clw;

    BOOST_CHECK(true);
}

BOOST_FIXTURE_TEST_CASE(ProducerClientCtx,TestEnv)
{
    createThreads(4);
    auto clientThread=thread(2);
    auto mqClientThread=thread(3);

    auto session=client::makeSessionNoAuthContext();
    auto rawClientCtx=createClient(clientThread.get());
    auto clientWithAuthCtx=createClientWithAuth(rawClientCtx,session);
    auto mappedClients=makeShared<MqApiClient::MappedClients>(clientWithAuthCtx);

    auto ctx=makeTaskContextType<MqProducerClientCtx>(
        subcontexts(
            subcontext("mq_producer1"),
            subcontext("mq_service1",mappedClients),
            subcontext(mqClientThread.get()),
            subcontext(),
            subcontext(),
            subcontext()
        )
    );

    BOOST_CHECK(static_cast<bool>(ctx));
}

BOOST_AUTO_TEST_SUITE_END()
