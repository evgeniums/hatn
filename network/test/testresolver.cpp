#include <boost/test/unit_test.hpp>

#include <hatn/common/locker.h>

#include <hatn/common/logger.h>
#include <hatn/test/multithreadfixture.h>

#include <hatn/network/asio/caresolver.h>

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
}

struct Env : public ::hatn::test::MultiThreadFixture
{
    Env()
    {
        if (!::hatn::common::Logger::isRunning())
        {
            ::hatn::common::Logger::setFatalTracing(false);

            ::hatn::common::Logger::start();
        }
        else
        {
            BOOST_REQUIRE(::hatn::common::Logger::isSeparateThread());
        }
        setLogHandler();
        std::vector<std::string> modules={"dnsresolver;debug;1","global;debug;1"};
        ::hatn::common::Logger::configureModules(modules);

        std::ignore=::hatn::network::CaresLib::init();
    }

    ~Env()
    {
        ::hatn::network::CaresLib::cleanup();
        ::hatn::common::Logger::stop();
        ::hatn::common::Logger::resetModules();
    }

    Env(const Env&)=delete;
    Env(Env&&) noexcept=delete;
    Env& operator=(const Env&)=delete;
    Env& operator=(Env&&) noexcept=delete;
};
}

BOOST_AUTO_TEST_SUITE(Resolver)

BOOST_FIXTURE_TEST_CASE(ResolveNotFound,Env)
{
    std::shared_ptr<hatn::network::asio::CaResolver> resolver;

    createThreads(1);
    hatn::common::Thread* thread0=thread(0).get();

    HATN_REQUIRE_TS(thread0!=nullptr);

    size_t doneCbCount=0;
    size_t doneCbCountMax=3;
    auto checkDone=[&doneCbCount,&doneCbCountMax,this]()
    {
        G_DEBUG(fmt::format("ResolveNotFound: Check done {}/{}",doneCbCount,doneCbCountMax));

        if (++doneCbCount>=doneCbCountMax)
        {
            G_DEBUG("Quit ResolveNotFound");
            quit();
        }
    };

    // codechecker_false_positive [all] Never thread0 is nullptr or don't care
    thread0->start();

    thread0->execAsync(
                    [&resolver,&checkDone]()
                    {
                        auto notFoundCb=[&checkDone](const hatn::common::Error& ec,const std::vector<hatn::network::asio::IpEndpoint>&)
                        {
                            G_DEBUG("ResolveNotFound: Not found cb");

                            HATN_CHECK_TS(ec);
                            auto notFound1=4;
                            auto notFound2=1;
                            bool ok=ec.value()==notFound1 || ec.value()==notFound2;
                            HATN_CHECK_TS(ok);
                            if (!ok)
                            {
                                G_WARN("Not found error: "<<ec.message());
                            }
                            checkDone();
                        };

                        resolver=std::make_shared<hatn::network::asio::CaResolver>("resolver");
                        resolver->resolveName("notfound.hatn.com",notFoundCb);
                        resolver->resolveService("_notfound._tcp.hatn.com",notFoundCb);
                        resolver->resolveMx("notfound-mx.hatn.com",notFoundCb);
                    }
                );

    exec(20);

    G_DEBUG("ResolveNotFound exec() done");

    HATN_CHECK_EXEC_SYNC(
    thread0->execSync(
                    [&resolver]()
                    {
                        resolver.reset();
                    }));

    exec(1);

    G_DEBUG("ResolveNotFound exec(1) done");

    thread0->stop();

    HATN_CHECK_EQUAL_TS(doneCbCountMax,doneCbCount);
}

