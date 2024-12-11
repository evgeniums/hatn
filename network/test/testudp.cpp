#include <boost/test/unit_test.hpp>

#include <hatn/common/locker.h>

#include <hatn/logcontext/streamlogger.h>

#include <hatn/test/multithreadfixture.h>

#include <hatn/network/asio/udpchannel.h>

#define HATN_TEST_LOG_CONSOLE

namespace {

struct Env : public ::hatn::test::MultiThreadFixture
{
    Env()
    {
    }

    ~Env()
    {
    }

    Env(const Env&)=delete;
    Env(Env&&) =delete;
    Env& operator=(const Env&)=delete;
    Env& operator=(Env&&) =delete;
};
}

BOOST_AUTO_TEST_SUITE(Udp)

BOOST_FIXTURE_TEST_CASE(UdpChannelCtor,Env)
{
    auto server1=HATN_NETWORK_NAMESPACE::asio::makeUdpServerCtx(mainThread().get(),"server1");
    auto client1=HATN_NETWORK_NAMESPACE::asio::makeUdpClientCtx(mainThread().get(),"client1");

    BOOST_CHECK_EQUAL((*server1)->thread(),mainThread().get());
    BOOST_CHECK_EQUAL((*client1)->thread(),mainThread().get());

    createThreads(1);
    auto* thread0=thread(0).get();

    thread0->start();
    auto ec=thread0->execSync(
                    [thread0]()
                    {
                        auto server2=HATN_NETWORK_NAMESPACE::asio::makeUdpServerCtx("server2");
                        auto client2=HATN_NETWORK_NAMESPACE::asio::makeUdpClientCtx("client2");

                        HATN_CHECK_EQUAL_TS((*server2)->thread(),thread0);
                        HATN_CHECK_EQUAL_TS((*client2)->thread(),thread0);
                    }
                );
    BOOST_REQUIRE(!ec);
    thread0->stop();
}

