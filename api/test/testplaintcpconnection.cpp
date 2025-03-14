/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/test/testsendrecv.cpp
  */

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include "hatn_test_config.h"
#include <hatn/test/multithreadfixture.h>

#include <hatn/common/random.h>

#include <hatn/network/asio/caresolver.h>

#include <hatn/dataunit/ipp/syntax.ipp>

#include <hatn/api/api.h>
#include <hatn/api/client/plaintcpconnection.h>
#include <hatn/api/server/plaintcpserver.h>

// HATN_API_USING
// HATN_COMMON_USING
// HATN_LOGCONTEXT_USING

namespace {

struct Env : public ::hatn::test::MultiThreadFixture
{
    Env()
    {
        auto ec=HATN_NETWORK_NAMESPACE::CaresLib::init();
        if (ec)
        {
            BOOST_TEST_MESSAGE(fmt::format("cares lib init error: {}",ec.message()));
        }
        BOOST_REQUIRE(!ec);
    }

    ~Env()
    {
        HATN_NETWORK_NAMESPACE::CaresLib::cleanup();
    }

    Env(const Env&)=delete;
    Env(Env&&) =delete;
    Env& operator=(const Env&)=delete;
    Env& operator=(Env&&) =delete;
};
}

constexpr const uint32_t TcpPort=53852;

auto createClient(HATN_COMMON_NAMESPACE::Thread* thread)
{
    auto resolver=std::make_shared<HATN_API_NAMESPACE::client::IpHostResolver>(thread);
    std::vector<HATN_API_NAMESPACE::client::IpHostName> hosts;
    hosts.resize(1);
    hosts[0].name="localhost";
    hosts[0].port=TcpPort;
    hosts[0].ipVersion=HATN_NETWORK_NAMESPACE::IpVersion::V4;

    auto connectionCtx=HATN_API_NAMESPACE::client::makePlainTcpConnectionContext(
        hosts,
        resolver,
        thread,
        "client"
    );

    return connectionCtx;
}

auto createServer(HATN_COMMON_NAMESPACE::ThreadQWithTaskContext* thread)
{
    auto tcpServerCtx=HATN_API_NAMESPACE::server::makePlainTcpServerContext(thread,"server");
    auto serverEnv=HATN_COMMON_NAMESPACE::makeEnvType<HATN_API_NAMESPACE::server::BasicEnv>(
        HATN_COMMON_NAMESPACE::contexts(
                HATN_COMMON_NAMESPACE::context(thread),
                HATN_COMMON_NAMESPACE::context(),
                HATN_COMMON_NAMESPACE::context()
            )
        );
    serverEnv->get<HATN_COMMON_NAMESPACE::WithMappedThreads>().threads()->setDefaultThread(thread);
    auto& tcpServer=tcpServerCtx->get<HATN_API_NAMESPACE::server::PlainTcpServer>();
    tcpServer.setEnv(serverEnv);
    return tcpServerCtx;
}

BOOST_AUTO_TEST_SUITE(TestPlainTcpConnection)

BOOST_FIXTURE_TEST_CASE(CreateClient,Env)
{
    createThreads(2);
    auto workThread=thread(1);

    std::ignore=createClient(workThread.get());

    BOOST_CHECK(true);
}

BOOST_FIXTURE_TEST_CASE(CreateServer,Env)
{
    createThreads(2);
    auto workThread=threadWithContextTask(1);

    std::ignore=createServer(workThread.get());

    BOOST_CHECK(true);
}