void checkResolve(Env* env, bool useGoogleServer)
{
    G_DEBUG("checkResolve: enter");

    std::shared_ptr<hatn::network::asio::CaResolver> resolver;

    env->createThreads(1);
    hatn::common::Thread* thread0=env->thread(0).get();

    HATN_REQUIRE_TS(thread0!=nullptr);

    size_t doneCbCount=0;
#ifdef DNS_RESOLVE_LOCAL_HOSTS
    size_t doneCbCountMax=4;
#else
    size_t doneCbCountMax=3;
#endif
    auto checkDone=[&doneCbCount,&doneCbCountMax,env]()
    {
        G_DEBUG(fmt::format("checkResolve: Check done {}/{}",doneCbCount,doneCbCountMax));

        if (++doneCbCount==doneCbCountMax)
        {
            G_DEBUG("checkResolve: Check done quit");

            env->quit();
        }
    };

    // codechecker_false_positive [all] Never thread0 is nullptr or don't care
    thread0->start();

    thread0->execAsync(
                    [&resolver,&checkDone,useGoogleServer]()
                    {
                        auto checkEc=[](const hatn::common::Error& ec)
                        {
                            HATN_CHECK_TS(!ec);
                            if (ec)
                            {
                                G_WARN("Resolving failed: "<<ec.message());
                            }
                        };

                        auto foundCb1=[&checkDone,checkEc](const hatn::common::Error& ec,std::vector<hatn::network::asio::IpEndpoint> endpoints)
                        {
                            G_DEBUG("Done foundCb1");
                            checkEc(ec);

                            std::set<hatn::network::asio::IpEndpoint> eps={
                                hatn::network::asio::IpEndpoint("8.8.8.8",80),
                                hatn::network::asio::IpEndpoint("1.1.1.1",80),
                                hatn::network::asio::IpEndpoint("2001:4860:4860::8888",80),
                                hatn::network::asio::IpEndpoint("2001:4860:4860::8844",80)
                            };

                            HATN_CHECK_EQUAL_TS(eps.size(),endpoints.size());
                            for (size_t i=0;i<endpoints.size();i++)
                            {
                                HATN_CHECK_TS(eps.find(endpoints[i])!=eps.end());
                                G_DEBUG(HATN_FORMAT("Endpoint {}:{}",endpoints[i].address().to_string(),endpoints[i].port()));
                            }

                            checkDone();
                        };

                        auto foundCb2=[&checkDone,checkEc](const hatn::common::Error& ec,std::vector<hatn::network::asio::IpEndpoint> endpoints)
                        {
                            G_DEBUG("Done foundCb2");
                            checkEc(ec);

                            std::set<hatn::network::asio::IpEndpoint> eps={
                                hatn::network::asio::IpEndpoint("4.4.4.4",11555),
                                hatn::network::asio::IpEndpoint("1.1.1.1",12345),
                                hatn::network::asio::IpEndpoint("8.8.8.8",12345),
                                hatn::network::asio::IpEndpoint("2001:4860:4860::8888",12345),
                                hatn::network::asio::IpEndpoint("2001:4860:4860::8844",12345)
                            };

                            HATN_CHECK_EQUAL_TS(eps.size(),endpoints.size());
                            for (size_t i=0;i<endpoints.size();i++)
                            {
                                HATN_CHECK_TS(eps.find(endpoints[i])!=eps.end());
                                G_DEBUG(HATN_FORMAT("Endpoint {}:{}",endpoints[i].address().to_string(),endpoints[i].port()));
                            }

                            checkDone();
                        };

                        auto foundCb3=[&checkDone,checkEc](const hatn::common::Error& ec,std::vector<hatn::network::asio::IpEndpoint> endpoints)
                        {
                            G_DEBUG("Done foundCb3");

                            checkEc(ec);

                            std::set<hatn::network::asio::IpEndpoint> eps={
                                hatn::network::asio::IpEndpoint("172.217.23.133",0),
                                hatn::network::asio::IpEndpoint("fe80::922b:34ff:fe7b:6ff1",0)
                            };

                            HATN_CHECK_EQUAL_TS(eps.size(),endpoints.size());
                            for (size_t i=0;i<endpoints.size();i++)
                            {
                                HATN_CHECK_TS(eps.find(endpoints[i])!=eps.end());
                                G_DEBUG(HATN_FORMAT("Endpoint {}:{}",endpoints[i].address().to_string(),endpoints[i].port()));
                            }

                            checkDone();
                        };
#ifdef DNS_RESOLVE_LOCAL_HOSTS
                        auto foundCb4=[&checkDone,checkEc](const hatn::common::Error& ec,std::vector<hatn::network::asio::IpEndpoint> endpoints)
                        {
                            checkEc(ec);

                            std::set<hatn::network::asio::IpEndpoint> eps={
                                hatn::network::asio::IpEndpoint("192.168.111.151",22),
                                hatn::network::asio::IpEndpoint("::1",22)
                            };

                            HATN_CHECK_EQUAL_TS(eps.size(),endpoints.size());
                            for (size_t i=0;i<endpoints.size();i++)
                            {
                                HATN_CHECK_TS(eps.find(endpoints[i])!=eps.end());
                                G_DEBUG(HATN_FORMAT("Endpoint {}:{}",endpoints[i].address().to_string(),endpoints[i].port()));
                            }

                            checkDone();
                        };
#endif
                        if (useGoogleServer)
                        {
                            std::vector<hatn::network::NameServer> nameServers={"8.8.8.8"};//{"91.121.209.194"};
                            resolver=std::make_shared<hatn::network::asio::CaResolver>(nameServers,"resolver");
                        }
                        else
                        {
                            resolver=std::make_shared<hatn::network::asio::CaResolver>("resolver");
                        }
                        resolver->resolveName("r-test2.hatn.org",foundCb1,80);
                        resolver->resolveService("_test._udp.hatn.org",foundCb2);
                        resolver->resolveMx("hatn.org",foundCb3);
#ifdef DNS_RESOLVE_LOCAL_HOSTS
                        resolver->resolveName("test1.local",foundCb4,22);
#endif
                    }
                );

    env->exec(20);

    G_DEBUG("checkResolve: after exec1");

    HATN_CHECK_EXEC_SYNC(
    thread0->execSync(
                    [&resolver]()
                    {
                        G_DEBUG("checkResolve: reset resolver");

                        resolver.reset();
                    }));

    G_DEBUG("checkResolve: after exec2");

    thread0->stop();

    HATN_CHECK_EQUAL_TS(doneCbCountMax,doneCbCount);
}

BOOST_FIXTURE_TEST_CASE(ResolveDefaultServer,Env)
{
    checkResolve(this,false);
}

BOOST_FIXTURE_TEST_CASE(ResolveGoogleServer,Env)
{
    checkResolve(this,true);
}

BOOST_AUTO_TEST_SUITE_END()
