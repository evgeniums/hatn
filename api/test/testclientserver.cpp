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

#include <hatn/dataunit/ipp/syntax.ipp>

#include <hatn/api/api.h>

#include <hatn/api/client/plaintcpconnection.h>
#include <hatn/api/client/plaintcprouter.h>
#include <hatn/api/server/plaintcpserver.h>

#include <hatn/api/client/client.h>
#include <hatn/api/client/session.h>

#include <hatn/api/ipp/client.ipp>
#include <hatn/api/ipp/request.ipp>
#include <hatn/api/ipp/auth.ipp>
#include <hatn/api/ipp/message.ipp>
#include <hatn/api/ipp/methodauth.ipp>

HATN_API_USING
HATN_COMMON_USING
HATN_LOGCONTEXT_USING
HATN_NETWORK_USING

namespace {

struct TestEnv : public ::hatn::test::MultiThreadFixture
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
}

constexpr const uint32_t TcpPort=53852;

using ClientType=client::Client<client::PlainTcpRouter,client::SessionNoAuth,LogCtxType>;
HATN_TASK_CONTEXT_DECLARE(ClientType)
HATN_TASK_CONTEXT_DEFINE(ClientType,ClientType)

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

#if 0

auto createServer(Thread* thread)
{
    return server::makePlainTcpServerContext(thread,"server");
}

BOOST_AUTO_TEST_SUITE(TestClientServer)

BOOST_FIXTURE_TEST_CASE(CreateClient,TestEnv)
{
    createThreads(2);
    auto workThread=thread(1);

    std::ignore=createClient(workThread.get());

    BOOST_CHECK(true);
}

BOOST_FIXTURE_TEST_CASE(CreateServer,TestEnv)
{
    createThreads(2);
    auto workThread=thread(1);

    std::ignore=createServer(workThread.get());

    BOOST_CHECK(true);
}

BOOST_FIXTURE_TEST_CASE(ConnectClientServer,TestEnv)
{
    createThreads(2);
    auto serverThread=thread(0);
    auto clientThread=thread(1);

    auto serverCtx=createServer(serverThread.get());
    auto clientCtx=createClient(clientThread.get());

    serverThread->start();
    clientThread->start();

    std::atomic<size_t> clientConnectCount{0};
    std::atomic<size_t> serverConnectCount{0};

    auto serverCb=[&serverConnectCount](SharedPtr<server::PlainTcpConnectionContext> ctx, const HATN_NAMESPACE::Error& ec)
    {
        HATN_TEST_MESSAGE_TS(fmt::format("serverCb: {}/{}",ec.code(),ec.message()));
        if (!ec)
        {
            serverConnectCount++;
        }
    };

    auto& server=serverCtx->get<server::PlainTcpServer>();
    server.setConnectionHandler(serverCb);
    asio::IpEndpoint serverEp{"127.0.0.1",TcpPort};
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
    auto& client=clientCtx->get<client::PlainTcpConnection>();
    client.connect(clientCb);

    auto clientCtx2=createClient(clientThread.get());
    auto& client2=clientCtx2->get<client::PlainTcpConnection>();
    client2.connect(clientCb);

    exec(1);

    std::ignore=server.close();

    serverThread->stop();
    clientThread->stop();

    exec(1);

    BOOST_CHECK_EQUAL(serverConnectCount,2);
    BOOST_CHECK_EQUAL(clientConnectCount,2);
}

