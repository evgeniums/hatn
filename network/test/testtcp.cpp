#include <boost/test/unit_test.hpp>

#include <hatn/common/locker.h>

#include <hatn/common/logger.h>
#include <hatn/test/multithreadfixture.h>

#include <hatn/network/asio/tcpserverconfig.h>
#include <hatn/network/asio/tcpserver.h>
#include <hatn/network/asio/tcpstream.h>

#define HATN_TEST_LOG_CONSOLE

namespace {

static void setLogHandler()
{
    auto handler=[](const ::hatn::common::FmtAllocatedBufferChar &s)
    {
        #ifdef HATN_TEST_LOG_CONSOLE
            std::cout<<::hatn::common::lib::toStringView(s)<<std::endl;
        #else
            std::ignore=s;
        #endif
    };

    ::hatn::common::Logger::setOutputHandler(handler);
    ::hatn::common::Logger::setFatalLogHandler(handler);
    // ::hatn::common::Logger::setDefaultVerbosity(::hatn::common::LoggerVerbosity::DEBUG);
}

struct Env : public ::hatn::test::MultiThreadFixture
{
    Env()
    {
        if (!::hatn::common::Logger::isRunning())
        {
            ::hatn::common::Logger::setFatalTracing(false);

            setLogHandler();
            ::hatn::common::Logger::start();
        }
    }

    ~Env()
    {
        ::hatn::common::Logger::stop();
    }

    Env(const Env&)=delete;
    Env(Env&&) =delete;
    Env& operator=(const Env&)=delete;
    Env& operator=(Env&&) =delete;
};
}

BOOST_AUTO_TEST_SUITE(Tcp)

BOOST_FIXTURE_TEST_CASE(TcpServerCtor,Env)
{
    hatn::network::asio::TcpServer server1(mainThread().get(),"server1");

    hatn::network::asio::TcpServerConfig config(10);
    hatn::network::asio::TcpServer server2(&config,mainThread().get(),"server2");

    BOOST_CHECK_EQUAL(server1.thread(),server2.thread());

    {
        createThreads(2);
        hatn::common::Thread* thread0=thread(0).get();

        thread0->start();
        auto ec=thread0->execSync(
                [&config,thread0]()
                {
                    hatn::network::asio::TcpServer server3("server3");
                    hatn::network::asio::TcpServer server4(&config,"server4");
                    hatn::network::asio::TcpServer server5(&config,thread0,"server5");
                    BOOST_CHECK_EQUAL(server3.thread(),server4.thread());
                    BOOST_CHECK_EQUAL(server4.thread(),server5.thread());
                }
        );
        BOOST_REQUIRE(!ec);
        thread0->stop();
    }
}