BOOST_FIXTURE_TEST_CASE(UdpServerBind,Env)
{
    uint16_t portNumber1=11511;
    uint16_t portNumber2=11712;
    uint16_t portNumber3=11513;
    uint16_t portNumber4=11714;

    createThreads(2);
    auto* thread0=thread(0).get();
    auto* thread1=thread(1).get();

    {
        HATN_NETWORK_NAMESPACE::asio::UdpServerSharedCtx server1,server2,server3,server4;
        HATN_NETWORK_NAMESPACE::asio::UdpServerSharedCtx server5,server6,server7,server8;
        HATN_NETWORK_NAMESPACE::asio::UdpServerSharedCtx server9,server10;

        thread0->start();
        thread1->start();

        std::atomic<size_t> okCount{0};
        auto bindOk=[&okCount](const HATN_COMMON_NAMESPACE::Error &ec)
        {
            HATN_CHECK_TS(!ec);
            okCount++;
        };

        std::atomic<size_t> failCount{0};
        auto bindFail=[&failCount](const HATN_COMMON_NAMESPACE::Error &ec)
        {            
            HATN_CHECK_TS(ec);
            HATN_CHECK_EQUAL_TS(ec.errorCondition(),boost::system::errc::address_in_use);
            failCount++;
        };

        auto ec=thread0->execSync(
                        [&bindOk,&server1,&server2,&server3,&server4,
                         portNumber1,portNumber2,portNumber3,portNumber4,&server9,&server10]()
                        {
                            server1=HATN_NETWORK_NAMESPACE::asio::makeUdpServerCtx("server1");
                            HATN_NETWORK_NAMESPACE::asio::UdpEndpoint ep1("127.0.0.1",portNumber1);
                            (*server1)->bind(ep1,bindOk);

                            server2=HATN_NETWORK_NAMESPACE::asio::makeUdpServerCtx("server2");
                            HATN_NETWORK_NAMESPACE::asio::UdpEndpoint ep2(boost::asio::ip::address_v4::any(),portNumber2);
                            (*server2)->bind(ep2,bindOk);

                            server3=HATN_NETWORK_NAMESPACE::asio::makeUdpServerCtx("server3");
                            HATN_NETWORK_NAMESPACE::asio::UdpEndpoint ep3(boost::asio::ip::address_v6::loopback(),portNumber3);
                            (*server3)->bind(ep3,bindOk);

                            server4=HATN_NETWORK_NAMESPACE::asio::makeUdpServerCtx("server4");
                            HATN_NETWORK_NAMESPACE::asio::UdpEndpoint ep4(boost::asio::ip::address_v6::any(),portNumber4);
                            (*server4)->bind(ep4,bindOk);

                            server9=HATN_NETWORK_NAMESPACE::asio::makeUdpServerCtx("server9");
                            HATN_NETWORK_NAMESPACE::asio::UdpEndpoint ep5(boost::asio::ip::address_v4::any());
                            (*server9)->bind(ep5,bindOk);

                            server10=HATN_NETWORK_NAMESPACE::asio::makeUdpServerCtx("server10");
                            HATN_NETWORK_NAMESPACE::asio::UdpEndpoint ep6(boost::asio::ip::address_v6::any());
                            (*server10)->bind(ep6,bindOk);
                        }
                    );
        BOOST_REQUIRE(!ec);
        exec(1);

        ec=thread1->execSync(
                        [&bindFail,&server5,&server6,&server7,&server8,
                        portNumber1,portNumber2,portNumber3,portNumber4]()
                        {
                            server5=HATN_NETWORK_NAMESPACE::asio::makeUdpServerCtx("server5");
                            HATN_NETWORK_NAMESPACE::asio::UdpEndpoint ep1("127.0.0.1",portNumber1);
                            (*server5)->bind(ep1,bindFail);

                            server6=HATN_NETWORK_NAMESPACE::asio::makeUdpServerCtx("server6");
                            HATN_NETWORK_NAMESPACE::asio::UdpEndpoint ep2(boost::asio::ip::address_v4::any(),portNumber2);
                            (*server6)->bind(ep2,bindFail);

                            server7=HATN_NETWORK_NAMESPACE::asio::makeUdpServerCtx("server7");
                            HATN_NETWORK_NAMESPACE::asio::UdpEndpoint ep3(boost::asio::ip::address_v6::loopback(),portNumber3);
                            (*server7)->bind(ep3,bindFail);

                            server8=HATN_NETWORK_NAMESPACE::asio::makeUdpServerCtx("server8");
                            HATN_NETWORK_NAMESPACE::asio::UdpEndpoint ep4(boost::asio::ip::address_v6::any(),portNumber4);
                            (*server8)->bind(ep4,bindFail);
                        }
                    );
        BOOST_REQUIRE(!ec);
        exec(1);

        std::atomic<size_t> closeCount{0};
        ec=thread0->execSync(
                        [&closeCount,&server1,&server2,&server3,&server4,&server9,&server10]()
                        {
                            auto closeOk=[&closeCount](const HATN_COMMON_NAMESPACE::Error &ec)
                            {
                                HATN_CHECK_TS(!ec);
                                closeCount++;
                            };

                            (*server1)->close(closeOk);
                            (*server2)->close(closeOk);
                            (*server3)->close(closeOk);
                            (*server4)->close(closeOk);

                            (*server9)->close(closeOk);
                            (*server10)->close(closeOk);
                        }
                    );
        BOOST_REQUIRE(!ec);
        exec(1);

        thread0->stop();
        thread1->stop();

        BOOST_CHECK_EQUAL(static_cast<size_t>(okCount),6);
        BOOST_CHECK_EQUAL(static_cast<size_t>(failCount),4);
        BOOST_CHECK_EQUAL(static_cast<size_t>(closeCount),6);
    }
}

