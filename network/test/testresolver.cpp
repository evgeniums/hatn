#include <boost/test/unit_test.hpp>

#include <hatn/common/locker.h>

#include <hatn/logcontext/context.h>
#include <hatn/logcontext/contextlogger.h>

#include <hatn/test/multithreadfixture.h>

#include <hatn/network/asio/caresolver.h>
#include <hatn/network/resolvershuffle.h>

#define HATN_TEST_LOG_CONSOLE

namespace {

struct Env : public HATN_TEST_NAMESPACE::MultiThreadFixture
{
    Env()
    {
        std::ignore=::HATN_NETWORK_NAMESPACE::CaresLib::init();
    }

    ~Env()
    {
        ::HATN_NETWORK_NAMESPACE::CaresLib::cleanup();
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
    std::shared_ptr<HATN_NETWORK_NAMESPACE::asio::CaResolver> resolver;
    auto context=HATN_LOGCONTEXT_NAMESPACE::makeLogCtx("resolver");

    createThreads(1);
    HATN_COMMON_NAMESPACE::Thread* thread0=thread(0).get();

    HATN_REQUIRE_TS(thread0!=nullptr);

    size_t doneCbCount=0;
    size_t doneCbCountMax=3;
    auto checkDone=[&doneCbCount,&doneCbCountMax,this]()
    {
        HATN_TEST_MESSAGE_TS(fmt::format("ResolveNotFound: check done {}/{}",doneCbCount,doneCbCountMax));

        if (++doneCbCount>=doneCbCountMax)
        {
            HATN_TEST_MESSAGE_TS("quit ResolveNotFound");
            quit();
        }
    };

    // codechecker_false_positive [all] Never thread0 is nullptr or don't care
    thread0->start();

    thread0->execAsync(
                    [&resolver,&checkDone,context]()
                    {
                        auto notFoundCb=[&checkDone](const HATN_COMMON_NAMESPACE::Error& ec,const std::vector<HATN_NETWORK_NAMESPACE::asio::IpEndpoint>&)
                        {
                            HATN_TEST_MESSAGE_TS("ResolveNotFound: not found cb");

                            HATN_CHECK_TS(ec);
                            bool ok=ec.is(HATN_NETWORK_NAMESPACE::CaresError::ARES_ENODATA)
                                      ||
                                    ec.is(HATN_NETWORK_NAMESPACE::CaresError::ARES_ENOTFOUND);
                            if (!ok)
                            {
                                HATN_TEST_MESSAGE_TS("not found error("<< ec.codeString() << "):" <<ec.message());
                            }
                            HATN_CHECK_TS(ok);
                            checkDone();
                        };

                        resolver=std::make_shared<HATN_NETWORK_NAMESPACE::asio::CaResolver>();
                        resolver->resolveName("notfound.decfile.com",notFoundCb,context);
                        resolver->resolveService("_notfound._tcp.decfile.com",notFoundCb,context);
                        resolver->resolveMx("notfound-mx.decfile.com",notFoundCb,context);
                    }
                );

    exec(20);

    HATN_TEST_MESSAGE_TS("ResolveNotFound exec() done");

    resolver->cancel();
    exec(1);
    HATN_TEST_MESSAGE_TS("ResolveNotFound exec(1) done");

    HATN_CHECK_EXEC_SYNC(
    thread0->execSync(
                    [&resolver]()
                    {
                        resolver.reset();
                    }));        

    thread0->stop();

    HATN_CHECK_EQUAL_TS(doneCbCountMax,doneCbCount);
}

#if 1
void checkResolve(Env* env, bool useGoogleServer)
{
    //! @todo change addresses to addresses of private networks

    HATN_TEST_MESSAGE_TS("checkResolve: enter");

    std::shared_ptr<HATN_NETWORK_NAMESPACE::asio::CaResolver> resolver;
    auto context=HATN_LOGCONTEXT_NAMESPACE::makeLogCtx("resolver");

    env->createThreads(1);
    HATN_COMMON_NAMESPACE::Thread* thread0=env->thread(0).get();

    HATN_REQUIRE_TS(thread0!=nullptr);

    size_t doneCbCount=0;
#ifdef DNS_RESOLVE_LOCAL_HOSTS
    size_t doneCbCountMax=4;
#else
    size_t doneCbCountMax=3;
#endif
    auto checkDone=[&doneCbCount,&doneCbCountMax,env]()
    {
        HATN_TEST_MESSAGE_TS(fmt::format("checkResolve: Check done {}/{}",doneCbCount+1,doneCbCountMax));

        if (++doneCbCount==doneCbCountMax)
        {
            HATN_TEST_MESSAGE_TS("checkResolve: Check done quit");

            env->quit();
        }
    };

    // codechecker_false_positive [all] Never thread0 is nullptr or don't care
    thread0->start();

    thread0->execAsync(
                    [&resolver,&checkDone,context,useGoogleServer]()
                    {
                        auto checkEc=[](const HATN_COMMON_NAMESPACE::Error& ec)
                        {
                            HATN_CHECK_TS(!ec);
                            if (ec)
                            {
                                HATN_TEST_MESSAGE_TS("Resolving failed: "<<ec.message());
                            }
                        };

                        auto foundCb1=[&checkDone,checkEc](const HATN_COMMON_NAMESPACE::Error& ec,std::vector<HATN_NETWORK_NAMESPACE::asio::IpEndpoint> endpoints)
                        {
                            G_DEBUG("Done foundCb1");
                            checkEc(ec);

                            std::set<HATN_NETWORK_NAMESPACE::asio::IpEndpoint> eps={
                                HATN_NETWORK_NAMESPACE::asio::IpEndpoint("8.8.8.8",80),
                                HATN_NETWORK_NAMESPACE::asio::IpEndpoint("1.1.1.1",80),
                                HATN_NETWORK_NAMESPACE::asio::IpEndpoint("2001:4860:4860::8888",80),
                                HATN_NETWORK_NAMESPACE::asio::IpEndpoint("2001:4860:4860::8844",80)
                            };

                            HATN_CHECK_EQUAL_TS(eps.size(),endpoints.size());
                            for (size_t i=0;i<endpoints.size();i++)
                            {
                                HATN_CHECK_TS(eps.find(endpoints[i])!=eps.end());
                                HATN_TEST_MESSAGE_TS(fmt::format("Endpoint {}:{}",endpoints[i].address().to_string(),endpoints[i].port()));
                            }

                            checkDone();
                        };

                        auto foundCb2=[&checkDone,checkEc](const HATN_COMMON_NAMESPACE::Error& ec,std::vector<HATN_NETWORK_NAMESPACE::asio::IpEndpoint> endpoints)
                        {
                            HATN_TEST_MESSAGE_TS("Done foundCb2");
                            checkEc(ec);

                            std::set<HATN_NETWORK_NAMESPACE::asio::IpEndpoint> eps={
                                HATN_NETWORK_NAMESPACE::asio::IpEndpoint("4.4.4.4",11555),
                                HATN_NETWORK_NAMESPACE::asio::IpEndpoint("1.1.1.1",12345),
                                HATN_NETWORK_NAMESPACE::asio::IpEndpoint("8.8.8.8",12345),
                                HATN_NETWORK_NAMESPACE::asio::IpEndpoint("2001:4860:4860::8888",12345),
                                HATN_NETWORK_NAMESPACE::asio::IpEndpoint("2001:4860:4860::8844",12345)
                            };

                            HATN_CHECK_EQUAL_TS(eps.size(),endpoints.size());
                            for (size_t i=0;i<endpoints.size();i++)
                            {
                                HATN_CHECK_TS(eps.find(endpoints[i])!=eps.end());
                                HATN_TEST_MESSAGE_TS(fmt::format("Endpoint {}:{}",endpoints[i].address().to_string(),endpoints[i].port()));
                            }

                            checkDone();
                        };

                        auto foundCb3=[&checkDone,checkEc](const HATN_COMMON_NAMESPACE::Error& ec,std::vector<HATN_NETWORK_NAMESPACE::asio::IpEndpoint> endpoints)
                        {
                            HATN_TEST_MESSAGE_TS("Done foundCb3");

                            checkEc(ec);

                            std::set<HATN_NETWORK_NAMESPACE::asio::IpEndpoint> eps={
                                HATN_NETWORK_NAMESPACE::asio::IpEndpoint("192.187.101.109",0),
                                HATN_NETWORK_NAMESPACE::asio::IpEndpoint("fd00:c832:63d5::6ff1",0)
                            };

                            HATN_CHECK_EQUAL_TS(eps.size(),endpoints.size());
                            for (size_t i=0;i<endpoints.size();i++)
                            {
                                HATN_CHECK_TS(eps.find(endpoints[i])!=eps.end());
                                HATN_TEST_MESSAGE_TS(fmt::format("Endpoint {}:{}",endpoints[i].address().to_string(),endpoints[i].port()));
                            }

                            checkDone();
                        };
#ifdef DNS_RESOLVE_LOCAL_HOSTS
                        auto foundCb4=[&checkDone,checkEc](const HATN_COMMON_NAMESPACE::Error& ec,std::vector<HATN_NETWORK_NAMESPACE::asio::IpEndpoint> endpoints)
                        {
                            checkEc(ec);

                            std::set<HATN_NETWORK_NAMESPACE::asio::IpEndpoint> eps={
                                HATN_NETWORK_NAMESPACE::asio::IpEndpoint("192.168.111.151",22),
                                HATN_NETWORK_NAMESPACE::asio::IpEndpoint("::1",22)
                            };

                            HATN_CHECK_EQUAL_TS(eps.size(),endpoints.size());
                            for (size_t i=0;i<endpoints.size();i++)
                            {
                                HATN_CHECK_TS(eps.find(endpoints[i])!=eps.end());
                                HATN_TEST_MESSAGE_TS(fmt::format("Endpoint {}:{}",endpoints[i].address().to_string(),endpoints[i].port()));
                            }

                            checkDone();
                        };
#endif
                        if (useGoogleServer)
                        {
                            std::vector<HATN_NETWORK_NAMESPACE::NameServer> nameServers={"8.8.8.8"};//{"91.121.209.194"};
                            resolver=std::make_shared<HATN_NETWORK_NAMESPACE::asio::CaResolver>(nameServers);
                        }
                        else
                        {
                            resolver=std::make_shared<HATN_NETWORK_NAMESPACE::asio::CaResolver>();
                        }
                        resolver->resolveName("r-test2.decfile.com",foundCb1,context,80);
                        resolver->resolveService("_test._udp.decfile.com",foundCb2,context);
                        resolver->resolveMx("decfile.com",foundCb3,context);
#ifdef DNS_RESOLVE_LOCAL_HOSTS
                        resolver->resolveName("test1.local",foundCb4,context,22);
#endif
                    }
                );

    env->exec(20);

    HATN_TEST_MESSAGE_TS("checkResolve: after exec1");

    HATN_CHECK_EXEC_SYNC(
    thread0->execSync(
                    [&resolver]()
                    {
                        HATN_TEST_MESSAGE_TS("checkResolve: reset resolver");

                        resolver.reset();
                    }));

    HATN_TEST_MESSAGE_TS("checkResolve: after exec2");

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

BOOST_AUTO_TEST_CASE(Shuffle)
{
    HATN_NETWORK_NAMESPACE::ResolverShuffle shuffle;

    std::vector<HATN_NETWORK_NAMESPACE::asio::IpEndpoint> eps0{
        HATN_NETWORK_NAMESPACE::asio::IpEndpoint("8.8.8.8",80),
        HATN_NETWORK_NAMESPACE::asio::IpEndpoint("1.1.1.1",80),
        HATN_NETWORK_NAMESPACE::asio::IpEndpoint("2001:4860:4860::8888",80),
        HATN_NETWORK_NAMESPACE::asio::IpEndpoint("2001:4860:4860::8844",80)
    };
    std::vector<uint16_t> ports{8080,9090};
    shuffle.loadFallbackPorts(ports);

    auto log=[](const auto& eps)
    {
        for (auto&& ep:eps)
        {
            HATN_TEST_MESSAGE_TS(fmt::format("Endpoint {}:{}",ep.address().to_string(),ep.port()));
        }
    };

    HATN_TEST_MESSAGE_TS("Before shuffle");
    log(eps0);

    auto eps1=eps0;
    shuffle.shuffle(eps1);
    HATN_TEST_MESSAGE_TS("After shuffle mode NONE");
    log(eps1);
    BOOST_CHECK_EQUAL(eps1.size(),eps0.size());

    auto eps2=eps0;
    shuffle.setMode(HATN_NETWORK_NAMESPACE::ResolverShuffle::Mode::RANDOM);
    shuffle.shuffle(eps2);
    HATN_TEST_MESSAGE_TS("After shuffle mode RANDOM");
    log(eps2);
    BOOST_CHECK_EQUAL(eps2.size(),eps0.size());

    auto eps3=eps0;
    shuffle.setMode(HATN_NETWORK_NAMESPACE::ResolverShuffle::Mode::APPEND_FALLBACK_PORTS);
    shuffle.shuffle(eps3);
    HATN_TEST_MESSAGE_TS("After shuffle mode APPEND_FALLBACK_PORTS");
    log(eps3);
    BOOST_CHECK_EQUAL(eps3.size(),eps0.size()*3);

    auto eps4=eps0;
    shuffle.setMode(HATN_NETWORK_NAMESPACE::ResolverShuffle::Mode::RANDOM_APPEND_FALLBACK_PORTS);
    shuffle.shuffle(eps4);
    HATN_TEST_MESSAGE_TS("After shuffle mode RANDOM+APPEND_FALLBACK_PORTS");
    log(eps4);
    BOOST_CHECK_EQUAL(eps4.size(),eps0.size()*3);
}

#endif
BOOST_AUTO_TEST_SUITE_END()