BOOST_FIXTURE_TEST_CASE(TcpServerListen,Env)
{
    uint16_t portNumber1=11511;
    uint16_t portNumber2=11712;
    uint16_t portNumber3=11513;
    uint16_t portNumber4=11714;

    createThreads(2);
    hatn::common::Thread* thread0=thread(0).get();
    hatn::common::Thread* thread1=thread(1).get();

    {
        std::shared_ptr<hatn::network::asio::TcpServer> server1,server2,server3,server4;
        std::shared_ptr<hatn::network::asio::TcpServer> server5,server6,server7,server8;

        thread0->start();
        thread1->start();

        HATN_CHECK_EXEC_SYNC(
        thread0->execSync(
                        [&server1,&server2,&server3,&server4,portNumber1,portNumber2,portNumber3,portNumber4]()
                        {
                            server1=std::make_shared<hatn::network::asio::TcpServer>("server1");
                            hatn::network::asio::TcpEndpoint ep1("127.0.0.1",portNumber1);
                            auto ec=server1->listen(ep1);
                            BOOST_CHECK(!ec);

                            server2=std::make_shared<hatn::network::asio::TcpServer>("server2");
                            hatn::network::asio::TcpEndpoint ep2(boost::asio::ip::address_v4::any(),portNumber2);
                            ec=server2->listen(ep2);
                            BOOST_CHECK(!ec);

                            server3=std::make_shared<hatn::network::asio::TcpServer>("server3");
                            hatn::network::asio::TcpEndpoint ep3(boost::asio::ip::address_v6::loopback(),portNumber3);
                            ec=server3->listen(ep3);
                            BOOST_CHECK(!ec);

                            server4=std::make_shared<hatn::network::asio::TcpServer>("server4");
                            hatn::network::asio::TcpEndpoint ep4(boost::asio::ip::address_v6::any(),portNumber4);
                            ec=server4->listen(ep4);
                            BOOST_CHECK(!ec);
                        }
                    ));

        HATN_CHECK_EXEC_SYNC(
        thread1->execSync(
                        [&server5,&server6,&server7,&server8,portNumber1,portNumber2,portNumber3,portNumber4]()
                        {
                            server5=std::make_shared<hatn::network::asio::TcpServer>("server5");
                            hatn::network::asio::TcpEndpoint ep1("127.0.0.1",portNumber1);
                            auto ec=server5->listen(ep1);
                            BOOST_CHECK(ec);
                            BOOST_CHECK_EQUAL(ec.errorCondition(),boost::system::errc::address_in_use);

                            server6=std::make_shared<hatn::network::asio::TcpServer>("server6");
                            hatn::network::asio::TcpEndpoint ep2(boost::asio::ip::address_v4::any(),portNumber2);
                            ec=server6->listen(ep2);
                            BOOST_CHECK(ec);
                            BOOST_CHECK_EQUAL(ec.errorCondition(),boost::system::errc::address_in_use);

                            server7=std::make_shared<hatn::network::asio::TcpServer>("server7");
                            hatn::network::asio::TcpEndpoint ep3(boost::asio::ip::address_v6::loopback(),portNumber3);
                            ec=server7->listen(ep3);
                            BOOST_CHECK(ec);
                            BOOST_CHECK_EQUAL(ec.errorCondition(),boost::system::errc::address_in_use);

                            server8=std::make_shared<hatn::network::asio::TcpServer>("server8");
                            hatn::network::asio::TcpEndpoint ep4(boost::asio::ip::address_v6::any(),portNumber4);
                            ec=server8->listen(ep4);
                            BOOST_CHECK(ec);
                            BOOST_CHECK_EQUAL(ec.errorCondition(),boost::system::errc::address_in_use);
                        }
                    ));

        HATN_CHECK_EXEC_SYNC(
        thread0->execSync(
                        [&server1,&server2,&server3,&server4]()
                        {
                            auto ec=server1->close();
                            BOOST_CHECK(!ec);

                            ec=server2->close();
                            BOOST_CHECK(!ec);

                            ec=server3->close();
                            BOOST_CHECK(!ec);

                            ec=server4->close();
                            BOOST_CHECK(!ec);
                        }
                    ));

        thread0->stop();
        thread1->stop();
    }
}

static uint16_t genPortNumber()
{
    srand(static_cast<unsigned int>(time(NULL)));
    uint16_t minPort=9124;
    uint16_t maxPort=31000;
    return (minPort + (rand() % static_cast<uint16_t>(maxPort - minPort + 1)));
}