BOOST_FIXTURE_TEST_CASE(UdpClientPrepare,Env)
{
    uint16_t serverPortNumber=11511;
    uint16_t portNumber1=11712;
    uint16_t portNumber2=11513;

    createThreads(1);
    HATN_COMMON_NAMESPACE::Thread* thread0=thread(0).get();

    {
        HATN_NETWORK_NAMESPACE::asio::UdpClientSharedCtx client1,client2,client3,client4;

        thread0->start();

        std::atomic<size_t> prepareCount{0};

        auto ec=thread0->execSync(
                        [&prepareCount,&client1,&client2,&client3,&client4,portNumber1,portNumber2,serverPortNumber]()
                        {
                            client1=HATN_NETWORK_NAMESPACE::asio::makeUdpClientCtx("client1");
                            HATN_NETWORK_NAMESPACE::asio::UdpEndpoint serverEp1(boost::asio::ip::address_v4::loopback(),serverPortNumber);
                            (*client1)->setRemoteEndpoint(serverEp1);
                            auto prepareCb1=[&prepareCount,&client1](const HATN_COMMON_NAMESPACE::Error &ec)
                            {
                                HATN_CHECK_TS(!ec);
                                HATN_CHECK_EQUAL_TS((*client1)->localEndpoint().address().to_string(),"127.0.0.1");
                                prepareCount++;
                            };
                            (*client1)->prepare(prepareCb1);

                            client2=HATN_NETWORK_NAMESPACE::asio::makeUdpClientCtx("client2");
                            HATN_NETWORK_NAMESPACE::asio::UdpEndpoint serverEp2(boost::asio::ip::address_v6::loopback(),serverPortNumber);
                            (*client2)->setRemoteEndpoint(serverEp2);
                            auto prepareCb2=[&prepareCount,&client2](const HATN_COMMON_NAMESPACE::Error &ec)
                            {
                                HATN_CHECK_TS(!ec);
                                HATN_CHECK_EQUAL_TS((*client2)->localEndpoint().address().to_string(),"::1");
                                prepareCount++;
                            };
                            (*client2)->prepare(prepareCb2);

                            client3=HATN_NETWORK_NAMESPACE::asio::makeUdpClientCtx("client3");
                            (*client3)->setRemoteEndpoint(serverEp1);
                            (*client3)->setLocalEndpoint(HATN_NETWORK_NAMESPACE::asio::UdpEndpoint(boost::asio::ip::address_v4::any(),portNumber1));
                            auto prepareCb3=[&prepareCount,&client3,portNumber1](const HATN_COMMON_NAMESPACE::Error &ec)
                            {
                                HATN_CHECK_TS(!ec);
                                HATN_CHECK_EQUAL_TS((*client3)->localEndpoint().address().to_string(),"127.0.0.1");
                                HATN_CHECK_EQUAL_TS((*client3)->localEndpoint().port(),portNumber1);
                                prepareCount++;
                            };
                            (*client3)->prepare(prepareCb3);

                            client4=HATN_NETWORK_NAMESPACE::asio::makeUdpClientCtx("client4");
                            (*client4)->setRemoteEndpoint(serverEp2);
                            (*client4)->setLocalEndpoint(HATN_NETWORK_NAMESPACE::asio::UdpEndpoint(boost::asio::ip::address_v6::any(),portNumber2));
                            auto prepareCb4=[&prepareCount,&client4,portNumber2](const HATN_COMMON_NAMESPACE::Error &ec)
                            {
                                HATN_CHECK_TS(!ec);
                                HATN_CHECK_EQUAL_TS((*client4)->localEndpoint().address().to_string(),"::1");
                                HATN_CHECK_EQUAL_TS((*client4)->localEndpoint().port(),portNumber2);
                                prepareCount++;
                            };
                            (*client4)->prepare(prepareCb4);
                        }
                    );
        BOOST_REQUIRE(!ec);

        exec(1);

        std::atomic<size_t> closeCount{0};
        ec=thread0->execSync(
                        [&closeCount,&client1,&client2,&client3,&client4]()
                        {
                            auto closeOk=[&closeCount](const HATN_COMMON_NAMESPACE::Error &ec)
                            {
                                HATN_CHECK_TS(!ec);
                                closeCount++;
                            };

                            (*client1)->close(closeOk);
                            (*client2)->close(closeOk);
                            (*client3)->close(closeOk);
                            (*client4)->close(closeOk);
                        }
                    );
        BOOST_REQUIRE(!ec);

        exec(1);

        BOOST_CHECK_EQUAL(static_cast<size_t>(closeCount),4);
        BOOST_CHECK_EQUAL(static_cast<size_t>(prepareCount),4);

        thread0->stop();
    }
}

constexpr static const size_t dataSize=512;
#ifdef BUILD_VALGRIND
    constexpr static const size_t TxPacketCount=10;
#else
    #if defined (ANDROID)
        constexpr static const size_t TxPacketCount=10000;
    #else
        #if defined(BUILD_DEBUG)
            constexpr static const size_t TxPacketCount=1000;
        #else
            constexpr static const size_t TxPacketCount=100000;
        #endif
    #endif
#endif