BOOST_FIXTURE_TEST_CASE(ConnectClientServer,Env)
{
    createThreads(2);
    auto serverThread=threadWithContextTask(0);
    auto clientThread=threadWithContextTask(1);

    auto serverCtx=createServer(serverThread.get());
    auto clientCtx=createClient(clientThread.get());

    serverThread->start();
    clientThread->start();

    std::atomic<size_t> clientConnectCount{0};
    std::atomic<size_t> serverConnectCount{0};

    auto serverCb=[&serverConnectCount](HATN_COMMON_NAMESPACE::SharedPtr<HATN_API_NAMESPACE::server::PlainTcpConnectionContext> ctx, const HATN_NAMESPACE::Error& ec)
    {
        HATN_TEST_MESSAGE_TS(fmt::format("serverCb: {}/{}",ec.code(),ec.message()));
        if (!ec)
        {
            serverConnectCount++;
        }
    };

    auto& server=serverCtx->get<HATN_API_NAMESPACE::server::PlainTcpServer>();
    server.setConnectionHandler(serverCb);
    HATN_NETWORK_NAMESPACE::asio::IpEndpoint serverEp{"127.0.0.1",TcpPort};
    auto ec=server.run(serverCtx,serverEp);
    if (ec)
    {
        BOOST_TEST_MESSAGE(fmt::format("failed to listen: {}/{}",ec.code(),ec.message()));
    }
    BOOST_REQUIRE(!ec);

    auto clientCb=[&clientConnectCount](const HATN_NAMESPACE::Error& ec)
    {
        HATN_TEST_MESSAGE_TS(fmt::format("clientCb: {}/{}",ec.code(),ec.message()));

        if (!ec)
        {
            clientConnectCount++;
        }
        else
        {
            HATN_CHECK_TS(false);
        }
    };
    auto& client=clientCtx->get<HATN_API_NAMESPACE::client::PlainTcpConnection>();
    client.connect(clientCb);

    auto clientCtx2=createClient(clientThread.get());
    auto& client2=clientCtx2->get<HATN_API_NAMESPACE::client::PlainTcpConnection>();
    client2.connect(clientCb);

    exec(1);

    std::ignore=server.close();

    serverThread->stop();
    clientThread->stop();

    exec(1);

    BOOST_CHECK_EQUAL(serverConnectCount,2);
    BOOST_CHECK_EQUAL(clientConnectCount,2);
}

using TestBuffersSet=std::vector<std::vector<HATN_COMMON_NAMESPACE::ByteArray>>;

size_t totalSentBytes=0;

template <typename ConnectionT, typename ContextT, typename CallbackT>
void sendNext(ContextT ctx, const TestBuffersSet& bufferSet, size_t idx, CallbackT cb)
{
    if (idx==bufferSet.size())
    {
        cb(HATN_NAMESPACE::Error{});
        return;
    }

    auto& buf=bufferSet[idx];
    auto spanBuffers=HATN_COMMON_NAMESPACE::spanBuffers(buf);

    auto& connection=ctx->template get<ConnectionT>();
    connection.send(
        ctx,
        spanBuffers,
        [idx,&bufferSet,cb{std::move(cb)}](ContextT ctx, const HATN_NAMESPACE::Error& ec, size_t sentBytes, HATN_COMMON_NAMESPACE::SpanBuffers)
        {
            totalSentBytes+=sentBytes;
            HATN_TEST_MESSAGE_TS(fmt::format("sendNext cb {} of size {}, total {}: {}/{}",idx,sentBytes,totalSentBytes,ec.code(),ec.message()));

            if (ec)
            {
                cb(ec);
                return;
            }

            sendNext<ConnectionT>(std::move(ctx),bufferSet,idx+1,std::move(cb));
        }
    );
}

size_t totalreadBytes=0;

template <typename ConnectionT, typename ContextT, typename CallbackT>
void readNext(ContextT ctx, HATN_COMMON_NAMESPACE::ByteArray& rxBuf, HATN_COMMON_NAMESPACE::ByteArray& tmpBuf, CallbackT cb, size_t dataSize)
{
    auto& connection=ctx->template get<ConnectionT>();
    connection.read(
        ctx,
        tmpBuf.data(),
        tmpBuf.size(),
        [dataSize,cb{std::move(cb)},&tmpBuf,&rxBuf](ContextT ctx, const HATN_NAMESPACE::Error& ec, size_t readBytes)
        {
            // totalreadBytes+=readBytes;
            // HATN_TEST_MESSAGE_TS(fmt::format("readNext cb {}, total {}/{}: {}/{}",readBytes,totalreadBytes,dataSize,ec.code(),ec.message()));
            rxBuf.append(tmpBuf.data(),readBytes);

            if (ec)
            {
                cb(ec);
                return;
            }

            if (rxBuf.size()==dataSize)
            {
                cb(HATN_NAMESPACE::Error{});
            }

            readNext<ConnectionT>(std::move(ctx),rxBuf,tmpBuf,std::move(cb),dataSize);
        }
    );
}