using TestBuffersSet=std::vector<std::vector<ByteArray>>;

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
    auto spanBuffers=spanBuffers(buf);

    auto& connection=ctx->template get<ConnectionT>();
    connection.send(
        ctx,
        spanBuffers,
        [idx,&bufferSet,cb{std::move(cb)}](ContextT ctx, const HATN_NAMESPACE::Error& ec, size_t sentBytes, SpanBuffers)
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
void readNext(ContextT ctx, ByteArray& rxBuf, ByteArray& tmpBuf, CallbackT cb, size_t dataSize)
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
void recvNext(ContextT ctx, ByteArray& rxBuf, ByteArray& tmpBuf, CallbackT cb, size_t dataSize,
              const TestBuffersSet& bufferSet, size_t bufsIdx)
{
    if (bufsIdx==bufferSet.size())
    {
        HATN_CHECK_TS(false);
        cb(HATN_NAMESPACE::Error{});
        return;
    }

    ByteArray testBuf;
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
    ByteArray totalBuf;
    bufsSet.resize(setCount);
    for (size_t i=0;i<setCount;i++)
    {
        auto& bufs=bufsSet[i];

        auto bufCount=Random::uniform(1,8);
        bufs.resize(bufCount);
        size_t buffersSize=0;
        for (size_t j=0;j<bufCount;j++)
        {
            auto& buf=bufs[j];
            Random::randContainer(buf,maxBufSize,minBufSize);
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
void sendRead(TestEnv* TestEnv, MakeServerT makeServer, MakeClientT makeClient, bool sendServerToClient, bool recv=false)
{
    TestEnv->createThreads(2);
    auto serverThread=TestEnv->thread(0);
    auto clientThread=TestEnv->thread(1);

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
    ByteArray tmpBuf;
    tmpBuf.resize(rxTmpBufSize);
    ByteArray rxBuf;

    bool serverConnectionClosed=false;
    auto closeServerConnection=[&serverConnectionClosed](SharedPtr<server::PlainTcpConnectionContext> ctx, std::function<void()> cb)
    {
        serverConnectionClosed=true;
        auto& connection=ctx->get<server::PlainTcpConnection>();
        auto cb1=[ctx,cb](const HATN_NAMESPACE::Error& ec)
        {
            std::ignore=ctx;
            HATN_CHECK_TS(!ec);
            cb();
        };
        connection.close(cb1);
    };

    auto serverReadCb=[TestEnv,&serverConnectionClosed,&closeServerConnection](SharedPtr<server::PlainTcpConnectionContext> ctx,const HATN_NAMESPACE::Error& ec)
    {
        HATN_TEST_MESSAGE_TS(fmt::format("serverReadCb: {}/{}",ec.code(),ec.message()));
        if (!serverConnectionClosed)
        {
            closeServerConnection(ctx,[TestEnv](){TestEnv->quit();});
        }
    };
    auto serverSendCb=[](const HATN_NAMESPACE::Error& ec)
    {
        HATN_TEST_MESSAGE_TS(fmt::format("serverSendCb: {}/{}",ec.code(),ec.message()));
        HATN_CHECK_TS(!ec);
    };
    SharedPtr<server::PlainTcpConnectionContext> serverConnectionCtx;;
    auto serverConnectCb=[recv,sendServerToClient,&serverConnectionCtx,&serverConnectionClosed,&serverSendCb,&closeServerConnection,&sendBuffers,&serverReadCb,&rxBuf,&tmpBuf](SharedPtr<server::PlainTcpConnectionContext> ctx, const HATN_NAMESPACE::Error& ec)
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
                sendNext<server::PlainTcpConnection>(ctx,sendBuffers.first,0,cb);
            }
            else
            {
                if (recv)
                {
                    recvNext<server::PlainTcpConnection>(ctx,rxBuf,tmpBuf,
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
                    readNext<server::PlainTcpConnection>(ctx,rxBuf,tmpBuf,
                                                                             [ctx,&serverReadCb](const HATN_NAMESPACE::Error& ec){
                                                                                 serverReadCb(ctx,ec);
                                                                             },
                                                                             sendBuffers.second.size());
                }
            }
        }
    };

    auto& server=serverCtx->get<server::PlainTcpServer>();
    server.setConnectionHandler(serverConnectCb);
    asio::IpEndpoint serverEp{"127.0.0.1",TcpPort};
    auto ec=server.run(serverCtx,serverEp);
    if (ec)
    {
        BOOST_TEST_MESSAGE(fmt::format("failed to listen: {}/{}",ec.code(),ec.message()));
    }
    BOOST_REQUIRE(!ec);

    bool clientConnectionClosed=false;
    auto closeClientConnection=[&clientConnectionClosed](SharedPtr<client::PlainTcpConnectionContext> ctx, std::function<void()> cb)
    {
        clientConnectionClosed=true;
        auto& connection=ctx->get<client::PlainTcpConnection>();
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
    auto clientReadCb=[&clientConnectionClosed,&closeClientConnection,clientCtx,TestEnv](const HATN_NAMESPACE::Error& ec)
    {
        HATN_TEST_MESSAGE_TS(fmt::format("clientReadCb: {}/{}",ec.code(),ec.message()));
        if (!clientConnectionClosed)
        {
            closeClientConnection(clientCtx,[TestEnv](){TestEnv->quit();});
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
            sendNext<client::PlainTcpConnection>(clientCtx,sendBuffers.first,0,cb);
        }
        else
        {
            if (recv)
            {
                recvNext<client::PlainTcpConnection>(clientCtx,rxBuf,tmpBuf,clientReadCb,sendBuffers.second.size(),
                                                                         sendBuffers.first,
                                                                         0
                                                                        );
            }
            else
            {
                readNext<client::PlainTcpConnection>(clientCtx,rxBuf,tmpBuf,clientReadCb,sendBuffers.second.size());
            }
        }
    };
    auto& client=clientCtx->get<client::PlainTcpConnection>();
    client.connect(clientConnectCb);

    TestEnv->exec(10);

    serverConnectionClosed=true;
    clientConnectionClosed=true;
    std::ignore=server.close();
    client.close();

    serverThread->stop();
    clientThread->stop();

    TestEnv->exec(1);

    BOOST_CHECK_EQUAL(sendBuffers.second.size(),rxBuf.size());
    BOOST_CHECK(sendBuffers.second==rxBuf);
}

BOOST_FIXTURE_TEST_CASE(ClientSendServerRead,TestEnv)
{
    sendRead(this,createServer,createClient,false);
}

BOOST_FIXTURE_TEST_CASE(ServerSendClientRead,TestEnv)
{
    sendRead(this,createServer,createClient,true);
}

BOOST_FIXTURE_TEST_CASE(ClientSendServerRecv,TestEnv)
{
    sendRead(this,createServer,createClient,false,true);
}

BOOST_FIXTURE_TEST_CASE(ServerSendClientRecv,TestEnv)
{
    sendRead(this,createServer,createClient,true,true);
}

BOOST_AUTO_TEST_SUITE_END()

#endif