static void sendNextServerScattered(HATN_NETWORK_NAMESPACE::asio::UdpServer* server,
      std::array<char,dataSize>& buf,
      const HATN_NETWORK_NAMESPACE::asio::UdpEndpoint& ep,
      size_t& packetsToSend,
      size_t& txPacketsCount,
      bool append
);

static void sendNextServer(HATN_NETWORK_NAMESPACE::asio::UdpServer* server,
              std::array<char,dataSize>& buf,
              const HATN_NETWORK_NAMESPACE::asio::UdpEndpoint& ep,
              size_t& packetsToSend,
              size_t& txPacketsCount,
              bool append
              )
{
    HATN_CTX_SCOPE("sendNextServer")

    if (append)
    {
        ++packetsToSend;
    }
    if (packetsToSend>0)
    {
        --packetsToSend;
        HATN_COMMON_NAMESPACE::SpanBuffer cbuf(
                                            HATN_COMMON_NAMESPACE::makeShared<HATN_COMMON_NAMESPACE::ByteArrayManaged>(buf.data(),dataSize)
                                           );
        server->sendTo(std::move(cbuf),ep,
                       [server,&buf,&ep,&packetsToSend,&txPacketsCount](const HATN_COMMON_NAMESPACE::Error& ec,size_t size,const HATN_COMMON_NAMESPACE::SpanBuffer&)
                        {
                            HATN_CTX_SCOPE("sendNextServerCb")
                            if (!ec)
                            {
                                if (size!=dataSize)
                                {
                                    BOOST_FAIL("Invalid size of UDP packet");
                                }

                                ++txPacketsCount;
                                sendNextServerScattered(server,buf,ep,packetsToSend,txPacketsCount,false);
                            }
                        }
                       );
    }
}

static void sendNextServerScattered(HATN_NETWORK_NAMESPACE::asio::UdpServer* server,
          std::array<char,dataSize>& buf,
          const HATN_NETWORK_NAMESPACE::asio::UdpEndpoint& ep,
          size_t& packetsToSend,
          size_t& txPacketsCount,
          bool append
    )
{
    HATN_CTX_SCOPE("sendNextServerScattered")

    if (append)
    {
        ++packetsToSend;
    }
    if (packetsToSend>0)
    {
        --packetsToSend;

        HATN_COMMON_NAMESPACE::SpanBuffers bufs;
        bufs.emplace_back(
                            HATN_COMMON_NAMESPACE::makeShared<HATN_COMMON_NAMESPACE::ByteArrayManaged>(buf.data(),dataSize/4)
                    );
        auto sharedBuf=HATN_COMMON_NAMESPACE::makeShared<HATN_COMMON_NAMESPACE::ByteArrayManaged>(buf.data(),dataSize);
        size_t offset=dataSize/4;
        bufs.emplace_back(
                        sharedBuf,offset,dataSize/4
                    );
        offset+=dataSize/4;
        bufs.emplace_back(
                        sharedBuf,offset,dataSize/4
                    );
        offset+=dataSize/4;
        bufs.emplace_back(
                        sharedBuf,offset,dataSize/4
                    );

        server->sendTo(std::move(bufs),ep,
                       [server,&buf,&ep,&packetsToSend,&txPacketsCount](const HATN_COMMON_NAMESPACE::Error& ec,size_t size,const HATN_COMMON_NAMESPACE::SpanBuffers&)
                        {
                            HATN_CTX_SCOPE("sendNextServerScatteredCb")
                            if (!ec)
                            {
                                if (size!=dataSize)
                                {
                                    BOOST_FAIL("Invalid size of UDP packet");
                                }

                                ++txPacketsCount;
                                sendNextServer(server,buf,ep,packetsToSend,txPacketsCount,false);
                            }
                        }
                       );
    }
}