template <typename ConnectionT, typename ContextT, typename CallbackT>
void recvNext(ContextT ctx, HATN_COMMON_NAMESPACE::ByteArray& rxBuf, HATN_COMMON_NAMESPACE::ByteArray& tmpBuf, CallbackT cb, size_t dataSize,
              const TestBuffersSet& bufferSet, size_t bufsIdx)
{
    if (bufsIdx==bufferSet.size())
    {
        HATN_CHECK_TS(false);
        cb(HATN_NAMESPACE::Error{});
        return;
    }

    HATN_COMMON_NAMESPACE::ByteArray testBuf;
    const auto& bufs=bufferSet[bufsIdx];
    for (auto&& it: bufs)
    {
        testBuf.append(it);
    }
    tmpBuf.resize(testBuf.size());
    tmpBuf.fill(0);

    auto& connection=ctx->template get<ConnectionT>();
    connection.recv(
        ctx,
        tmpBuf.data(),
        tmpBuf.size(),
        [testBuf{std::move(testBuf)},&bufferSet,bufsIdx,dataSize,cb{std::move(cb)},&tmpBuf,&rxBuf](ContextT ctx, const HATN_NAMESPACE::Error& ec)
        {
            if (ec)
            {
                cb(ec);
                return;
            }

            totalreadBytes+=testBuf.size();
            HATN_TEST_MESSAGE_TS(fmt::format("recvNext cb {}, total {}/{}: {}/{}",testBuf.size(),totalreadBytes,dataSize,ec.code(),ec.message()));

            HATN_CHECK_TS(testBuf==tmpBuf);
            rxBuf.append(tmpBuf);

            if (rxBuf.size()==dataSize)
            {
                cb(HATN_NAMESPACE::Error{});
                return;
            }

            recvNext<ConnectionT>(std::move(ctx),rxBuf,tmpBuf,std::move(cb),dataSize,bufferSet,bufsIdx+1);
        }
        );
}

auto genTestBuffersSet(size_t setCount, size_t minBufs, size_t maxBufs, size_t minBufSize, size_t maxBufSize)
{
    TestBuffersSet bufsSet;
    HATN_COMMON_NAMESPACE::ByteArray totalBuf;
    bufsSet.resize(setCount);
    for (size_t i=0;i<setCount;i++)
    {
        auto& bufs=bufsSet[i];

        auto bufCount=HATN_COMMON_NAMESPACE::Random::uniform(1,8);
        bufs.resize(bufCount);
        size_t buffersSize=0;
        for (size_t j=0;j<bufCount;j++)
        {
            auto& buf=bufs[j];
            HATN_COMMON_NAMESPACE::Random::randContainer(buf,maxBufSize,minBufSize);
            BOOST_TEST_MESSAGE(fmt::format("Gen send buf {}/{} of size {}",i,j,buf.size()));
            buffersSize+=buf.size();
            totalBuf.append(buf);
        }
        BOOST_TEST_MESSAGE(fmt::format("Gen send buffers {} of size {}",i,buffersSize));
    }

    BOOST_TEST_MESSAGE(fmt::format("Gen send buffers total size {}",totalBuf.size()));

    return std::make_pair(bufsSet,totalBuf);
}