BOOST_FIXTURE_TEST_CASE(TcpServerAccept,Env)
{
    hatn::common::MutexLock mutex;

    {
        createThreads(2);
        hatn::common::Thread* thread0=thread(0).get();
        hatn::common::Thread* thread1=thread(1).get();

        hatn::network::asio::TcpEndpoint serverEndpoint;
        serverEndpoint.setPort(9123);

        hatn::network::asio::TcpEndpoint client2Endpoint;
        client2Endpoint.setPort(genPortNumber());

        thread0->start();
        thread1->start();

        for (int i=0;i<2;i++)
        {
            bool isIpv6=i==1;
            if (!isIpv6)
            {
                G_DEBUG("Begin IPv4");
                serverEndpoint.setAddress(boost::asio::ip::address_v4::loopback());
                client2Endpoint.setAddress(boost::asio::ip::address_v4::loopback());
            }
            else
            {
                G_DEBUG("Begin IPv6");
                serverEndpoint.setAddress(boost::asio::ip::address_v6::loopback());
                client2Endpoint.setAddress(boost::asio::ip::address_v6::loopback());
            }
            std::shared_ptr<hatn::network::asio::TcpServer> server;

            std::shared_ptr<hatn::network::asio::TcpStream> client1,client2,serverClient1,serverClient2;

            std::atomic<int> acceptCount(0);
            std::atomic<int> connectCount(0);
            std::atomic<int> closeCount(0);

            auto acceptCb2=[&thread0,&acceptCount,&mutex](const hatn::common::Error& ec)
            {
                {
                    hatn::common::MutexScopedLock l(mutex);

                    BOOST_CHECK(!ec);
                    BOOST_CHECK_EQUAL(thread0,hatn::common::Thread::currentThread());
                }

                ++acceptCount;
            };

            auto acceptCb1=[&thread0,&mutex,&server,&serverClient2,&acceptCb2,&acceptCount,isIpv6](const hatn::common::Error& ec)
            {
                {
                    hatn::common::MutexScopedLock l(mutex);
                    BOOST_REQUIRE(!ec);
                }

                serverClient2=std::make_shared<hatn::network::asio::TcpStream>(isIpv6?"serverClient2-6":"serverClient2-4");
                server->accept(serverClient2->socket(),acceptCb2);
                ++acceptCount;

                {
                    hatn::common::MutexScopedLock l(mutex);
                    BOOST_CHECK_EQUAL(thread0,hatn::common::Thread::currentThread());
                    BOOST_CHECK_EQUAL(thread0,serverClient2->thread());
                }
            };

            G_DEBUG("Run server");

            HATN_CHECK_EXEC_SYNC(
                thread0->execSync(
                    [&server,&mutex,&serverEndpoint,&serverClient1,&acceptCb1,isIpv6]()
                    {
                        server=std::make_shared<hatn::network::asio::TcpServer>(isIpv6?"server-6":"server-4");
                        auto ec=server->listen(serverEndpoint);
                        {
                            hatn::common::MutexScopedLock l(mutex);
                            BOOST_REQUIRE(!ec);
                        }

                        serverClient1=std::make_shared<hatn::network::asio::TcpStream>(isIpv6?"serverClient1-6":"serverClient1-4");
                        server->accept(serverClient1->socket(),acceptCb1);
                    }
                    ));

            auto connectCb=[&thread1,&mutex,&connectCount](const hatn::common::Error& ec)
            {
                {
                    hatn::common::MutexScopedLock l(mutex);
                    BOOST_CHECK(!ec);
                    BOOST_CHECK_EQUAL(thread1,hatn::common::Thread::currentThread());
                }
                ++connectCount;
            };

            G_DEBUG("Connect clients");

            thread1->execAsync(
                [&client1,&client2,&serverEndpoint,&client2Endpoint,&connectCb,isIpv6]()
                {
                    client1=std::make_shared<hatn::network::asio::TcpStream>(isIpv6?"client1-6":"client1-4");
                    client1->setRemoteEndpoint(serverEndpoint);
                    client1->prepare(connectCb);

                    client2=std::make_shared<hatn::network::asio::TcpStream>(isIpv6?"client2-6":"client2-4");
                    client2->setRemoteEndpoint(serverEndpoint);
                    client2->setLocalEndpoint(client2Endpoint);
                    client2->prepare(connectCb);
                }
                );

            G_DEBUG("Exec ...");

            exec(1);

            auto closeCb=[&closeCount,&mutex](const hatn::common::Error& ec)
            {
                {
                    hatn::common::MutexScopedLock l(mutex);
                    BOOST_CHECK(!ec);
                }
                ++closeCount;
            };

            G_DEBUG("Close clients");

            HATN_CHECK_EXEC_SYNC(
                thread1->execSync(
                    [&client1,&client2,&closeCb]()
                    {
                        client1->close(closeCb);
                        client2->close(closeCb);
                    }
                    ));

            G_DEBUG("Close server");

            HATN_CHECK_EXEC_SYNC(
                thread0->execSync(
                    [&serverClient1,&mutex,&serverClient2,&closeCb,&server]()
                    {
                        serverClient1->close(closeCb);
                        if (serverClient2)
                        {
                            serverClient2->close(closeCb);
                        }

                        auto ec=server->close();
                        {
                            hatn::common::MutexScopedLock l(mutex);
                            BOOST_CHECK(!ec);
                        }
                    }
                    ));

            {
                hatn::common::MutexScopedLock l(mutex);

                BOOST_CHECK_EQUAL(acceptCount,2);
                BOOST_CHECK_EQUAL(connectCount,2);
                BOOST_CHECK_EQUAL(closeCount,4);
            }

            if (!isIpv6)
            {
                G_DEBUG("End IPv4");
            }
            else
            {
                G_DEBUG("End IPv6");
            }
        }

        thread0->stop();
        thread1->stop();
    }
}
constexpr static const size_t dataSize=0x20000;