static void recvNext(HATN_NETWORK_NAMESPACE::asio::UdpServer* server,
              std::array<char,dataSize>& buf,
              size_t& rxPacketCount,
              size_t& packetsToSend,
              size_t& txPacketsCount
              )
{
    HATN_CTX_SCOPE("recvNext")
    server->receiveFrom(buf.data(),buf.size(),
                            [server,&buf,&rxPacketCount,&packetsToSend,&txPacketsCount](const HATN_COMMON_NAMESPACE::Error& ec,size_t size,const HATN_NETWORK_NAMESPACE::asio::UdpEndpoint& ep)
                            {
                                HATN_CTX_SCOPE("recvNextCb")
                                if (!ec)
                                {
                                    if (size!=dataSize)
                                    {
                                        BOOST_FAIL("Invalid size of UDP packet");
                                    }

                                    if (++rxPacketCount==TxPacketCount)
                                    {
                                        BOOST_TEST_MESSAGE("Finished receiving on server");
                                    }

                                    sendNextServer(server,buf,ep,packetsToSend,txPacketsCount,true);
                                    recvNext(server,buf,rxPacketCount,packetsToSend,txPacketsCount);
                                }
                                else
                                {
                                    HATN_CTX_INFO_RECORDS("Failed to recv packet on server",{"packet_id",rxPacketCount},{"ec_code",ec.value()},{"ec_message",ec.message()});
                                }
                            }
                        );
}
static void recvNextClient(HATN_NETWORK_NAMESPACE::asio::UdpClient* client,
              std::array<char,dataSize>& buf,
              size_t& rxPacketCount
              )
{
    HATN_CTX_SCOPE("recvNextClient")
    client->receive(buf.data(),buf.size(),
                            [client,&buf,&rxPacketCount](const HATN_COMMON_NAMESPACE::Error& ec,size_t size)
                            {
                                HATN_CTX_SCOPE("recvNextClientCb")
                                ++rxPacketCount;
                                if (!ec)
                                {
                                    if (size!=dataSize)
                                    {
                                        BOOST_FAIL("Invalid size of UDP packet");
                                    }

                                    recvNextClient(client,buf,rxPacketCount);
                                }
                                else
                                {
                                    HATN_CTX_INFO_RECORDS("Failed to recv packet on client",{"packet_id",rxPacketCount},{"ec_code",ec.value()},{"ec_message",ec.message()});
                                }
                            }
                        );
}

static void sendNextScatter(HATN_NETWORK_NAMESPACE::asio::UdpClient* client,
      std::array<char,dataSize>& buf,
      size_t& txPacketCount,
      hatn::test::MultiThreadFixture* mainLoop
);

static void sendNext(HATN_NETWORK_NAMESPACE::asio::UdpClient* client,
              std::array<char,dataSize>& buf,
              size_t& txPacketCount,
              hatn::test::MultiThreadFixture* mainLoop
              )
{
    HATN_CTX_SCOPE("sendNext")
    if (txPacketCount<TxPacketCount)
    {
        ++txPacketCount;
        HATN_COMMON_NAMESPACE::SpanBuffer cbuf(HATN_COMMON_NAMESPACE::makeShared<HATN_COMMON_NAMESPACE::ByteArrayManaged>(buf.data(),dataSize));
        client->send(std::move(cbuf),
                                [client,&buf,&txPacketCount,mainLoop](const HATN_COMMON_NAMESPACE::Error& ec,size_t size,const HATN_COMMON_NAMESPACE::SpanBuffer&)
                                {
                                    HATN_CTX_SCOPE("sendNextCb")
                                    if (!ec)
                                    {
                                        if (size!=dataSize)
                                        {
                                            BOOST_FAIL("Invalid size of UDP packet");
                                        }
                                        sendNextScatter(client,buf,txPacketCount,mainLoop);
                                    }
                                    else
                                    {
                                        HATN_CTX_INFO_RECORDS("Failed to send packet on client",{"packet_id",txPacketCount},{"ec_code",ec.value()},{"ec_message",ec.message()});
                                    }
                                }
                            );
    }
    else
    {
        HATN_CTX_INFO("finished sending");

        mainLoop->mainThread()->installTimer(5000*1000,
                                             [mainLoop]()
                                             {
                                                mainLoop->quit();
                                                return false;
                                             },
                                                true
                                             );
    }
}

