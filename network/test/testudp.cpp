#include <boost/test/unit_test.hpp>

#include <hatn/common/locker.h>

#include <hatn/common/logger.h>
#include <hatn/test/multithreadfixture.h>

#include <hatn/network/asio/udpchannel.h>

#define HATN_TEST_LOG_CONSOLE

namespace {

// static void setLogHandler()
// {
//     auto handler=[](const ::hatn::common::FmtAllocatedBufferChar &s)
//     {
//         #ifdef HATN_TEST_LOG_CONSOLE
//             std::cout<<::hatn::common::lib::toStringView(s)<<std::endl;
//         #else
//             std::ignore=s;
//         #endif
//     };

//     ::hatn::common::Logger::setOutputHandler(handler);
//     ::hatn::common::Logger::setFatalLogHandler(handler);
//     ::hatn::common::Logger::setDefaultVerbosity(hatn::common::LoggerVerbosity::INFO);
// }

struct Env : public ::hatn::test::MultiThreadFixture
{
    Env()
    {
        // if (!::hatn::common::Logger::isRunning())
        // {
        //     ::hatn::common::Logger::setFatalTracing(false);

        //     setLogHandler();
        //     ::hatn::common::Logger::start();
        // }
    }

    ~Env()
    {
        // ::hatn::common::Logger::stop();
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
    hatn::common::Thread* thread0=thread(0).get();

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
#if 0
BOOST_FIXTURE_TEST_CASE(UdpServerBind,Env)
{
    hatn::common::MutexLock mutex;

    uint16_t portNumber1=11511;
    uint16_t portNumber2=11712;
    uint16_t portNumber3=11513;
    uint16_t portNumber4=11714;

    createThreads(2);
    hatn::common::Thread* thread0=thread(0).get();
    hatn::common::Thread* thread1=thread(1).get();

    {
        std::shared_ptr<hatn::network::asio::UdpServer> server1,server2,server3,server4;
        std::shared_ptr<hatn::network::asio::UdpServer> server5,server6,server7,server8;
        std::shared_ptr<hatn::network::asio::UdpServer> server9,server10;

        thread0->start();
        thread1->start();

        auto bindOk=[&mutex](const hatn::common::Error &ec)
        {
            hatn::common::MutexScopedLock l(mutex);
            BOOST_CHECK(!ec);
        };
        auto bindFail=[&mutex](const hatn::common::Error &ec)
        {
            hatn::common::MutexScopedLock l(mutex);
            BOOST_CHECK(ec);
            BOOST_CHECK_EQUAL(ec.errorCondition(),boost::system::errc::address_in_use);
        };

        auto ec=thread0->execSync(
                        [&bindOk,&server1,&server2,&server3,&server4,portNumber1,portNumber2,portNumber3,portNumber4,&server9,&server10]()
                        {
                            server1=std::make_shared<hatn::network::asio::UdpServer>("server1");
                            hatn::network::asio::UdpEndpoint ep1("127.0.0.1",portNumber1);
                            server1->bind(ep1,bindOk);

                            server2=std::make_shared<hatn::network::asio::UdpServer>("server2");
                            hatn::network::asio::UdpEndpoint ep2(boost::asio::ip::address_v4::any(),portNumber2);
                            server2->bind(ep2,bindOk);

                            server3=std::make_shared<hatn::network::asio::UdpServer>("server3");
                            hatn::network::asio::UdpEndpoint ep3(boost::asio::ip::address_v6::loopback(),portNumber3);
                            server3->bind(ep3,bindOk);

                            server4=std::make_shared<hatn::network::asio::UdpServer>("server4");
                            hatn::network::asio::UdpEndpoint ep4(boost::asio::ip::address_v6::any(),portNumber4);
                            server4->bind(ep4,bindOk);

                            server9=std::make_shared<hatn::network::asio::UdpServer>("server9");
                            hatn::network::asio::UdpEndpoint ep5(boost::asio::ip::address_v4::any());
                            server9->bind(ep5,bindOk);

                            server10=std::make_shared<hatn::network::asio::UdpServer>("server10");
                            hatn::network::asio::UdpEndpoint ep6(boost::asio::ip::address_v6::any());
                            server10->bind(ep6,bindOk);
                        }
                    );
        BOOST_REQUIRE(!ec);

        ec=thread1->execSync(
                        [&bindFail,&server5,&server6,&server7,&server8,portNumber1,portNumber2,portNumber3,portNumber4]()
                        {
                            server5=std::make_shared<hatn::network::asio::UdpServer>("server5");
                            hatn::network::asio::UdpEndpoint ep1("127.0.0.1",portNumber1);
                            server5->bind(ep1,bindFail);

                            server6=std::make_shared<hatn::network::asio::UdpServer>("server6");
                            hatn::network::asio::UdpEndpoint ep2(boost::asio::ip::address_v4::any(),portNumber2);
                            server6->bind(ep2,bindFail);

                            server7=std::make_shared<hatn::network::asio::UdpServer>("server7");
                            hatn::network::asio::UdpEndpoint ep3(boost::asio::ip::address_v6::loopback(),portNumber3);
                            server7->bind(ep3,bindFail);

                            server8=std::make_shared<hatn::network::asio::UdpServer>("server8");
                            hatn::network::asio::UdpEndpoint ep4(boost::asio::ip::address_v6::any(),portNumber4);
                            server8->bind(ep4,bindFail);
                        }
                    );
        BOOST_REQUIRE(!ec);

        ec=thread0->execSync(
                        [&mutex,&server1,&server2,&server3,&server4,&server9,&server10]()
                        {
                            auto closeOk=[&mutex](const hatn::common::Error &ec)
                            {
                                hatn::common::MutexScopedLock l(mutex);
                                BOOST_CHECK(!ec);
                            };

                            server1->close(closeOk);
                            server2->close(closeOk);
                            server3->close(closeOk);
                            server4->close(closeOk);

                            server9->close(closeOk);
                            server10->close(closeOk);
                        }
                    );
        BOOST_REQUIRE(!ec);

        thread0->stop();
        thread1->stop();
    }
}

BOOST_FIXTURE_TEST_CASE(UdpClientConnect,Env)
{
    hatn::common::MutexLock mutex;

    uint16_t serverPortNumber=11511;
    uint16_t portNumber1=11712;
    uint16_t portNumber2=11513;

    createThreads(1);
    hatn::common::Thread* thread0=thread(0).get();

    {
        std::shared_ptr<hatn::network::asio::UdpClient> client1,client2,client3,client4;

        thread0->start();

        HATN_CHECK_EXEC_SYNC(
        thread0->execSync(
                        [&mutex,&client1,&client2,&client3,&client4,portNumber1,portNumber2,serverPortNumber]()
                        {
                            client1=std::make_shared<hatn::network::asio::UdpClient>("client1");
                            hatn::network::asio::UdpEndpoint serverEp1(boost::asio::ip::address_v4::loopback(),serverPortNumber);
                            client1->setRemoteEndpoint(serverEp1);
                            auto connectCb1=[&mutex,&client1](const hatn::common::Error &ec)
                            {
                                hatn::common::MutexScopedLock l(mutex);
                                BOOST_CHECK(!ec);
                                BOOST_CHECK_EQUAL(client1->localEndpoint().address().to_string(),"127.0.0.1");
                            };
                            client1->prepare(connectCb1);

                            client2=std::make_shared<hatn::network::asio::UdpClient>("client2");
                            hatn::network::asio::UdpEndpoint serverEp2(boost::asio::ip::address_v6::loopback(),serverPortNumber);
                            client2->setRemoteEndpoint(serverEp2);
                            auto connectCb2=[&mutex,&client2](const hatn::common::Error &ec)
                            {
                                hatn::common::MutexScopedLock l(mutex);
                                BOOST_CHECK(!ec);
                                BOOST_CHECK_EQUAL(client2->localEndpoint().address().to_string(),"::1");
                            };
                            client2->prepare(connectCb2);

                            client3=std::make_shared<hatn::network::asio::UdpClient>("client3");
                            client3->setRemoteEndpoint(serverEp1);
                            client3->setLocalEndpoint(hatn::network::asio::UdpEndpoint(boost::asio::ip::address_v4::any(),portNumber1));
                            auto connectCb3=[&mutex,&client3,portNumber1](const hatn::common::Error &ec)
                            {
                                hatn::common::MutexScopedLock l(mutex);
                                BOOST_CHECK(!ec);
                                BOOST_CHECK_EQUAL(client3->localEndpoint().address().to_string(),"127.0.0.1");
                                BOOST_CHECK_EQUAL(client3->localEndpoint().port(),portNumber1);
                            };
                            client3->prepare(connectCb3);

                            client4=std::make_shared<hatn::network::asio::UdpClient>("client4");
                            client4->setRemoteEndpoint(serverEp2);
                            client4->setLocalEndpoint(hatn::network::asio::UdpEndpoint(boost::asio::ip::address_v6::any(),portNumber2));
                            auto connectCb4=[&mutex,&client4,portNumber2](const hatn::common::Error &ec)
                            {
                                hatn::common::MutexScopedLock l(mutex);
                                BOOST_CHECK(!ec);
                                BOOST_CHECK_EQUAL(client4->localEndpoint().address().to_string(),"::1");
                                BOOST_CHECK_EQUAL(client4->localEndpoint().port(),portNumber2);
                            };
                            client4->prepare(connectCb4);
                        }
                    ));

        exec(1);

        HATN_CHECK_EXEC_SYNC(
        thread0->execSync(
                        [&mutex,&client1,&client2,&client3,&client4]()
                        {
                            auto closeOk=[&mutex](const hatn::common::Error &ec)
                            {
                                hatn::common::MutexScopedLock l(mutex);
                                BOOST_CHECK(!ec);
                            };

                            client1->close(closeOk);
                            client2->close(closeOk);
                            client3->close(closeOk);
                            client4->close(closeOk);
                        }
                    ));

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

static void sendNextServerScattered(hatn::network::asio::UdpServer* server,
      std::array<char,dataSize>& buf,
      const hatn::network::asio::UdpEndpoint& ep,
      size_t& packetsToSend,
      size_t& txPacketsCount,
      bool append
);

static void sendNextServer(hatn::network::asio::UdpServer* server,
              std::array<char,dataSize>& buf,
              const hatn::network::asio::UdpEndpoint& ep,
              size_t& packetsToSend,
              size_t& txPacketsCount,
              bool append
              )
{
    if (append)
    {
        ++packetsToSend;
    }
    if (packetsToSend>0)
    {
        --packetsToSend;
        hatn::common::SpanBuffer cbuf(
                                            hatn::common::makeShared<hatn::common::ByteArrayManaged>(buf.data(),dataSize)
                                           );
        server->sendTo(std::move(cbuf),ep,
                       [server,&buf,&ep,&packetsToSend,&txPacketsCount](const hatn::common::Error& ec,size_t size,const hatn::common::SpanBuffer&)
                        {
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

static void sendNextServerScattered(hatn::network::asio::UdpServer* server,
          std::array<char,dataSize>& buf,
          const hatn::network::asio::UdpEndpoint& ep,
          size_t& packetsToSend,
          size_t& txPacketsCount,
          bool append
    )
{
    if (append)
    {
        ++packetsToSend;
    }
    if (packetsToSend>0)
    {
        --packetsToSend;

        hatn::common::SpanBuffers bufs;
        bufs.emplace_back(
                            hatn::common::makeShared<hatn::common::ByteArrayManaged>(buf.data(),dataSize/4)
                    );
        auto sharedBuf=hatn::common::makeShared<hatn::common::ByteArrayManaged>(buf.data(),dataSize);
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
                       [server,&buf,&ep,&packetsToSend,&txPacketsCount](const hatn::common::Error& ec,size_t size,const hatn::common::SpanBuffers&)
                        {
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

static void recvNext(hatn::network::asio::UdpServer* server,
              std::array<char,dataSize>& buf,
              size_t& rxPacketCount,
              size_t& packetsToSend,
              size_t& txPacketsCount
              )
{
    server->receiveFrom(buf.data(),buf.size(),
                            [server,&buf,&rxPacketCount,&packetsToSend,&txPacketsCount](const hatn::common::Error& ec,size_t size,const hatn::network::asio::UdpEndpoint& ep)
                            {
                                if (!ec)
                                {
                                    if (size!=dataSize)
                                    {
                                        BOOST_FAIL("Invalid size of UDP packet");
                                    }

                                    if (++rxPacketCount==TxPacketCount)
                                    {
                                        G_DEBUG("Finished receiving on server");
                                    }

                                    sendNextServer(server,buf,ep,packetsToSend,txPacketsCount,true);
                                    recvNext(server,buf,rxPacketCount,packetsToSend,txPacketsCount);
                                }
                                else
                                {
                                    G_INFO(HATN_FORMAT("Failed to recv {} packet on server: ({}) {}",rxPacketCount,ec.value(),ec.message()));
                                }
                            }
                        );
}
static void recvNextClient(hatn::network::asio::UdpClient* client,
              std::array<char,dataSize>& buf,
              size_t& rxPacketCount
              )
{
    client->receive(buf.data(),buf.size(),
                            [client,&buf,&rxPacketCount](const hatn::common::Error& ec,size_t size)
                            {
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
                                    G_INFO(HATN_FORMAT("Failed to recv {} packet on client: ({}) {}",rxPacketCount,ec.value(),ec.message()));
                                }
                            }
                        );
}

static void sendNextScatter(hatn::network::asio::UdpClient* client,
      std::array<char,dataSize>& buf,
      size_t& txPacketCount,
      hatn::test::MultiThreadFixture* mainLoop
);

static void sendNext(hatn::network::asio::UdpClient* client,
              std::array<char,dataSize>& buf,
              size_t& txPacketCount,
              hatn::test::MultiThreadFixture* mainLoop
              )
{
    if (txPacketCount<TxPacketCount)
    {
        ++txPacketCount;
        hatn::common::SpanBuffer cbuf(hatn::common::makeShared<hatn::common::ByteArrayManaged>(buf.data(),dataSize));
        client->send(std::move(cbuf),
                                [client,&buf,&txPacketCount,mainLoop](const hatn::common::Error& ec,size_t size,const hatn::common::SpanBuffer&)
                                {
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
                                        G_INFO(HATN_FORMAT("Failed to send {} packet: ({}) {}",txPacketCount,ec.value(),ec.message()));
                                    }
                                }
                            );
    }
    else
    {
        {
            G_DEBUG("Finished sending");
        }

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

static void sendNextScatter(hatn::network::asio::UdpClient* client,
              std::array<char,dataSize>& buf,
              size_t& txPacketCount,
              hatn::test::MultiThreadFixture* mainLoop
    )
{
    if (txPacketCount<TxPacketCount)
    {
        ++txPacketCount;

        hatn::common::SpanBuffers bufs;
        bufs.emplace_back(
                            hatn::common::makeShared<hatn::common::ByteArrayManaged>(buf.data(),dataSize/4)
                    );
        auto sharedBuf=hatn::common::makeShared<hatn::common::ByteArrayManaged>(buf.data(),dataSize);
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
                        [client,&buf,&txPacketCount,mainLoop](const hatn::common::Error& ec,size_t size,const hatn::common::SpanBuffers&)
                        {
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
                                G_INFO(HATN_FORMAT("Failed to send {} packet: ({}) {}",txPacketCount,ec.value(),ec.message()));
                            }
                        }
                    );
    }
    else
    {
        {
            G_DEBUG("Finished sending");
        }

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
    hatn::common::MutexLock mutex;

    createThreads(2);
    hatn::common::Thread* thread0=thread(0).get();
    hatn::common::Thread* thread1=thread(1).get();

    hatn::network::asio::UdpEndpoint serverEndpoint;
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
            G_DEBUG("Begin IPv4");
            serverEndpoint.setAddress(boost::asio::ip::address_v4::loopback());
        }
        else
        {
            G_DEBUG("Begin IPv6");
            serverEndpoint.setAddress(boost::asio::ip::address_v6::loopback());
        }

        std::shared_ptr<hatn::network::asio::UdpServer> server;
        std::shared_ptr<hatn::network::asio::UdpClient> client;

        G_DEBUG("Run server");

        HATN_CHECK_EXEC_SYNC(
        thread0->execSync(
                        [&server,&serverEndpoint,&rxPacketCount,&serverRecvBuf,&mutex,&serverPacketsToSend,&serverTxPacketCount,isIpv6]()
                        {
                            server=std::make_shared<hatn::network::asio::UdpServer>(isIpv6?"server-6":"server-4");
                            auto bindOk=[&server,&rxPacketCount,&mutex,&serverRecvBuf,&serverPacketsToSend,&serverTxPacketCount](const hatn::common::Error &ec)
                            {
                                {
                                    hatn::common::MutexScopedLock l(mutex);
                                    BOOST_REQUIRE(!ec);
                                }
                                recvNext(server.get(),serverRecvBuf,rxPacketCount,serverPacketsToSend,serverTxPacketCount);
                            };
                            server->bind(serverEndpoint,bindOk);
                        }
        ));

        auto connectCb=[&thread1,&client,&mutex,&clientSendBuf,&txPacketCount,&clientRecvBuf,&clientRxPacketCount,this](const hatn::common::Error& ec)
        {
            {
                hatn::common::MutexScopedLock l(mutex);
                BOOST_REQUIRE(!ec);
                BOOST_CHECK_EQUAL(thread1,hatn::common::Thread::currentThread());
            }
            G_DEBUG("Start sending ...");
            recvNextClient(client.get(),clientRecvBuf,clientRxPacketCount);
            sendNext(client.get(),clientSendBuf,txPacketCount,this);
        };

        G_DEBUG("Connect client");

        thread1->execAsync(
                        [&client,&serverEndpoint,&connectCb,isIpv6]()
                        {
                            client=std::make_shared<hatn::network::asio::UdpClient>(isIpv6?"client-6":"client-4");
                            client->setRemoteEndpoint(serverEndpoint);
                            client->prepare(connectCb);
                        }
        );

        G_DEBUG("Exec ...");

        exec(20);

        auto closeCb=[&mutex](const hatn::common::Error& ec)
        {
            {
                hatn::common::MutexScopedLock l(mutex);
                BOOST_CHECK(!ec);
            }
        };

        G_DEBUG("Close client");

        HATN_CHECK_EXEC_SYNC(
        thread1->execSync(
                        [&client,&closeCb]()
                        {
                            client->close(closeCb);
                        }
        ));

        G_DEBUG("Close server");

        HATN_CHECK_EXEC_SYNC(
        thread0->execSync(
                        [&closeCb,&server]()
                        {
                            server->close(closeCb);
                        }
        ));

        if (!isIpv6)
        {
            G_DEBUG("End IPv4");
        }
        else
        {
            G_DEBUG("End IPv6");
        }

        {
            hatn::common::MutexScopedLock l(mutex);
            BOOST_CHECK_EQUAL(txPacketCount,TxPacketCount);
            BOOST_CHECK(rxPacketCount>(TxPacketCount/2));
            BOOST_CHECK_EQUAL(rxPacketCount,serverTxPacketCount);
            BOOST_CHECK(clientRxPacketCount>(serverTxPacketCount/2));
        }
        G_INFO(HATN_FORMAT("Server received {} of {} packets",rxPacketCount,TxPacketCount));
        G_INFO(HATN_FORMAT("Client received {} of {} packets",clientRxPacketCount,serverTxPacketCount));
    }

    exec(3);

    thread0->stop();
    thread1->stop();
}
#endif
BOOST_AUTO_TEST_SUITE_END()