static void readNext(const hatn::common::Error& ec,
                     size_t size,
                     hatn::network::asio::TcpStream* client,
                     size_t& recvSize,
                     std::array<char,dataSize>& recvBuf,
                     const std::function<void()>& checkDone,
                     hatn::common::MutexLock& mutex
                     )
{
    if (ec)
    {
        checkDone();
        return;
    }

    recvSize+=size;
    if (recvSize>=dataSize)
    {
        G_DEBUG("Read done "<<client->id().c_str()<<" recvSize="<<recvSize);

        checkDone();
        return;
    }
    size_t space=0x1000;
    if ((recvSize+space)>dataSize)
    {
        space=dataSize-recvSize;
    }
    G_DEBUG("Read "<<client->id().c_str()<<" space="<<space<<" recvSize="<<recvSize);

    client->read(recvBuf.data()+recvSize,space,
                       [client,&recvSize,&recvBuf,checkDone,&mutex](const hatn::common::Error& ec1, size_t rxSize)
                       {
                            G_DEBUG("Read cb recvSize="<<rxSize<<" ec1="<<ec1.value());

                            readNext(
                                        ec1,rxSize,client,recvSize,recvBuf,checkDone,mutex
                                    );
                       }
                );
}

BOOST_FIXTURE_TEST_CASE(TcpSendRecv,Env)
{
    hatn::common::MutexLock mutex;

    createThreads(2);
    hatn::common::Thread* thread0=thread(0).get();
    hatn::common::Thread* thread1=thread(1).get();

    hatn::network::asio::TcpEndpoint serverEndpoint;
    serverEndpoint.setPort(19123);

    thread0->start();
    thread1->start();

    for (int i=0;i<2;i++)
    {
        std::array<char,dataSize> clientSendBuf,serverSendBuf;
        std::array<char,dataSize> clientRecvBuf,serverRecvBuf;

        size_t clientRxSize=0;
        size_t serverRxSize=0;
        randBuf(clientSendBuf);
        randBuf(serverSendBuf);

        hatn::common::ByteArrayShared clientSendBuf1(
                        new hatn::common::ByteArrayManaged(clientSendBuf.data(),dataSize/4)
                    );
        hatn::common::ByteArrayShared clientSendBuf2(
                        new hatn::common::ByteArrayManaged(clientSendBuf.data(),clientSendBuf.size())
                    );

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

        std::atomic<int> rxDoneCount(0);
        auto checkDone=[&rxDoneCount,this]()
        {
            if (++rxDoneCount==2)
            {
                quit();
            }
        };

        std::shared_ptr<hatn::network::asio::TcpServer> server;
        std::shared_ptr<hatn::network::asio::TcpStream> client,serverClient;

        auto acceptCb=[&mutex,&serverClient,&serverSendBuf,&serverRecvBuf,&checkDone,&serverRxSize](const hatn::common::Error& ec)
        {
            {
                hatn::common::MutexScopedLock l(mutex);
                BOOST_REQUIRE(!ec);
            }
            serverClient->write(serverSendBuf.data(),serverSendBuf.size(),
                                [&mutex](const hatn::common::Error& ec1, size_t size)
                                {
                                    hatn::common::MutexScopedLock l(mutex);
                                    BOOST_REQUIRE(!ec1);
                                    BOOST_CHECK_EQUAL(size,dataSize);
                                }
                        );
            readNext(ec,0,serverClient.get(),serverRxSize,serverRecvBuf,checkDone,mutex);
        };

        G_DEBUG("Run server");

        HATN_CHECK_EXEC_SYNC(
        thread0->execSync(
                        [&server,&mutex,&serverEndpoint,&serverClient,&acceptCb,isIpv6]()
                        {
                            server=std::make_shared<hatn::network::asio::TcpServer>(isIpv6?"server-6":"server-4");
                            auto ec=server->listen(serverEndpoint);
                            {
                                hatn::common::MutexScopedLock l(mutex);
                                BOOST_REQUIRE(!ec);
                            }

                            serverClient=std::make_shared<hatn::network::asio::TcpStream>(isIpv6?"serverClient-6":"serverClient-4");
                            server->accept(serverClient->socket(),acceptCb);
                        }
        ));

        auto connectCb=[&thread1,&client,&checkDone,&mutex,
                        clientSendBuf1,clientSendBuf2,
                        &clientRecvBuf,&clientRxSize](const hatn::common::Error& ec)
        {
            {
                hatn::common::MutexScopedLock l(mutex);
                BOOST_REQUIRE(!ec);
                BOOST_CHECK_EQUAL(thread1,hatn::common::Thread::currentThread());
            }

            hatn::common::SpanBuffers bufs;
            bufs.push_back(
                            hatn::common::SpanBuffer(clientSendBuf1)
                        );
            size_t offset=dataSize/4;
            bufs.push_back(
                            hatn::common::SpanBuffer(clientSendBuf2,offset,dataSize/4)
                        );
            offset+=dataSize/4;
            bufs.push_back(
                            hatn::common::SpanBuffer(clientSendBuf2,offset,dataSize/4)
                        );
            offset+=dataSize/4;

            client->write(std::move(bufs),
                                [&mutex,client,offset,clientSendBuf2](const hatn::common::Error& ec1, size_t size, const hatn::common::SpanBuffers&)
                                {
                                    {
                                        hatn::common::MutexScopedLock l(mutex);
                                        BOOST_REQUIRE(!ec1);
                                        BOOST_CHECK_EQUAL(size,dataSize-dataSize/4);
                                    }
                                    auto&& cb1=[&mutex](const hatn::common::Error& ec2, size_t size2, const hatn::common::SpanBuffer&)
                                    {
                                        hatn::common::MutexScopedLock l(mutex);
                                        BOOST_REQUIRE(!ec2);
                                        BOOST_CHECK_EQUAL(size2,dataSize/4);
                                    };

                                    hatn::common::SpanBuffer buf(clientSendBuf2,offset,dataSize/4);
                                    client->write(std::move(buf),cb1);
                                }
                        );
            readNext(ec,0,client.get(),clientRxSize,clientRecvBuf,checkDone,mutex);
        };

        G_DEBUG("Connect clients");

        thread1->execAsync(
                        [&client,&serverEndpoint,&connectCb,isIpv6]()
                        {
                            client=std::make_shared<hatn::network::asio::TcpStream>(isIpv6?"client-6":"client-4");
                            client->setRemoteEndpoint(serverEndpoint);
                            client->prepare(connectCb);
                        }
        );

        G_DEBUG("Exec ...");

        exec(10);

        auto closeCb=[&mutex](const hatn::common::Error& ec)
        {
            {
                hatn::common::MutexScopedLock l(mutex);
                BOOST_CHECK(!ec);
            }
        };

        G_DEBUG("Close clients");

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
                        [&serverClient,&mutex,&closeCb,&server]()
                        {
                            serverClient->close(closeCb);
                            auto ec=server->close();
                            {
                                hatn::common::MutexScopedLock l(mutex);
                                BOOST_CHECK(!ec);
                            }
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
            BOOST_REQUIRE_EQUAL(serverRxSize,dataSize);
            BOOST_REQUIRE_EQUAL(clientRxSize,dataSize);
            BOOST_CHECK(clientSendBuf==serverRecvBuf);
            BOOST_CHECK(clientRecvBuf==serverSendBuf);
        }

    }

    thread0->stop();
    thread1->stop();
}

BOOST_AUTO_TEST_SUITE_END()