static void sendNextScatter(HATN_NETWORK_NAMESPACE::asio::UdpClient* client,
              std::array<char,dataSize>& buf,
              size_t& txPacketCount,
              hatn::test::MultiThreadFixture* mainLoop
    )
{
    HATN_CTX_SCOPE("sendNextScatter")
    if (txPacketCount<TxPacketCount)
    {
        ++txPacketCount;

        HATN_COMMON_NAMESPACE::SpanBuffers bufs;
        bufs.emplace_back(
                            HATN_COMMON_NAMESPACE::makeShared<HATN_COMMON_NAMESPACE::ByteArrayManaged>(buf.data(),dataSize/4)
                    );
        auto sharedBuf=HATN_COMMON_NAMESPACE::makeShared<HATN_COMMON_NAMESPACE::ByteArrayManaged>(buf.data(),dataSize);
        size_t offset=dataSize/4;
        bufs.emplace_back(
                        sharedBuf,offset,dataSize/4
                    );
        offset+=dataSize/4;
        bufs.emplace_back(
                        sharedBuf,offset,dataSize/4
                    );
        offset+=dataSize/4;
        bufs.emplace_back(
                        sharedBuf,offset,dataSize/4
                    );

        client->send(std::move(bufs),
                        [client,&buf,&txPacketCount,mainLoop](const HATN_COMMON_NAMESPACE::Error& ec,size_t size,const HATN_COMMON_NAMESPACE::SpanBuffers&)
                        {
                            HATN_CTX_SCOPE("sendNextScatterCb")
                            if (!ec)
                            {
                                if (size!=dataSize)
                                {
                                    BOOST_FAIL("Invalid size of UDP packet");
                                }
                                sendNext(client,buf,txPacketCount,mainLoop);
                            }
                            else
                            {
                                HATN_CTX_INFO_RECORDS("Failed to send packet on client",{"packet_id",txPacketCount},{"ec_code",ec.value()},{"ec_message",ec.message()});
                            }
                        }
                    );
    }
    else
    {
        HATN_CTX_INFO("finished scattered sending");

        mainLoop->mainThread()->installTimer(5000*1000,
                                             [mainLoop]()
                                             {
                                                mainLoop->quit();
                                                return false;
                                             },
                                                true
                                             );
    }
}