template <typename MakeServerT, typename MakeClientT>
void sendRead(Env* env, MakeServerT makeServer, MakeClientT makeClient, bool sendServerToClient, bool recv=false)
{
    env->createThreads(2);
    auto serverThread=env->threadWithContextTask(0);
    auto clientThread=env->threadWithContextTask(1);

    auto serverCtx=createServer(serverThread.get());
    auto clientCtx=createClient(clientThread.get());

    serverThread->start();
    clientThread->start();

    size_t serverBufferSetCount=4;
    size_t minBufSize=1;
    size_t maxBufSize=1*1024*1024;
    size_t minBufsCount=1;
    size_t maxBufsCount=4;
    size_t rxTmpBufSize=64*1024;

    auto sendBuffers=genTestBuffersSet(serverBufferSetCount,minBufsCount,maxBufsCount,minBufSize,maxBufSize);
    HATN_COMMON_NAMESPACE::ByteArray tmpBuf;
    tmpBuf.resize(rxTmpBufSize);
    HATN_COMMON_NAMESPACE::ByteArray rxBuf;

    bool serverConnectionClosed=false;
    auto closeServerConnection=[&serverConnectionClosed](HATN_COMMON_NAMESPACE::SharedPtr<HATN_API_NAMESPACE::server::PlainTcpConnectionContext> ctx, std::function<void()> cb)
    {
        serverConnectionClosed=true;
        auto& connection=ctx->get<HATN_API_NAMESPACE::server::PlainTcpConnection>();
        auto cb1=[ctx,cb](const HATN_NAMESPACE::Error& ec)
        {
            std::ignore=ctx;
            HATN_CHECK_TS(!ec);
            cb();
        };
        connection.close(cb1);
    };

    auto serverReadCb=[env,&serverConnectionClosed,&closeServerConnection](HATN_COMMON_NAMESPACE::SharedPtr<HATN_API_NAMESPACE::server::PlainTcpConnectionContext> ctx,const HATN_NAMESPACE::Error& ec)
    {
        HATN_TEST_MESSAGE_TS(fmt::format("serverReadCb: {}/{}",ec.code(),ec.message()));
        if (!serverConnectionClosed)
        {
            closeServerConnection(ctx,[env](){env->quit();});
        }
    };
    auto serverSendCb=[](const HATN_NAMESPACE::Error& ec)
    {
        HATN_TEST_MESSAGE_TS(fmt::format("serverSendCb: {}/{}",ec.code(),ec.message()));
        HATN_CHECK_TS(!ec);
    };
    HATN_COMMON_NAMESPACE::SharedPtr<HATN_API_NAMESPACE::server::PlainTcpConnectionContext> serverConnectionCtx;;
    auto serverConnectCb=[recv,sendServerToClient,&serverConnectionCtx,&serverConnectionClosed,&serverSendCb,&closeServerConnection,&sendBuffers,&serverReadCb,&rxBuf,&tmpBuf](HATN_COMMON_NAMESPACE::SharedPtr<HATN_API_NAMESPACE::server::PlainTcpConnectionContext> ctx, const HATN_NAMESPACE::Error& ec)
    {
        HATN_TEST_MESSAGE_TS(fmt::format("serverCb: {}/{}",ec.code(),ec.message()));
        serverConnectionCtx=ctx;

        if (!serverConnectionClosed)
        {
            HATN_REQUIRE_TS(!ec);

            if (sendServerToClient)
            {
                auto cb=[ctx,&serverSendCb,&closeServerConnection](const HATN_NAMESPACE::Error& ec)
                {
                    serverSendCb(ec);
                    closeServerConnection(ctx,[](){});
                };
                sendNext<HATN_API_NAMESPACE::server::PlainTcpConnection>(ctx,sendBuffers.first,0,cb);
            }
            else
            {
                if (recv)
                {
                    recvNext<HATN_API_NAMESPACE::server::PlainTcpConnection>(ctx,rxBuf,tmpBuf,
                                                                             [ctx,&serverReadCb](const HATN_NAMESPACE::Error& ec){
                                                                                 serverReadCb(ctx,ec);
                                                                             },
                                                                             sendBuffers.second.size(),
                                                                             sendBuffers.first,
                                                                             0
                                                                            );
                }
                else
                {
                    readNext<HATN_API_NAMESPACE::server::PlainTcpConnection>(ctx,rxBuf,tmpBuf,
                                                                             [ctx,&serverReadCb](const HATN_NAMESPACE::Error& ec){
                                                                                 serverReadCb(ctx,ec);
                                                                             },
                                                                             sendBuffers.second.size());
                }
            }
        }
    };

    auto& server=serverCtx->get<HATN_API_NAMESPACE::server::PlainTcpServer>();
    server.setConnectionHandler(serverConnectCb);
    HATN_NETWORK_NAMESPACE::asio::IpEndpoint serverEp{"127.0.0.1",TcpPort};
    auto ec=server.run(serverCtx,serverEp);
    if (ec)
    {
        BOOST_TEST_MESSAGE(fmt::format("failed to listen: {}/{}",ec.code(),ec.message()));
    }
    BOOST_REQUIRE(!ec);

    bool clientConnectionClosed=false;
    auto closeClientConnection=[&clientConnectionClosed](HATN_COMMON_NAMESPACE::SharedPtr<HATN_API_NAMESPACE::client::PlainTcpConnectionContext> ctx, std::function<void()> cb)
    {
        clientConnectionClosed=true;
        auto& connection=ctx->get<HATN_API_NAMESPACE::client::PlainTcpConnection>();
        auto cb1=[ctx,cb](const HATN_NAMESPACE::Error& ec)
        {
            std::ignore=ctx;
            HATN_CHECK_TS(!ec);
            cb();
        };
        connection.close(cb1);
    };
    auto clientSendCb=[](const HATN_NAMESPACE::Error& ec)
    {
        HATN_TEST_MESSAGE_TS(fmt::format("clientSendCb: {}/{}",ec.code(),ec.message()));
        HATN_CHECK_TS(!ec);
    };
    auto clientReadCb=[&clientConnectionClosed,&closeClientConnection,clientCtx,env](const HATN_NAMESPACE::Error& ec)
    {
        HATN_TEST_MESSAGE_TS(fmt::format("clientReadCb: {}/{}",ec.code(),ec.message()));
        if (!clientConnectionClosed)
        {
            closeClientConnection(clientCtx,[env](){env->quit();});
        }
    };
    auto clientConnectCb=[recv,sendServerToClient,&closeClientConnection,clientCtx,&sendBuffers,&clientSendCb,&rxBuf,&tmpBuf,&clientReadCb](const HATN_NAMESPACE::Error& ec)
    {
        HATN_TEST_MESSAGE_TS(fmt::format("clientConnectCb: {}/{}",ec.code(),ec.message()));
        HATN_REQUIRE_TS(!ec);

        if (!sendServerToClient)
        {
            auto cb=[clientCtx,&clientSendCb,&closeClientConnection](const HATN_NAMESPACE::Error& ec)
            {
                clientSendCb(ec);
                closeClientConnection(clientCtx,[](){});
            };
            sendNext<HATN_API_NAMESPACE::client::PlainTcpConnection>(clientCtx,sendBuffers.first,0,cb);
        }
        else
        {
            if (recv)
            {
                recvNext<HATN_API_NAMESPACE::client::PlainTcpConnection>(clientCtx,rxBuf,tmpBuf,clientReadCb,sendBuffers.second.size(),
                                                                         sendBuffers.first,
                                                                         0
                                                                        );
            }
            else
            {
                readNext<HATN_API_NAMESPACE::client::PlainTcpConnection>(clientCtx,rxBuf,tmpBuf,clientReadCb,sendBuffers.second.size());
            }
        }
    };
    auto& client=clientCtx->get<HATN_API_NAMESPACE::client::PlainTcpConnection>();
    client.connect(clientConnectCb);

    env->exec(10);

    serverConnectionClosed=true;
    clientConnectionClosed=true;
    std::ignore=server.close();
    client.close();

    serverThread->stop();
    clientThread->stop();

    env->exec(1);

    BOOST_CHECK_EQUAL(sendBuffers.second.size(),rxBuf.size());
    BOOST_CHECK(sendBuffers.second==rxBuf);
}

BOOST_FIXTURE_TEST_CASE(ClientSendServerRead,Env)
{
    sendRead(this,createServer,createClient,false);
}

BOOST_FIXTURE_TEST_CASE(ServerSendClientRead,Env)
{
    sendRead(this,createServer,createClient,true);
}

BOOST_FIXTURE_TEST_CASE(ClientSendServerRecv,Env)
{
    sendRead(this,createServer,createClient,false,true);
}

BOOST_FIXTURE_TEST_CASE(ServerSendClientRecv,Env)
{
    sendRead(this,createServer,createClient,true,true);
}

BOOST_AUTO_TEST_SUITE_END()