BOOST_FIXTURE_TEST_CASE(UdpSendRecv,Env)
{
    auto handler=std::make_shared<HATN_LOGCONTEXT_NAMESPACE::StreamLogger>();
    HATN_LOGCONTEXT_NAMESPACE::ContextLogger::init(std::static_pointer_cast<HATN_LOGCONTEXT_NAMESPACE::LoggerHandler>(handler));
    HATN_LOGCONTEXT_NAMESPACE::ContextLogger::instance().setDefaultLogLevel(HATN_LOGCONTEXT_NAMESPACE::LogLevel::Debug);

    createThreads(2);
    auto* thread0=thread(0).get();
    auto* thread1=thread(1).get();

    HATN_NETWORK_NAMESPACE::asio::UdpEndpoint serverEndpoint;
    serverEndpoint.setPort(19123);

    thread0->start();
    thread1->start();

    std::array<char,dataSize> clientSendBuf;
    std::array<char,dataSize> serverRecvBuf;
    std::array<char,dataSize> clientRecvBuf;
    randBuf(clientSendBuf);

    for (int i=0;i<2;i++)
    {
        size_t rxPacketCount=0;
        size_t txPacketCount=0;
        size_t serverPacketsToSend=0;
        size_t serverTxPacketCount=0;
        size_t clientRxPacketCount=0;

        bool isIpv6=i==1;
        if (!isIpv6)
        {
            BOOST_TEST_MESSAGE("Begin IPv4");
            serverEndpoint.setAddress(boost::asio::ip::address_v4::loopback());
        }
        else
        {
            BOOST_TEST_MESSAGE("Begin IPv6");
            serverEndpoint.setAddress(boost::asio::ip::address_v6::loopback());
        }

        HATN_NETWORK_NAMESPACE::asio::UdpServerSharedCtx server;
        HATN_NETWORK_NAMESPACE::asio::UdpClientSharedCtx client;

        BOOST_TEST_MESSAGE("Run server");

        auto ec=thread0->execSync(
                        [&server,&serverEndpoint,&rxPacketCount,&serverRecvBuf,&serverPacketsToSend,&serverTxPacketCount,isIpv6]()
                        {
                            server=HATN_NETWORK_NAMESPACE::asio::makeUdpServerCtx(isIpv6?"server-6":"server-4");
                            server->beginTaskContext();

                            HATN_CTX_SCOPE("serverbind")
                            HATN_CTX_DEBUG("binding server")

                            auto bindOk=[&server,&rxPacketCount,&serverRecvBuf,&serverPacketsToSend,&serverTxPacketCount](const HATN_COMMON_NAMESPACE::Error &ec)
                            {
                                HATN_CTX_SCOPE("serverbindcb")
                                HATN_CTX_DEBUG("server bound")

                                HATN_REQUIRE_TS(!ec);
                                recvNext(&server->get(),serverRecvBuf,rxPacketCount,serverPacketsToSend,serverTxPacketCount);
                            };
                            (*server)->bind(serverEndpoint,bindOk);
                        }
        );
        BOOST_REQUIRE(!ec);

        auto connectCb=[&thread1,&client,&clientSendBuf,&txPacketCount,&clientRecvBuf,&clientRxPacketCount,this](const HATN_COMMON_NAMESPACE::Error& ec)
        {
            HATN_CTX_SCOPE("connectCb")
            HATN_CTX_DEBUG("client connected")

            HATN_REQUIRE_TS(!ec);
            HATN_CHECK_EQUAL_TS(thread1,HATN_COMMON_NAMESPACE::Thread::currentThread());
            BOOST_TEST_MESSAGE("Sending/receiving...");
            recvNextClient(&client->get(),clientRecvBuf,clientRxPacketCount);
            sendNext(&client->get(),clientSendBuf,txPacketCount,this);
        };

        BOOST_TEST_MESSAGE("Connect client");

        thread1->execAsync(
                        [&client,&serverEndpoint,&connectCb,isIpv6]()
                        {
                            client=HATN_NETWORK_NAMESPACE::asio::makeUdpClientCtx(isIpv6?"client-6":"client-4");
                            client->beginTaskContext();

                            HATN_CTX_SCOPE("clientPrepare")
                            HATN_CTX_DEBUG("client preparing")

                            (*client)->setRemoteEndpoint(serverEndpoint);
                            (*client)->prepare(connectCb);
                        }
        );

        BOOST_TEST_MESSAGE("Exec for 20 seconds...");
        exec(20);

        auto clientCloseCb=[](const HATN_COMMON_NAMESPACE::Error& ec)
        {
            HATN_CTX_SCOPE("clientCloseCb")
            HATN_CTX_DEBUG("client closed")

            HATN_CHECK_TS(!ec);
        };

        BOOST_TEST_MESSAGE("Close client");

        ec=thread1->execSync(
                        [&client,&clientCloseCb]()
                        {
                            client->beginTaskContext();

                            HATN_CTX_SCOPE("clientclose")
                            HATN_CTX_DEBUG("client closing")

                            (*client)->close(clientCloseCb);
                        }
        );
        BOOST_REQUIRE(!ec);

        BOOST_TEST_MESSAGE("Close server");

        auto serverCloseCb=[](const HATN_COMMON_NAMESPACE::Error& ec)
        {
            HATN_CTX_SCOPE("serverCloseCb")
            HATN_CTX_DEBUG("server closed")

            HATN_CHECK_TS(!ec);
        };

        ec=thread0->execSync(
                        [&serverCloseCb,&server]()
                        {
                            server->beginTaskContext();
                            HATN_CTX_SCOPE("serverclose")
                            HATN_CTX_DEBUG("server closing")

                            (*server)->close(serverCloseCb);
                        }
        );
        BOOST_REQUIRE(!ec);
        exec(1);

        if (!isIpv6)
        {
            BOOST_TEST_MESSAGE("End IPv4");
        }
        else
        {
            BOOST_TEST_MESSAGE("End IPv6");
        }

        {
            HATN_CHECK_EQUAL_TS(txPacketCount,TxPacketCount);
            HATN_CHECK_TS(rxPacketCount>(TxPacketCount/2));
            HATN_CHECK_EQUAL_TS(rxPacketCount,serverTxPacketCount);
            HATN_CHECK_TS(clientRxPacketCount>(serverTxPacketCount/2));
        }
        BOOST_TEST_MESSAGE(fmt::format("Server received {} of {} packets",rxPacketCount,TxPacketCount));
        BOOST_TEST_MESSAGE(fmt::format("Client received {} of {} packets",clientRxPacketCount,serverTxPacketCount));
    }

    BOOST_TEST_MESSAGE("Waiting for 3 seconds");
    exec(3);

    thread0->stop();
    thread1->stop();
}

BOOST_AUTO_TEST_SUITE_END()
