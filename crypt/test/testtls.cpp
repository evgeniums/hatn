/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string.hpp>

#include <hatn_test_config.h>

#include <hatn/common/bytearray.h>
#include <hatn/common/format.h>
#include <hatn/common/fileutils.h>
#include <hatn/common/utils.h>

#include <hatn/crypt/cryptplugin.h>
#include <hatn/crypt/x509certificate.h>
#include <hatn/crypt/x509certificatestore.h>
#include <hatn/crypt/x509certificatechain.h>
#include <hatn/crypt/securestreamtypes.h>
#include <hatn/crypt/securestream.h>
#include <hatn/crypt/securestreamcontext.h>
#include <hatn/crypt/dh.h>

#include <hatn/crypt/ciphersuite.h>

#include <hatn/test/multithreadfixture.h>

#include "initcryptplugin.h"

#define HATN_TEST_LOG_CONSOLE

namespace hatn {

using namespace common;
using namespace crypt;

namespace test {

namespace {

struct Env : public ::hatn::test::CryptTestFixture
{
    Env()
    {
    }

    ~Env()
    {
#ifdef HATN_TEST_LOG_CONSOLE
        Logger::resetModules();
#endif
    }

    Env(const Env&)=delete;
    Env(Env&&) =delete;
    Env& operator=(const Env&)=delete;
    Env& operator=(Env&&) =delete;

    std::pair<common::SharedPtr<common::TaskContext>,common::SharedPtr<common::TaskContext>> tasks;
};
}

BOOST_AUTO_TEST_SUITE(TestTls)

namespace
{

struct TlsConfig
{
    SecureStreamTypes::ProtocolVersion serverProtocol=SecureStreamTypes::ProtocolVersion::TLS1_3;
    SecureStreamTypes::ProtocolVersion clientProtocol=SecureStreamTypes::ProtocolVersion::TLS1_3;

    SecureStreamTypes::Verification clientVerification=SecureStreamTypes::Verification::Peer;
    SecureStreamTypes::Verification serverVerification=SecureStreamTypes::Verification::None;

    SharedPtr<X509Certificate> serverCrt;
    SharedPtr<X509CertificateStore> clientStore;
    SharedPtr<X509CertificateStore> serverStore;

    SharedPtr<X509CertificateChain> serverChain;
    SharedPtr<X509CertificateChain> clientChain;

    SharedPtr<DH> clientDH;
    SharedPtr<DH> serverDH;

    std::vector<std::string> clientEC;
    std::vector<std::string> serverEC;
    std::vector<const CryptAlgorithm*> clientECAlgs;
    std::vector<const CryptAlgorithm*> serverECAlgs;

    std::vector<std::string> clientCipherSuites;
    std::vector<std::string> serverCipherSuites;

    SecureStreamErrors clientIgnoreErrors;
    std::function<void (const Error&)> clientVerifyCb;
    std::function<void (const Error&)> serverVerifyCb;
    SharedPtr<X509Certificate> clientCrt;

    bool clientAllErrorsIgnored=false;
    bool clientAllErrorsCollect=true;

    int clientVerifyDepth=-1;
    SharedPtr<PrivateKey> serverPkey;
    SharedPtr<PrivateKey> clientPkey;

    ByteArray serverName;
    ByteArray secondServerName;

    StreamBridge bridge;

    bool ctxExpectedFail=false;

    Thread* thread=nullptr;

    bool autoDH=false;
};

struct TestContextStorage
{
    std::vector<std::shared_ptr<TlsConfig>> cfgs;
    std::pair<SharedPtr<SecureStreamContext>,SharedPtr<SecureStreamContext>> contexts;
    std::pair<SharedPtr<SecureStreamV>,SharedPtr<SecureStreamV>> streams;
    //! @todo Figure out what's wrong with std::vector<SharedPtr<SecureStreamContext>> and with std::vector<SharedPtr<SecureStreamV>>
    //! when building with mingw - come compilation warnings regarding destroyng vector elements.

    void reset()
    {
        cfgs.clear();
        contexts=std::pair<SharedPtr<SecureStreamContext>,SharedPtr<SecureStreamContext>>{};
        streams=std::pair<SharedPtr<SecureStreamV>,SharedPtr<SecureStreamV>>{};
    }
};
static TestContextStorage testContextStorage;

void checkAlg(
        const std::function<void (std::shared_ptr<CryptPlugin>&,
                            const std::string&,
                            const std::string&)>& handler
    )
{
#ifdef HATN_TEST_LOG_CONSOLE
#if 0
        std::vector<std::string> debugModule={"opensslstream;debug;0","global;debug;0"};
#else
        std::vector<std::string> debugModule={"global;debug;0"};
#endif
        Logger::configureModules(debugModule);
#endif

    CryptPluginTest::instance().eachPlugin<CryptTestTraits>(
        [handler](std::shared_ptr<CryptPlugin>& plugin)
        {
            if (plugin->isFeatureImplemented(Crypt::Feature::X509)
                    &&
                plugin->isFeatureImplemented(Crypt::Feature::Signature)
                    &&
                plugin->isFeatureImplemented(Crypt::Feature::TLS)
               )
            {
                auto eachPath=[&plugin,handler](const std::string& path)
                {
                    auto eachLine=[&plugin,handler,&path](const std::string& algName)
                    {
                        auto algPathName=algName;
                        boost::algorithm::replace_all(algPathName,std::string("/"),std::string("-"));
                        std::string prefix=fmt::format("{}/tls/tls-{}",path,algPathName);

                        const CryptAlgorithm* alg=nullptr;
                        auto ec=plugin->findAlgorithm(alg,CryptAlgorithm::Type::SIGNATURE,algName);
                        if (!ec && alg)
                        {
                            handler(plugin,algName,prefix);
                            testContextStorage.reset();
                        }
                    };
                    std::string fileName=fmt::format("{}/tls-algs.txt",path);
                    if (boost::filesystem::exists(fileName))
                    {
                        PluginList::eachLinefromFile(fileName,eachLine);
                    }
                };
                eachPath(PluginList::assetsPath("crypt"));
                eachPath(PluginList::assetsPath("crypt",plugin->info()->name));
            }
        }
    );
}

void tlsSingleStreamNoConfig(
        SharedPtr<SecureStreamContext> serverContext
    )
{
    auto tmpStream=serverContext->createSecureStream(Thread::mainThread().get());

    tmpStream->setWriteNext(
        [&](const char*,std::size_t size,const StreamChain::ResultCb&)
        {
            HATN_DEBUG_LVL(global,5,fmt::format("Write handler: size={}",size));
        }
    );
    tmpStream->setReadNext(
        [&](char*,std::size_t maxSize,const StreamChain::ResultCb&)
        {
            HATN_DEBUG_LVL(global,5,fmt::format("Read handler: size={}",maxSize));
        }
    );

    auto&& cb=[&](const Error& ec)
    {
        auto msg=ec.message();
        HATN_DEBUG_LVL(global,5,fmt::format("Callback: {}",msg));
    };
    tmpStream->prepare(cb);

    tmpStream.reset();
}

auto tlsStreamsInit(
        TlsConfig& config,
        SharedPtr<SecureStreamContext> serverContext,
        SharedPtr<SecureStreamContext> clientContext,
        const std::function<void (
        SharedPtr<SecureStreamV> serverStream,
        SharedPtr<SecureStreamV> clientStream
                )>& cb
    )
{
    Error ec;

    auto serverTaskCtx=common::makeShared<common::TaskContext>("server");
    auto clientTaskCtx=common::makeShared<common::TaskContext>("client");

    auto serverStream=serverContext->createSecureStream(config.thread);
    BOOST_REQUIRE(serverStream);
    serverStream->setMainCtx(serverTaskCtx.get());

    auto clientStream=clientContext->createSecureStream(config.thread);
    BOOST_REQUIRE(clientStream);
    clientStream->setMainCtx(clientTaskCtx.get());

    cb(serverStream,clientStream);

    if (!config.serverName.isEmpty())
    {
        if (!config.secondServerName.isEmpty())
        {
            ec=clientStream->addPeerVerifyName(config.serverName);
            BOOST_CHECK(!ec);
            ec=clientStream->addPeerVerifyName(config.secondServerName);
            BOOST_CHECK(!ec);
        }
        else
        {
            ec=clientStream->setPeerVerifyName(config.serverName);
            BOOST_CHECK(!ec);
        }
    }

    HATN_DEBUG_LVL(global,5,"Server left, client right");

    serverStream->setWriteNext(
        [&](const char* data,std::size_t size,StreamChain::ResultCb cb)
        {
            config.bridge.writeLeftToRight(data,size,std::move(cb));
        }
    );
    serverStream->setReadNext(
        [&](char* data,std::size_t maxSize,StreamChain::ResultCb cb)
        {
            config.bridge.readRightToLeft(data,maxSize,std::move(cb));
        }
    );
    clientStream->setWriteNext(
        [&](const char* data,std::size_t size,StreamChain::ResultCb cb)
        {
            config.bridge.writeRightToLeft(data,size,std::move(cb));
        }
    );
    clientStream->setReadNext(
        [&](char* data,std::size_t maxSize,StreamChain::ResultCb cb)
        {
            config.bridge.readLeftToRight(data,maxSize,std::move(cb));
        }
    );

    auto&& serverHandshakeCb=[&](const Error& ec)
    {
        config.serverVerifyCb(ec);
    };
    serverStream->prepare(serverHandshakeCb);

    auto&& clientHandshakeCb=[&](const Error& ec)
    {
        config.clientVerifyCb(ec);
    };
    clientStream->prepare(clientHandshakeCb);

    return std::make_pair(serverTaskCtx,clientTaskCtx);
}

void tlsContext(
        TlsConfig& config,
        std::shared_ptr<CryptPlugin>& plugin,
        SharedPtr<SecureStreamContext>& serverContext,
        SharedPtr<SecureStreamContext>& clientContext
    )
{
    config.bridge.setPromoteAsync(true);

    Error ec;

    serverContext=plugin->createSecureStreamContext(SecureStreamTypes::Endpoint::Server,
                                                         config.serverVerification);
    HATN_REQUIRE(serverContext);
    BOOST_CHECK(serverContext->endpointType()==SecureStreamTypes::Endpoint::Server);
    BOOST_CHECK(serverContext->verifyMode()==config.serverVerification);
    BOOST_CHECK(serverContext->verifyMode()==config.serverVerification);
    BOOST_CHECK(serverContext->minProtocolVersion()==SecureStreamTypes::ProtocolVersion::TLS1_3);
    serverContext->setMinProtocolVersion(config.serverProtocol);
    BOOST_CHECK(serverContext->minProtocolVersion()==config.serverProtocol);
    if (config.serverPkey)
    {
        ec=serverContext->setPrivateKey(config.serverPkey);
        if (ec)
        {
            G_DEBUG(fmt::format("Failed to set server private key: {}",ec.message()));
        }
        BOOST_CHECK(!ec);
    }
    if (config.serverChain)
    {
        ec=serverContext->setX509CertificateChain(config.serverChain);
        if (ec)
        {
            G_DEBUG(fmt::format("Failed to set server certificate chain: {}",ec.message()));
        }
        BOOST_CHECK(!ec);
    }
    if (config.serverCrt)
    {
        ec=serverContext->setX509Certificate(config.serverCrt);
        if (ec)
        {
            G_DEBUG(fmt::format("Failed to set server certificate: {}",ec.message()));
        }
        if (config.ctxExpectedFail)
        {
            BOOST_CHECK(ec);
        }
        else
        {
            BOOST_CHECK(!ec);
        }
    }
    if (config.serverStore)
    {
        ec=serverContext->setX509CertificateStore(config.serverStore);
        if (ec)
        {
            G_DEBUG(fmt::format("Failed to set server certificate store: {}",ec.message()));
        }
        BOOST_CHECK(!ec);
    }
    if (config.serverDH)
    {
        ec=serverContext->setDH(config.serverDH);
        if (ec)
        {
            G_DEBUG(fmt::format("Failed to set server DH: {}",ec.message()));
        }
        BOOST_CHECK(!ec);
    }
    if (config.autoDH)
    {
        ec=serverContext->setDH(true);
        if (ec)
        {
            G_DEBUG(fmt::format("Failed to set server auto DH: {}",ec.message()));
        }
        BOOST_CHECK(!ec);
    }
    if (!config.serverEC.empty())
    {
        ec=serverContext->setECDHAlgs(config.serverEC);
        if (ec)
        {
            G_DEBUG(fmt::format("Failed to set server ECDH algorithm names: {}",ec.message()));
        }
        BOOST_CHECK(!ec);
    }
    if (!config.serverECAlgs.empty())
    {
        ec=serverContext->setECDHAlgs(config.serverECAlgs);
        if (ec)
        {
            G_DEBUG(fmt::format("Failed to set server ECDH algorithms: {}",ec.message()));
        }
        BOOST_CHECK(!ec);
    }
    if (!config.serverCipherSuites.empty())
    {
        ec=serverContext->setCipherSuites(config.serverCipherSuites);
        if (ec)
        {
            G_DEBUG(fmt::format("Failed to set server cipher suites: {}",ec.message()));
        }
        BOOST_CHECK(!ec);
    }

    clientContext=plugin->createSecureStreamContext(SecureStreamTypes::Endpoint::Client,
                                                         config.clientVerification);
    HATN_REQUIRE(clientContext);
    BOOST_CHECK(clientContext->endpointType()==SecureStreamTypes::Endpoint::Client);
    BOOST_CHECK(clientContext->verifyMode()==config.clientVerification);

    clientContext->setIgnoredErrors(config.clientIgnoreErrors);
    BOOST_CHECK(config.clientIgnoreErrors==clientContext->ignoredErrors());

    clientContext->setAllErrorsIgnored(config.clientAllErrorsIgnored);
    BOOST_CHECK_EQUAL(clientContext->checkAllErrorsIgnored(),config.clientAllErrorsIgnored);

    clientContext->setCollectAllErrors(config.clientAllErrorsCollect);
    BOOST_CHECK_EQUAL(clientContext->checkCollectAllErrors(),config.clientAllErrorsCollect);

    BOOST_CHECK(clientContext->minProtocolVersion()==SecureStreamTypes::ProtocolVersion::TLS1_3);
    clientContext->setMinProtocolVersion(config.clientProtocol);
    BOOST_CHECK(clientContext->minProtocolVersion()==config.clientProtocol);
    clientContext->setMaxProtocolVersion(config.clientProtocol);
    BOOST_CHECK(clientContext->maxProtocolVersion()==config.clientProtocol);

    if (config.clientVerifyDepth>=0)
    {
        ec=clientContext->setVerifyDepth(config.clientVerifyDepth);
        if (ec)
        {
            G_DEBUG(fmt::format("Failed to set client verify depth: {}",ec.message()));
        }
        BOOST_CHECK(!ec);
    }
    if (config.clientStore)
    {
        ec=clientContext->setX509CertificateStore(config.clientStore);
        if (ec)
        {
            G_DEBUG(fmt::format("Failed to set client certificate store: {}",ec.message()));
        }
        BOOST_CHECK(!ec);
    }
    if (config.clientChain)
    {
        ec=clientContext->setX509CertificateChain(config.clientChain);
        if (ec)
        {
            G_DEBUG(fmt::format("Failed to set client certificate chain: {}",ec.message()));
        }
        BOOST_CHECK(!ec);
    }
    if (config.clientPkey)
    {
        ec=clientContext->setPrivateKey(config.clientPkey);
        if (ec)
        {
            G_DEBUG(fmt::format("Failed to set client private key: {}",ec.message()));
        }
        BOOST_CHECK(!ec);
    }
    if (config.clientCrt)
    {
        ec=clientContext->setX509Certificate(config.clientCrt);
        if (ec)
        {
            G_DEBUG(fmt::format("Failed to set client certificate: {}",ec.message()));
        }
        BOOST_CHECK(!ec);
    }
    if (config.clientDH)
    {
        ec=clientContext->setDH(config.clientDH);
        if (ec)
        {
            G_DEBUG(fmt::format("Failed to set client DH: {}",ec.message()));
        }
        BOOST_CHECK(!ec);
    }
    if (config.autoDH)
    {
        ec=clientContext->setDH(true);
        if (ec)
        {
            G_DEBUG(fmt::format("Failed to set client auto DH: {}",ec.message()));
        }
        BOOST_CHECK(!ec);
    }
    if (!config.clientEC.empty())
    {
        ec=clientContext->setECDHAlgs(config.clientEC);
        if (ec)
        {
            G_DEBUG(fmt::format("Failed to set client ECDH algorithm names: {}",ec.message()));
        }
        BOOST_CHECK(!ec);
    }
    if (!config.clientECAlgs.empty())
    {
        ec=clientContext->setECDHAlgs(config.clientECAlgs);
        if (ec)
        {
            G_DEBUG(fmt::format("Failed to set client ECDH algorithms: {}",ec.message()));
        }
        BOOST_CHECK(!ec);
    }
    if (!config.clientCipherSuites.empty())
    {
        ec=clientContext->setCipherSuites(config.clientCipherSuites);
        if (ec)
        {
            G_DEBUG(fmt::format("Failed to set client cipher suites: {}",ec.message()));
        }
        BOOST_CHECK(!ec);
    }
}

void checkHandshake(
        Env* env,
        std::shared_ptr<CryptPlugin>& plugin,
        const std::string& algName,
        const std::string& pathPrefix,
        const std::function<void (TlsConfig&)>& setupCfg=std::function<void (TlsConfig&)>(),
        bool clientFail=false,
        const std::function<void (const Error&,SharedPtr<SecureStreamV> clientStream)>& clientCb=std::function<void (const Error&,SharedPtr<SecureStreamV> clientStream)>(),
        bool serverFail=false,
        const std::function<void (const Error&,SharedPtr<SecureStreamV> clientStream)>& serverCb=std::function<void (const Error&,SharedPtr<SecureStreamV> clientStream)>(),
        int duration=1
    )
{
    std::ignore=algName;
    Error ec;

    auto ca1Pem=plugin->createX509Certificate();
    HATN_REQUIRE(ca1Pem);
    std::string ca1PemFile=fmt::format("{}-ca1.pem",pathPrefix);
    ec=ca1Pem->loadFromFile(ca1PemFile,ContainerFormat::PEM);
    BOOST_CHECK(!ec);

    auto ca2Pem=plugin->createX509Certificate();
    HATN_REQUIRE(ca2Pem);
    std::string ca2PemFile=fmt::format("{}-ca2.pem",pathPrefix);
    ec=ca2Pem->loadFromFile(ca2PemFile,ContainerFormat::PEM);
    BOOST_CHECK(!ec);

    common::SharedPtr<PrivateKey> serverPkey;
    std::string serverPkeyFile=fmt::format("{}-host1_2_3_pkey.pem",pathPrefix);
    ByteArray serverPkeyBuf;
    ec=serverPkeyBuf.loadFromFile(serverPkeyFile);
    HATN_REQUIRE(!ec);
    ec=plugin->createAsymmetricPrivateKeyFromContent(serverPkey,serverPkeyBuf,ContainerFormat::PEM);
    HATN_REQUIRE(!ec);
    HATN_REQUIRE(serverPkey);

    auto serverCrt=plugin->createX509Certificate();
    HATN_REQUIRE(serverCrt);
    std::string serverCrtFile=fmt::format("{}-host1_2_3.pem",pathPrefix);
    ec=serverCrt->loadFromFile(serverCrtFile,ContainerFormat::PEM);
    BOOST_CHECK(!ec);

    auto store=plugin->createX509CertificateStore();
    HATN_REQUIRE(store);
    ec=store->addCertificate(*ca1Pem);
    ec=store->addCertificate(*ca2Pem);
    BOOST_CHECK(!ec);

    auto chain=plugin->createX509CertificateChain();
    HATN_REQUIRE(!ec);
    HATN_REQUIRE(chain);
    std::string chainPemFile=fmt::format("{}-chain.pem",pathPrefix);
    ec=chain->loadFromFile(chainPemFile);
    HATN_REQUIRE(!ec);

    auto config=std::make_shared<TlsConfig>();
    config->thread=Thread::mainThread().get();
    config->serverCrt=serverCrt;
    config->serverPkey=serverPkey;
    config->serverChain=chain;
    config->clientStore=store;
    std::string servernameFile=fmt::format("{}-servername.txt",pathPrefix);
    ec=config->serverName.loadFromFile(servernameFile);
    BOOST_CHECK(!ec);

    if (setupCfg)
    {
        setupCfg(*config);
    }

    testContextStorage.cfgs.push_back(config);    

    config->thread->execAsync(
        [env,plugin,config,serverFail,serverCb,clientFail,clientCb]()
        {
            SharedPtr<SecureStreamContext> serverContext;
            SharedPtr<SecureStreamContext> clientContext;

            auto pl=plugin;
            const auto& cfg=config;
            tlsContext(*cfg,pl,serverContext,clientContext);
            if (cfg->ctxExpectedFail)
            {
                env->quit();
                return;
            }
            HATN_REQUIRE(serverContext&&clientContext);

            testContextStorage.contexts=std::make_pair(serverContext,clientContext);

            auto streamInitCb=[cfg,serverFail,serverCb,clientFail,clientCb](
                    const SharedPtr<SecureStreamV>& serverStream,
                    const SharedPtr<SecureStreamV>& clientStream
            )
            {
                testContextStorage.streams=std::make_pair(serverStream,clientStream);

                cfg->serverVerifyCb=[serverFail,serverCb,serverStream](const Error& ec)
                {
                    G_DEBUG(fmt::format("Server verify: {}",ec.message()));
                    if (serverFail)
                    {
                        BOOST_CHECK(ec);
                    }
                    else
                    {
                        BOOST_CHECK(!ec);
                    }
                    if (serverCb)
                    {
                        serverCb(ec,serverStream);
                    }
                };
                cfg->clientVerifyCb=[clientFail,clientCb,clientStream](const Error& ec)
                {
                    G_DEBUG(fmt::format("Client verify: {}",ec.message()));
                    if (clientFail)
                    {
                        BOOST_CHECK(ec);
                    }
                    else
                    {
                        BOOST_CHECK(!ec);
                    }
                    if (clientCb)
                    {
                        clientCb(ec,clientStream);
                    }
                };
            };

            env->tasks=tlsStreamsInit(*cfg,serverContext,clientContext,streamInitCb);
        }
    );
    if (env->exec(duration))
    {
        testContextStorage.reset();
    }
}

} // anonymous namespace

BOOST_FIXTURE_TEST_CASE(CheckTlsSingleStream,Env)
{
    auto algHandler=[](std::shared_ptr<CryptPlugin>& plugin,const std::string& algName,const std::string& pathPrefix)
    {
        std::ignore=algName;
        std::ignore=pathPrefix;

        TlsConfig config;
        config.thread=Thread::mainThread().get();
        SharedPtr<SecureStreamContext> serverContext;
        SharedPtr<SecureStreamContext> clientContext;

        tlsContext(config,plugin,serverContext,clientContext);
        HATN_REQUIRE(serverContext&&clientContext);
        tlsSingleStreamNoConfig(serverContext);
    };
    checkAlg(algHandler);
}

BOOST_FIXTURE_TEST_CASE(CheckTlsEmptyContext,Env)
{
    auto algHandler=[this](std::shared_ptr<CryptPlugin>& plugin,const std::string& algName,const std::string& pathPrefix)
    {
        std::ignore=algName;
        std::ignore=pathPrefix;

        TlsConfig config;
        config.thread=Thread::mainThread().get();
        config.serverVerifyCb=[](const Error& ec)
        {
            G_DEBUG(fmt::format("Server verify: {}",ec.message()));
            BOOST_CHECK(ec);
        };
        config.clientVerifyCb=[](const Error& ec)
        {
            G_DEBUG(fmt::format("Client verify: {}",ec.message()));
            BOOST_CHECK(ec);
        };

        config.thread->execAsync(
            [&plugin,&config,this]()
            {
                SharedPtr<SecureStreamContext> serverContext;
                SharedPtr<SecureStreamContext> clientContext;

                tlsContext(config,plugin,serverContext,clientContext);

                HATN_REQUIRE(serverContext&&clientContext);

                testContextStorage.contexts=std::make_pair(serverContext,clientContext);

                auto streamInitCb=[](
                        const SharedPtr<SecureStreamV>& serverStream,
                        const SharedPtr<SecureStreamV>& clientStream
                )
                {
                    testContextStorage.streams=std::make_pair(serverStream,clientStream);
                };
                this->tasks=tlsStreamsInit(config,serverContext,clientContext,streamInitCb);
            }
        );
        exec(1);
        testContextStorage.reset();
    };
    checkAlg(algHandler);
}

BOOST_FIXTURE_TEST_CASE(CheckTlsNormalHandshakeSelfSigned,Env)
{
    auto algHandler=[this](std::shared_ptr<CryptPlugin>& plugin,const std::string& algName,const std::string& pathPrefix)
    {
        std::ignore=algName;
        Error ec;

        common::SharedPtr<PrivateKey> pkey1;
        std::string pkey1File=fmt::format("{}-ca1_pkey.pem",pathPrefix);
        ByteArray pkey1Buf;
        ec=pkey1Buf.loadFromFile(pkey1File);
        HATN_REQUIRE(!ec);
        ec=plugin->createAsymmetricPrivateKeyFromContent(pkey1,pkey1Buf,ContainerFormat::PEM);
        HATN_REQUIRE(!ec);
        HATN_REQUIRE(pkey1);

        auto ca1Pem=plugin->createX509Certificate();
        HATN_REQUIRE(ca1Pem);
        std::string ca1PemFile=fmt::format("{}-ca1.pem",pathPrefix);
        ec=ca1Pem->loadFromFile(ca1PemFile,ContainerFormat::PEM);
        BOOST_CHECK(!ec);

        auto store=plugin->createX509CertificateStore();
        HATN_REQUIRE(store);
        ec=store->addCertificate(*ca1Pem);
        BOOST_CHECK(!ec);

        auto config=std::make_shared<TlsConfig>();
        config->thread=Thread::mainThread().get();
        config->serverCrt=ca1Pem;
        config->serverPkey=pkey1;
        config->clientStore=store;

        std::string servernameFile=fmt::format("{}-servername-selfsigned.txt",pathPrefix);
        ec=config->serverName.loadFromFile(servernameFile);
        BOOST_CHECK(!ec);

        SharedPtr<SecureStreamContext> serverContext;
        SharedPtr<SecureStreamContext> clientContext;
        testContextStorage.contexts=std::make_pair(serverContext,clientContext);

        config->serverVerifyCb=[](const Error& ec)
        {
            G_DEBUG(fmt::format("Server verify: {}",ec.message()));
            BOOST_CHECK(!ec);
        };
        config->clientVerifyCb=[](const Error& ec)
        {
            G_DEBUG(fmt::format("Client verify: {}",ec.message()));
            BOOST_CHECK(!ec);
        };

        config->thread->execAsync(
            [&plugin,&config,&serverContext,&clientContext,this]()
            {
                tlsContext(*config,plugin,serverContext,clientContext);
                HATN_REQUIRE(serverContext&&clientContext);
                auto streamInitCb=[](
                        const SharedPtr<SecureStreamV>& serverStream,
                        const SharedPtr<SecureStreamV>& clientStream
                )
                {
                    testContextStorage.streams=std::make_pair(serverStream,clientStream);
                };
                this->tasks=tlsStreamsInit(*config,serverContext,clientContext,streamInitCb);
            }
        );
        exec(1);
        testContextStorage.reset();
    };
    checkAlg(algHandler);
}

BOOST_FIXTURE_TEST_CASE(CheckTlsHandshakeChainOk,Env)
{
    auto algHandler=[this](std::shared_ptr<CryptPlugin>& plugin,const std::string& algName,const std::string& pathPrefix)
    {
        ByteArray serverName;
        SharedPtr<X509Certificate> serverCrt;

        auto setupCfg=[&serverName,&serverCrt](TlsConfig& cfg)
        {
            serverName=cfg.serverName;
            serverCrt=cfg.serverCrt;
        };
        auto clientCb=[&serverName,&serverCrt](const Error& ec,SharedPtr<SecureStreamV> clientStream)
        {
            HATN_REQUIRE(clientStream);
            BOOST_CHECK(!ec);

            ByteArray peername(clientStream->getVerifiedPeerName());
            BOOST_CHECK(peername==serverName);

            auto peerCrt=clientStream->getPeerCertificate();
            HATN_REQUIRE(peerCrt);
            BOOST_CHECK(*peerCrt==*serverCrt);
        };
        checkHandshake(this,plugin,algName,pathPrefix,setupCfg,false,clientCb);
    };
    checkAlg(algHandler);
}

BOOST_FIXTURE_TEST_CASE(CheckTlsHandshakeNoCaClientFail,Env)
{
    auto algHandler=[this](std::shared_ptr<CryptPlugin>& plugin,const std::string& algName,const std::string& pathPrefix)
    {
        auto setupCfg=[](TlsConfig& config)
        {
            config.clientStore.reset();
        };
        checkHandshake(this,plugin,algName,pathPrefix,setupCfg,true);
    };
    checkAlg(algHandler);
}

BOOST_FIXTURE_TEST_CASE(CheckTlsHandshakeNoChainClientFail,Env)
{
    auto algHandler=[this](std::shared_ptr<CryptPlugin>& plugin,const std::string& algName,const std::string& pathPrefix)
    {
        auto setupCfg=[](TlsConfig& config)
        {
            config.serverChain.reset();
        };
        checkHandshake(this,plugin,algName,pathPrefix,setupCfg,true);
    };
    checkAlg(algHandler);
}

BOOST_FIXTURE_TEST_CASE(CheckTlsHandshakeInvalidCaClientFail,Env)
{
    auto algHandler=[this](std::shared_ptr<CryptPlugin>& plugin,const std::string& algName,const std::string& pathPrefix)
    {
        auto setupCfg=[plugin,pathPrefix](TlsConfig& config)
        {
            auto ca2Pem=plugin->createX509Certificate();
            HATN_REQUIRE(ca2Pem);
            std::string ca2PemFile=fmt::format("{}-ca2.pem",pathPrefix);
            auto ec=ca2Pem->loadFromFile(ca2PemFile,ContainerFormat::PEM);
            BOOST_CHECK(!ec);

            auto store=plugin->createX509CertificateStore();
            HATN_REQUIRE(store);
            ec=store->addCertificate(*ca2Pem);
            BOOST_CHECK(!ec);

            config.clientStore=store;
        };
        checkHandshake(this,plugin,algName,pathPrefix,setupCfg,true);
    };
    checkAlg(algHandler);
}

BOOST_FIXTURE_TEST_CASE(CheckTlsHandshakeInvalidCaClientIgnoreOk,Env)
{
    auto algHandler=[this](std::shared_ptr<CryptPlugin>& plugin,const std::string& algName,const std::string& pathPrefix)
    {
        auto setupCfg=[plugin,pathPrefix](TlsConfig& config)
        {
            auto ca2Pem=plugin->createX509Certificate();
            HATN_REQUIRE(ca2Pem);
            std::string ca2PemFile=fmt::format("{}-ca2.pem",pathPrefix);
            auto ec=ca2Pem->loadFromFile(ca2PemFile,ContainerFormat::PEM);
            BOOST_CHECK(!ec);

            auto store=plugin->createX509CertificateStore();
            HATN_REQUIRE(store);
            ec=store->addCertificate(*ca2Pem);
            BOOST_CHECK(!ec);

            config.clientStore=store;
            config.clientAllErrorsIgnored=true;
        };
        checkHandshake(this,plugin,algName,pathPrefix,setupCfg);
    };
    checkAlg(algHandler);
}

BOOST_FIXTURE_TEST_CASE(CheckTlsHandshakeInvalidChainClientFail,Env)
{
    auto algHandler=[this](std::shared_ptr<CryptPlugin>& plugin,const std::string& algName,const std::string& pathPrefix)
    {
        auto setupCfg=[plugin,pathPrefix](TlsConfig& config)
        {
            auto chain=plugin->createX509CertificateChain();
            HATN_REQUIRE(chain);
            std::string chainPemFile=fmt::format("{}-im1_2.pem",pathPrefix);
            auto ec=chain->loadFromFile(chainPemFile);
            HATN_REQUIRE(!ec);

            config.serverChain=chain;
        };
        checkHandshake(this,plugin,algName,pathPrefix,setupCfg,true);
    };
    checkAlg(algHandler);
}

BOOST_FIXTURE_TEST_CASE(CheckTlsHandshakeAltNameOk,Env)
{
    auto algHandler=[this](std::shared_ptr<CryptPlugin>& plugin,const std::string& algName,const std::string& pathPrefix)
    {
        auto setupCfg=[pathPrefix](TlsConfig& config)
        {
            std::string servernameFile=fmt::format("{}-servername-alt.txt",pathPrefix);
            auto ec=config.serverName.loadFromFile(servernameFile);
            BOOST_CHECK(!ec);
        };
        checkHandshake(this,plugin,algName,pathPrefix,setupCfg);
    };
    checkAlg(algHandler);
}

BOOST_FIXTURE_TEST_CASE(CheckTlsHandshakeWrongNameClientFail,Env)
{
    auto algHandler=[this](std::shared_ptr<CryptPlugin>& plugin,const std::string& algName,const std::string& pathPrefix)
    {
        auto setupCfg=[pathPrefix](TlsConfig& config)
        {
            std::string servernameFile=fmt::format("{}-servername-wrong.txt",pathPrefix);
            auto ec=config.serverName.loadFromFile(servernameFile);
            BOOST_CHECK(!ec);
        };
        checkHandshake(this,plugin,algName,pathPrefix,setupCfg,true);
    };
    checkAlg(algHandler);
}

BOOST_FIXTURE_TEST_CASE(CheckTlsHandshakeWrongNameIgnoreOk,Env)
{
    auto algHandler=[this](std::shared_ptr<CryptPlugin>& plugin,const std::string& algName,const std::string& pathPrefix)
    {
        auto setupCfg=[pathPrefix](TlsConfig& config)
        {
            std::string servernameFile=fmt::format("{}-servername-wrong.txt",pathPrefix);
            auto ec=config.serverName.loadFromFile(servernameFile);
            BOOST_CHECK(!ec);
        };
        auto clientCb=[this,setupCfg,pathPrefix,plugin,algName](const Error& ec,SharedPtr<SecureStreamV> clientStream)
        {
            HATN_REQUIRE(clientStream);
            BOOST_CHECK(ec);
            auto errors=clientStream->errors();
            BOOST_CHECK(!errors.empty());

            auto setupCfgIgnoreEc=[pathPrefix,&errors](TlsConfig& config)
            {
                std::string servernameFile=fmt::format("{}-servername-wrong.txt",pathPrefix);
                auto ec=config.serverName.loadFromFile(servernameFile);
                BOOST_CHECK(!ec);
                config.clientIgnoreErrors=errors;
            };
            std::shared_ptr<CryptPlugin> pl=plugin;
            checkHandshake(this,pl,algName,pathPrefix,setupCfgIgnoreEc);
        };
        checkHandshake(this,plugin,algName,pathPrefix,setupCfg,true,clientCb);
    };
    checkAlg(algHandler);
}

BOOST_FIXTURE_TEST_CASE(CheckTlsHandshakeSaveRestoreIgnoreEcOk,Env)
{
    auto algHandler=[this](std::shared_ptr<CryptPlugin>& plugin,const std::string& algName,const std::string& pathPrefix)
    {
        auto setupCfg=[pathPrefix](TlsConfig& config)
        {
            std::string servernameFile=fmt::format("{}-servername-wrong.txt",pathPrefix);
            auto ec=config.serverName.loadFromFile(servernameFile);
            BOOST_CHECK(!ec);
        };
        auto clientCb=[this,setupCfg,pathPrefix,plugin,algName](const Error& ec,SharedPtr<SecureStreamV> clientStream)
        {
            HATN_REQUIRE(clientStream);
            BOOST_CHECK(ec);
            auto errors=clientStream->errors();
            BOOST_CHECK(!errors.empty());
            SecureStreamErrors newErrors;
            for (auto&& it:errors)
            {
                ByteArray content;
                auto ec1=X509VerifyError::serialize(it,content);
                BOOST_REQUIRE(!ec1);
                auto error=clientStream->context()->createError(content);
                BOOST_CHECK(!error.isNull());
                newErrors.push_back(std::move(error));
            }

            auto setupCfgIgnoreEc=[pathPrefix,&newErrors](TlsConfig& config)
            {
                std::string servernameFile=fmt::format("{}-servername-wrong.txt",pathPrefix);
                auto ec=config.serverName.loadFromFile(servernameFile);
                BOOST_CHECK(!ec);

                config.clientIgnoreErrors=newErrors;
            };
            std::shared_ptr<CryptPlugin> pl=plugin;
            checkHandshake(this,pl,algName,pathPrefix,setupCfgIgnoreEc);
        };
        checkHandshake(this,plugin,algName,pathPrefix,setupCfg,true,clientCb);
    };
    checkAlg(algHandler);
}

BOOST_FIXTURE_TEST_CASE(CheckTlsHandshakeSecondNameOk,Env)
{
    auto algHandler=[this](std::shared_ptr<CryptPlugin>& plugin,const std::string& algName,const std::string& pathPrefix)
    {
        auto setupCfg=[pathPrefix](TlsConfig& config)
        {
            std::string servernameFile=fmt::format("{}-servername-wrong.txt",pathPrefix);
            auto ec=config.serverName.loadFromFile(servernameFile);
            BOOST_CHECK(!ec);
            std::string secondServernameFile=fmt::format("{}-servername-alt.txt",pathPrefix);
            ec=config.secondServerName.loadFromFile(secondServernameFile);
            BOOST_CHECK(!ec);
        };
        checkHandshake(this,plugin,algName,pathPrefix,setupCfg);
    };
    checkAlg(algHandler);
}

BOOST_FIXTURE_TEST_CASE(CheckTlsHandshakeNoNameVerifyOk,Env)
{
    auto algHandler=[this](std::shared_ptr<CryptPlugin>& plugin,const std::string& algName,const std::string& pathPrefix)
    {
        auto setupCfg=[](TlsConfig& config)
        {
            config.serverName.clear();
        };
        checkHandshake(this,plugin,algName,pathPrefix,setupCfg);
    };
    checkAlg(algHandler);
}

BOOST_FIXTURE_TEST_CASE(CheckTlsHandshakeDepthClientFail,Env)
{
    auto algHandler=[this](std::shared_ptr<CryptPlugin>& plugin,const std::string& algName,const std::string& pathPrefix)
    {
        auto setupCfg=[](TlsConfig& config)
        {
            config.clientVerifyDepth=0;
        };
        checkHandshake(this,plugin,algName,pathPrefix,setupCfg,true);
    };
    checkAlg(algHandler);
}

BOOST_FIXTURE_TEST_CASE(CheckTlsHandshakeVersionMismatchFail,Env)
{
    auto algHandler=[this](std::shared_ptr<CryptPlugin>& plugin,const std::string& algName,const std::string& pathPrefix)
    {
        auto setupCfg=[](TlsConfig& config)
        {
            config.serverProtocol=SecureStreamTypes::ProtocolVersion::TLS1_3;
            config.clientProtocol=SecureStreamTypes::ProtocolVersion::TLS1_2;
        };
        checkHandshake(this,plugin,algName,pathPrefix,setupCfg,
                       true,
                       std::function<void (const Error&,SharedPtr<SecureStreamV> clientStream)>(),
                       true
                       );
    };
    checkAlg(algHandler);
}

BOOST_FIXTURE_TEST_CASE(CheckTlsHandshakeInvalidPkeyFail,Env)
{
    auto algHandler=[this](std::shared_ptr<CryptPlugin>& plugin,const std::string& algName,const std::string& pathPrefix)
    {
        auto setupCfg=[pathPrefix,plugin](TlsConfig& config)
        {
            common::SharedPtr<PrivateKey> pkey;
            std::string pkeyFile=fmt::format("{}-ca1_pkey.pem",pathPrefix);
            ByteArray pkeyBuf;
            auto ec=pkeyBuf.loadFromFile(pkeyFile);
            HATN_REQUIRE(!ec);
            ec=plugin->createAsymmetricPrivateKeyFromContent(pkey,pkeyBuf,ContainerFormat::PEM);
            HATN_REQUIRE(!ec);
            HATN_REQUIRE(pkey);
            config.serverPkey=pkey;
            config.ctxExpectedFail=true;
        };
        checkHandshake(this,plugin,algName,pathPrefix,setupCfg,
                       true,
                       std::function<void (const Error&,SharedPtr<SecureStreamV> clientStream)>(),
                       true
                       );
    };
    checkAlg(algHandler);
}

BOOST_FIXTURE_TEST_CASE(CheckTlsHandshakeClientPkeyFail,Env)
{
    auto algHandler=[this](std::shared_ptr<CryptPlugin>& plugin,const std::string& algName,const std::string& pathPrefix)
    {
        ByteArray clientRd;
        clientRd.resize(4096);
        auto setupCfg=[pathPrefix,plugin](TlsConfig& config)
        {
            common::SharedPtr<PrivateKey> pkey;
            std::string pkeyFile=fmt::format("{}-client1_2_1_pkey.pem",pathPrefix);
            ByteArray pkeyBuf;
            auto ec=pkeyBuf.loadFromFile(pkeyFile);
            HATN_REQUIRE(!ec);
            ec=plugin->createAsymmetricPrivateKeyFromContent(pkey,pkeyBuf,ContainerFormat::PEM);
            HATN_REQUIRE(!ec);
            HATN_REQUIRE(pkey);
            config.clientPkey=pkey;

            auto clientCrt=plugin->createX509Certificate();
            HATN_REQUIRE(clientCrt);
            std::string clientCrtFile=fmt::format("{}-client1_2_1.pem",pathPrefix);
            ec=clientCrt->loadFromFile(clientCrtFile,ContainerFormat::PEM);
            BOOST_CHECK(!ec);
            config.clientCrt=clientCrt;

            config.serverVerification=SecureStreamTypes::Verification::Peer;
        };
        auto clientCb=[&clientRd](const Error& ec,SharedPtr<SecureStreamV> clientStream)
        {
            BOOST_CHECK(!ec);
            HATN_REQUIRE(clientStream);
            if (!ec)
            {
                auto readCb=[](const common::Error& ec1,size_t size)
                {
                    if (ec1)
                    {
                        G_DEBUG(fmt::format("Client read failed ({}): {}",ec1.value(),ec1.message()));
                    }
                    else
                    {
                        G_DEBUG(fmt::format("Client read size={}",size));
                    }
                    BOOST_CHECK(ec1);
                };
                clientStream->read(clientRd.data(),clientRd.size(),readCb);
            }
            else
            {
                G_DEBUG(fmt::format("Client handshake failed ({}): {}",ec.value(),ec.message()));
            }
        };
        checkHandshake(this,plugin,algName,pathPrefix,setupCfg,
                       false,
                       clientCb,
                       true
                       );
    };
    checkAlg(algHandler);
}

BOOST_FIXTURE_TEST_CASE(CheckTlsHandshakeClientPkeyOk,Env)
{
    auto algHandler=[this](std::shared_ptr<CryptPlugin>& plugin,const std::string& algName,const std::string& pathPrefix)
    {
        ByteArray clientRd;
        clientRd.resize(4096);
        auto setupCfg=[pathPrefix,plugin](TlsConfig& config)
        {
            auto caPem=plugin->createX509Certificate();
            HATN_REQUIRE(caPem);
            std::string caPemFile=fmt::format("{}-ca1.pem",pathPrefix);
            auto ec=caPem->loadFromFile(caPemFile,ContainerFormat::PEM);
            BOOST_CHECK(!ec);

            auto store=plugin->createX509CertificateStore();
            HATN_REQUIRE(store);
            ec=store->addCertificate(*caPem);
            config.serverStore=store;

            auto chain=plugin->createX509CertificateChain();
            HATN_REQUIRE(!ec);
            HATN_REQUIRE(chain);
            std::string chainPemFile=fmt::format("{}-chain.pem",pathPrefix);
            ec=chain->loadFromFile(chainPemFile);
            HATN_REQUIRE(!ec);
            config.clientChain=chain;

            common::SharedPtr<PrivateKey> pkey;
            std::string pkeyFile=fmt::format("{}-client1_2_1_pkey.pem",pathPrefix);
            ByteArray pkeyBuf;
            ec=pkeyBuf.loadFromFile(pkeyFile);
            HATN_REQUIRE(!ec);
            ec=plugin->createAsymmetricPrivateKeyFromContent(pkey,pkeyBuf,ContainerFormat::PEM);
            HATN_REQUIRE(!ec);
            HATN_REQUIRE(pkey);
            config.clientPkey=pkey;

            auto clientCrt=plugin->createX509Certificate();
            HATN_REQUIRE(clientCrt);
            std::string clientCrtFile=fmt::format("{}-client1_2_1.pem",pathPrefix);
            ec=clientCrt->loadFromFile(clientCrtFile,ContainerFormat::PEM);
            BOOST_CHECK(!ec);
            config.clientCrt=clientCrt;

            config.serverVerification=SecureStreamTypes::Verification::Peer;
        };
        auto clientCb=[&clientRd](const Error& ec,SharedPtr<SecureStreamV> clientStream)
        {
            BOOST_CHECK(!ec);
            HATN_REQUIRE(clientStream);
            if (!ec)
            {
                auto readCb=[](const common::Error& ec1,size_t size)
                {
                    if (ec1)
                    {
                        G_DEBUG(fmt::format("Client read failed ({}): {}",ec1.value(),ec1.message()));
                    }
                    else
                    {
                        G_DEBUG(fmt::format("Client read size={}",size));
                    }
                    BOOST_CHECK(ec1);
                };
                clientStream->read(clientRd.data(),clientRd.size(),readCb);
            }
            else
            {
                G_DEBUG(fmt::format("Client handshake failed ({}): {}",ec.value(),ec.message()));
            }
        };
        checkHandshake(this,plugin,algName,pathPrefix,setupCfg,
                       false,
                       clientCb,
                       false
                       );
    };
    checkAlg(algHandler);
}

BOOST_FIXTURE_TEST_CASE(CheckTlsHandshakeDHOk,Env)
{
    auto algHandler=[this](std::shared_ptr<CryptPlugin>& plugin,const std::string& algName,const std::string& pathPrefix)
    {
        if (plugin->isFeatureImplemented(Crypt::Feature::DH))
        {
            auto setupCfg=[plugin](TlsConfig& config)
            {
                config.serverProtocol=SecureStreamTypes::ProtocolVersion::TLS1_2;
                config.clientProtocol=SecureStreamTypes::ProtocolVersion::TLS1_2;

                const CryptAlgorithm* alg1=nullptr;
                std::string algName1="ffdhe4096";
                auto ec=plugin->findAlgorithm(alg1,CryptAlgorithm::Type::DH,algName1);
                BOOST_REQUIRE(!ec);

                auto clientDH=plugin->createDH(alg1);
                HATN_REQUIRE(clientDH);
                SharedPtr<PublicKey> pubKey1;
                ec=clientDH->generateKey(pubKey1);
                HATN_REQUIRE(!ec);
                config.clientDH=clientDH;

                auto serverDH=plugin->createDH(alg1);
                HATN_REQUIRE(serverDH);
                SharedPtr<PublicKey> pubKey2;
                ec=serverDH->generateKey(pubKey2);
                HATN_REQUIRE(!ec);
                config.serverDH=serverDH;
            };
            checkHandshake(this,plugin,algName,pathPrefix,setupCfg);
        }
    };
    checkAlg(algHandler);
}

BOOST_FIXTURE_TEST_CASE(CheckTlsHandshakeDHAutoOk,Env)
{
    auto algHandler=[this](std::shared_ptr<CryptPlugin>& plugin,const std::string& algName,const std::string& pathPrefix)
    {
        if (plugin->isFeatureImplemented(Crypt::Feature::DH))
        {
            auto setupCfg=[plugin](TlsConfig& config)
            {
                config.serverProtocol=SecureStreamTypes::ProtocolVersion::TLS1_2;
                config.clientProtocol=SecureStreamTypes::ProtocolVersion::TLS1_2;

                config.autoDH=true;
            };
            checkHandshake(this,plugin,algName,pathPrefix,setupCfg);
        }
    };
    checkAlg(algHandler);
}

BOOST_FIXTURE_TEST_CASE(CheckTlsHandshakeECDHOk,Env)
{
    auto algHandler=[this](std::shared_ptr<CryptPlugin>& plugin,const std::string& algName,const std::string& pathPrefix)
    {
        if (plugin->isFeatureImplemented(Crypt::Feature::ECDH))
        {
            std::string algsFile=fmt::format("{}-ecdh-curves1.txt",pathPrefix);
            if (boost::filesystem::exists(algsFile))
            {
                auto setupCfg=[plugin,algsFile](TlsConfig& config)
                {
                    auto algStr=PluginList::linefromFile(algsFile);
                    std::vector<std::string> algs;
                    Utils::trimSplit(algs,algStr,':');
                    if (!algs.empty())
                    {
                        config.clientEC=algs;
                        config.serverEC=algs;
                    }
                };
                checkHandshake(this,plugin,algName,pathPrefix,setupCfg);
            }
        }
    };
    checkAlg(algHandler);
}

BOOST_FIXTURE_TEST_CASE(CheckTlsHandshakeECDHMismatched,Env)
{
    auto algHandler=[this](std::shared_ptr<CryptPlugin>& plugin,const std::string& algName,const std::string& pathPrefix)
    {
        if (plugin->isFeatureImplemented(Crypt::Feature::ECDH))
        {
            std::string algsFile1=fmt::format("{}-ecdh-curves2.txt",pathPrefix);
            std::string algsFile2=fmt::format("{}-ecdh-curves3.txt",pathPrefix);
            if (boost::filesystem::exists(algsFile1) && boost::filesystem::exists(algsFile2))
            {
                auto setupCfg=[plugin,algsFile1,algsFile2](TlsConfig& config)
                {
                    auto algStr1=PluginList::linefromFile(algsFile1);
                    std::vector<std::string> algs1;
                    Utils::trimSplit(algs1,algStr1,':');
                    if (!algs1.empty())
                    {
                        config.clientEC=algs1;
                    }
                    auto algStr2=PluginList::linefromFile(algsFile2);
                    std::vector<std::string> algs2;
                    Utils::trimSplit(algs2,algStr2,':');
                    if (!algs2.empty())
                    {
                        config.serverEC=algs2;
                    }
                };
                checkHandshake(this,plugin,algName,pathPrefix,setupCfg,
                               true,
                               std::function<void (const Error&,SharedPtr<SecureStreamV> clientStream)>(),
                               true
                               );
            }
        }
    };
    checkAlg(algHandler);
}

BOOST_FIXTURE_TEST_CASE(CheckTlsHandshakeECDHAlgsOk,Env)
{
    auto algHandler=[this](std::shared_ptr<CryptPlugin>& plugin,const std::string& algName,const std::string& pathPrefix)
    {
        if (plugin->isFeatureImplemented(Crypt::Feature::ECDH))
        {
            std::string algsFile=fmt::format("{}-ecdh-curves1.txt",pathPrefix);
            if (boost::filesystem::exists(algsFile))
            {
                auto setupCfg=[plugin,algsFile](TlsConfig& config)
                {
                    auto algStr=PluginList::linefromFile(algsFile);
                    std::vector<std::string> algs;
                    Utils::trimSplit(algs,algStr,':');
                    if (!algs.empty())
                    {
                        for (auto&& it:algs)
                        {
                            const auto& curveAlgName=it;
                            const CryptAlgorithm* alg=nullptr;
                            auto ec=plugin->findAlgorithm(alg,CryptAlgorithm::Type::ECDH,curveAlgName);
                            if (!ec && alg)
                            {
                                config.clientECAlgs.push_back(alg);
                                config.serverECAlgs.push_back(alg);
                            }
                        }
                    }
                };
                checkHandshake(this,plugin,algName,pathPrefix,setupCfg);
            }
        }
    };
    checkAlg(algHandler);
}

BOOST_FIXTURE_TEST_CASE(CheckTlsHandshakeECDHAlgsMismatched,Env)
{
    auto algHandler=[this](std::shared_ptr<CryptPlugin>& plugin,const std::string& algName,const std::string& pathPrefix)
    {
        if (plugin->isFeatureImplemented(Crypt::Feature::ECDH))
        {
            std::string algsFile1=fmt::format("{}-ecdh-curves2.txt",pathPrefix);
            std::string algsFile2=fmt::format("{}-ecdh-curves3.txt",pathPrefix);
            if (boost::filesystem::exists(algsFile1) && boost::filesystem::exists(algsFile2))
            {
                auto setupCfg=[plugin,algsFile1,algsFile2](TlsConfig& config)
                {
                    auto algStr1=PluginList::linefromFile(algsFile1);
                    std::vector<std::string> algs1;
                    Utils::trimSplit(algs1,algStr1,':');
                    if (!algs1.empty())
                    {
                        for (auto&& it:algs1)
                        {
                            const auto& curveAlgName=it;
                            const CryptAlgorithm* alg=nullptr;
                            auto ec=plugin->findAlgorithm(alg,CryptAlgorithm::Type::ECDH,curveAlgName);
                            if (!ec && alg)
                            {
                                config.clientECAlgs.push_back(alg);
                            }
                        }
                    }
                    auto algStr2=PluginList::linefromFile(algsFile2);
                    std::vector<std::string> algs2;
                    Utils::trimSplit(algs2,algStr2,':');
                    if (!algs2.empty())
                    {
                        for (auto&& it:algs2)
                        {
                            const auto& curveAlgName=it;
                            const CryptAlgorithm* alg=nullptr;
                            auto ec=plugin->findAlgorithm(alg,CryptAlgorithm::Type::ECDH,curveAlgName);
                            if (!ec && alg)
                            {
                                config.serverECAlgs.push_back(alg);
                            }
                        }
                    }
                };
                checkHandshake(this,plugin,algName,pathPrefix,setupCfg,
                               true,
                               std::function<void (const Error&,SharedPtr<SecureStreamV> clientStream)>(),
                               true
                               );
            }
        }
    };
    checkAlg(algHandler);
}

BOOST_FIXTURE_TEST_CASE(CheckTlsHandshakeCipherSuitesOk,Env)
{
    auto algHandler=[this](std::shared_ptr<CryptPlugin>& plugin,const std::string& algName,const std::string& pathPrefix)
    {
        std::string suitesFile1=fmt::format("{}-ciphersuites1.txt",pathPrefix);
        if (boost::filesystem::exists(suitesFile1))
        {
            auto setupCfg=[plugin,suitesFile1](TlsConfig& config)
            {
                auto suitesStr1=PluginList::linefromFile(suitesFile1);
                std::vector<std::string> suites1;
                Utils::trimSplit(suites1,suitesStr1,':');
                if (!suites1.empty())
                {
                    config.clientCipherSuites=suites1;
                    config.serverCipherSuites=suites1;
                }
            };
            checkHandshake(this,plugin,algName,pathPrefix,setupCfg);
        }
    };
    checkAlg(algHandler);
}

BOOST_FIXTURE_TEST_CASE(CheckTlsHandshakeCipherSuitesMismatched,Env)
{
    auto algHandler=[this](std::shared_ptr<CryptPlugin>& plugin,const std::string& algName,const std::string& pathPrefix)
    {
        if (plugin->isFeatureImplemented(Crypt::Feature::ECDH))
        {
            std::string suitesFile1=fmt::format("{}-ciphersuites1.txt",pathPrefix);
            std::string suitesFile2=fmt::format("{}-ciphersuites2.txt",pathPrefix);
            if (boost::filesystem::exists(suitesFile1) && boost::filesystem::exists(suitesFile2))
            {
                auto setupCfg=[plugin,suitesFile1,suitesFile2](TlsConfig& config)
                {
                    auto suitesStr1=PluginList::linefromFile(suitesFile1);
                    std::vector<std::string> suites1;
                    Utils::trimSplit(suites1,suitesStr1,':');
                    if (!suites1.empty())
                    {
                        config.clientCipherSuites=suites1;
                    }
                    auto suitesStr2=PluginList::linefromFile(suitesFile2);
                    std::vector<std::string> suites2;
                    Utils::trimSplit(suites2,suitesStr2,':');
                    if (!suites2.empty())
                    {
                        config.serverCipherSuites=suites2;
                    }
                };
                checkHandshake(this,plugin,algName,pathPrefix,setupCfg,
                               true,
                               std::function<void (const Error&,SharedPtr<SecureStreamV> clientStream)>(),
                               true
                               );
            }
        }
    };
    checkAlg(algHandler);
}

BOOST_FIXTURE_TEST_CASE(CheckTlsShutdownClient,Env)
{
    auto algHandler=[this](std::shared_ptr<CryptPlugin>& plugin,const std::string& algName,const std::string& pathPrefix)
    {
        ByteArray serverRd;
        serverRd.resize(4096);
        ByteArray clientRd;
        clientRd.resize(4096);

        auto clientCb=[&clientRd](const Error& ec,SharedPtr<SecureStreamV> clientStream)
        {
            HATN_REQUIRE(clientStream);
            BOOST_CHECK(!ec);

            auto readCb=[](const common::Error& ec1,size_t size)
            {
                if (ec1)
                {
                    G_DEBUG(fmt::format("Client read failed ({}): {}",ec1.value(),ec1.message()));
                }
                else
                {
                    G_DEBUG(fmt::format("Client read size={}",size));
                }
                BOOST_CHECK(ec1);
            };
            clientStream->read(clientRd.data(),clientRd.size(),readCb);

            Thread::currentThread()->installTimer(
                1000000,
                [clientStream]()
                {
                    auto cb=[](const common::Error &ec1)
                    {
                        G_DEBUG(fmt::format("Client shutdown ({}): {}",ec1.value(),ec1.message()));
                        BOOST_CHECK(!ec1);
                    };

                    G_DEBUG("Closing client");
                    clientStream->close(cb);
                    return true;
                },
                true
            );
        };
        auto serverCb=[serverRd](const Error& ec,SharedPtr<SecureStreamV> serverStream)
        {
            HATN_REQUIRE(serverStream);
            BOOST_CHECK(!ec);

            auto readCb=[serverRd](const common::Error& ec1,size_t size)
            {
                if (ec1)
                {
                    G_DEBUG(fmt::format("Server read failed ({}): {}",ec1.value(),ec1.message()));
                }
                else
                {
                    G_DEBUG(fmt::format("Server read size={}",size));
                }
                BOOST_CHECK(ec1);
            };
            serverStream->read(serverRd.data(),serverRd.size(),readCb);
        };
        checkHandshake(this,plugin,algName,pathPrefix,std::function<void (TlsConfig&)>(),false,clientCb,false,serverCb,3);
    };
    checkAlg(algHandler);
}

BOOST_FIXTURE_TEST_CASE(CheckTlsShutdownServer,Env)
{
    auto algHandler=[this](std::shared_ptr<CryptPlugin>& plugin,const std::string& algName,const std::string& pathPrefix)
    {
        ByteArray serverRd;
        serverRd.resize(4096);
        ByteArray clientRd;
        clientRd.resize(4096);

        auto clientCb=[&clientRd](const Error& ec,SharedPtr<SecureStreamV> clientStream)
        {
            HATN_REQUIRE(clientStream);
            BOOST_CHECK(!ec);

            auto readCb=[](const common::Error& ec1,size_t size)
            {
                if (ec1)
                {
                    G_DEBUG(fmt::format("Client read failed ({}): {}",ec1.value(),ec1.message()));
                }
                else
                {
                    G_DEBUG(fmt::format("Client read size={}",size));
                }
                BOOST_CHECK(ec1);
            };
            clientStream->read(clientRd.data(),clientRd.size(),readCb);
        };
        auto serverCb=[serverRd](const Error& ec,SharedPtr<SecureStreamV> serverStream)
        {
            HATN_REQUIRE(serverStream);
            BOOST_CHECK(!ec);

            auto readCb=[serverRd](const common::Error& ec1,size_t size)
            {
                if (ec1)
                {
                    G_DEBUG(fmt::format("Server read failed ({}): {}",ec1.value(),ec1.message()));
                }
                else
                {
                    G_DEBUG(fmt::format("Server read size={}",size));
                }
                BOOST_CHECK(ec1);
            };
            serverStream->read(serverRd.data(),serverRd.size(),readCb);

            Thread::currentThread()->installTimer(
                1000000,
                [serverStream]()
                {
                    auto cb=[](const common::Error &ec1)
                    {
                        G_DEBUG(fmt::format("Server shutdown ({}): {}",ec1.value(),ec1.message()));
                        BOOST_CHECK(!ec1);
                    };

                    G_DEBUG("Closing server");
                    serverStream->close(cb);
                    return true;
                },
                true
            );
        };
        checkHandshake(this,plugin,algName,pathPrefix,std::function<void (TlsConfig&)>(),false,clientCb,false,serverCb,3);
    };
    checkAlg(algHandler);
}

BOOST_FIXTURE_TEST_CASE(CheckTlsShutdownBoth,Env)
{
    auto algHandler=[this](std::shared_ptr<CryptPlugin>& plugin,const std::string& algName,const std::string& pathPrefix)
    {
        ByteArray serverRd;
        serverRd.resize(4096);
        ByteArray clientRd;
        clientRd.resize(4096);

        auto clientCb=[&clientRd](const Error& ec,SharedPtr<SecureStreamV> clientStream)
        {
            HATN_REQUIRE(clientStream);
            BOOST_CHECK(!ec);

            auto readCb=[](const common::Error& ec1,size_t size)
            {
                if (ec1)
                {
                    G_DEBUG(fmt::format("Client read failed ({}): {}",ec1.value(),ec1.message()));
                }
                else
                {
                    G_DEBUG(fmt::format("Client read size={}",size));
                }
                BOOST_CHECK(ec1);
            };
            clientStream->read(clientRd.data(),clientRd.size(),readCb);

            Thread::currentThread()->installTimer(
                1000000,
                [clientStream]()
                {
                    auto cb=[](const common::Error &ec1)
                    {
                        G_DEBUG(fmt::format("Client shutdown ({}): {}",ec1.value(),ec1.message()));
                    };

                    G_DEBUG("Closing client");
                    clientStream->close(cb);
                    return true;
                },
                true
            );
        };
        auto serverCb=[serverRd](const Error& ec,SharedPtr<SecureStreamV> serverStream)
        {
            HATN_REQUIRE(serverStream);
            BOOST_CHECK(!ec);

            auto readCb=[serverRd](const common::Error& ec1,size_t size)
            {
                if (ec1)
                {
                    G_DEBUG(fmt::format("Server read failed ({}): {}",ec1.value(),ec1.message()));
                }
                else
                {
                    G_DEBUG(fmt::format("Server read size={}",size));
                }
                BOOST_CHECK(ec1);
            };
            serverStream->read(serverRd.data(),serverRd.size(),readCb);

            Thread::currentThread()->installTimer(
                1000000,
                [serverStream]()
                {
                    auto cb=[](const common::Error &ec1)
                    {
                        G_DEBUG(fmt::format("Server shutdown ({}): {}",ec1.value(),ec1.message()));
                    };

                    G_DEBUG("Closing server");
                    serverStream->close(cb);
                    return true;
                },
                true
            );
        };
        checkHandshake(this,plugin,algName,pathPrefix,std::function<void (TlsConfig&)>(),false,clientCb,false,serverCb,3);
    };
    checkAlg(algHandler);
}

static void readNext(
                 const Error& ec,
                 size_t size,
                 SharedPtr<SecureStreamV> stream,
                 ByteArray& rdBuf,
                 ByteArray& collectBuf,
                 const std::function<void(const Error&,SharedPtr<SecureStreamV>)>& doneCb,
                 size_t doneSize,
                 size_t shutdownSize=0,
                 const std::function<void(const Error&)>& shutdownCb=std::function<void(const Error&)>()
            )
{
    if (!ec)
    {
        HATN_REQUIRE(size>0);
        collectBuf.append(rdBuf.data(),size);

        HATN_DEBUG_LVL(global,5,HATN_FORMAT("readNext {}, size={}, complete {}/{}",stream->id(),size,collectBuf.size(),doneSize));

        if (collectBuf.size()>=doneSize)
        {
            G_DEBUG(fmt::format("{} read done",stream->id()));

            stream->read(rdBuf.data(),rdBuf.size(),
                [stream](const common::Error& ec1,size_t size1)
                {
                    G_DEBUG(fmt::format("{} readNext after done size={}, ec=({}): {}",stream->id(),size1,ec1.value(),ec1.message()));
                }
            );
            doneCb(Error(),stream);
            return;
        }

        stream->read(rdBuf.data(),rdBuf.size(),
            [&rdBuf,&collectBuf,stream,doneCb,doneSize,shutdownSize,shutdownCb](const common::Error& ec1,size_t size1)
            {
                readNext(ec1,size1,stream,rdBuf,collectBuf,doneCb,doneSize,shutdownSize,shutdownCb);
            }
        );

        if (shutdownSize>0 && collectBuf.size()>=shutdownSize)
        {
            G_DEBUG(fmt::format("{} shutdown",stream->id()));
            stream->close(
                [shutdownCb](const Error& ec)
                {
                    shutdownCb(ec);
                }
            );
        }
    }
    else
    {
        G_DEBUG(fmt::format("{} read failed ({}): {}",stream->id(),ec.value(),ec.message()));
        doneCb(ec,stream);
    }
}

static void writeNext(
                 const Error& ec,
                 size_t size,
                 SharedPtr<SecureStreamV> stream,
                 const ByteArray& wrBuf,
                 size_t offset,
                 const std::function<void(const Error&,SharedPtr<SecureStreamV>)>& doneCb
            )
{
    if (!ec)
    {
        HATN_REQUIRE(size>0);

        offset+=size;

        HATN_DEBUG_LVL(global,5,HATN_FORMAT("writeNext {}, size={}, complete {}/{}",stream->id(),size,offset,wrBuf.size()));

        if (offset==wrBuf.size())
        {
            G_DEBUG(fmt::format("{} write done",stream->id()));
            doneCb(Error(),stream);
            return;
        }

        size_t leftSize=wrBuf.size()-offset;
        size_t nextSize=Random::uniform(1,static_cast<uint32_t>(leftSize));

        HATN_DEBUG_LVL(global,5,HATN_FORMAT("writeNext {}, writing {}",stream->id(),nextSize));

        stream->write(wrBuf.data()+offset,nextSize,
            [&wrBuf,stream,offset,doneCb](const common::Error& ec1,size_t size1)
            {
                writeNext(ec1,size1,stream,wrBuf,offset,doneCb);
            }
        );
    }
    else
    {
        G_DEBUG(fmt::format("{} write failed ({}): {}",stream->id(),ec.value(),ec.message()));
        doneCb(ec,stream);
    }
}

#ifdef BUILD_VALGRIND
constexpr static const size_t testBufSize=0x80000;
#else
constexpr static const size_t testBufSize=0x400000;
#endif
constexpr static const size_t rdBufSize=0x10000;

BOOST_FIXTURE_TEST_CASE(CheckTlsReadWrite,Env)
{
    auto algHandler=[this](std::shared_ptr<CryptPlugin>& plugin,const std::string& algName,const std::string& pathPrefix)
    {
        ByteArray serverRd;
        serverRd.resize(rdBufSize);
        ByteArray clientRd;
        clientRd.resize(rdBufSize);

        ByteArray clientWriteBuf;
        auto ec=plugin->randContainer(clientWriteBuf,testBufSize);
        HATN_REQUIRE(!ec);
        ByteArray serverWriteBuf;
        ec=plugin->randContainer(serverWriteBuf,testBufSize);
        HATN_REQUIRE(!ec);

        SharedPtr<SecureStreamV> clientStream,serverStream;
        ByteArray clientRecvBuf,serverRecvBuf;

        size_t doneCount=0;
        size_t closeCount=0;

        auto doneCb=[this,&clientStream,&serverStream,&doneCount,&closeCount,&serverRecvBuf,&clientWriteBuf,&clientRecvBuf,&serverWriteBuf]()
        {
            if (++doneCount==4)
            {
                BOOST_CHECK(serverRecvBuf==clientWriteBuf);
                BOOST_CHECK(clientRecvBuf==serverWriteBuf);

                if (clientStream)
                {
                    auto closeCb1=[&closeCount,this](const Error& ec)
                    {
                        G_DEBUG(fmt::format("client shutdown done ({}): {}",ec.value(),ec.message()));
                        if (++closeCount==2)
                        {
                            quit();
                        }
                    };
                    clientStream->close(closeCb1);
                    if (serverStream)
                    {
                        auto closeCb2=[&closeCount,this](const Error& ec)
                        {
                            G_DEBUG(fmt::format("server shutdown done ({}): {}",ec.value(),ec.message()));
                            if (++closeCount==2)
                            {
                                quit();
                            }
                        };
                        serverStream->close(closeCb2);
                    }
                    else
                    {
                        BOOST_CHECK(false);
                        quit();
                    }
                }
                else
                {
                    BOOST_CHECK(false);
                    quit();
                }
            }
        };

        auto handshakeCb=[doneCb](const Error& ec,SharedPtr<SecureStreamV> stream,
                    ByteArray& rdBuf,
                    ByteArray& collectBuf,
                    ByteArray& wrBuf
                )
        {
            G_DEBUG(fmt::format("{} handshaking done ({}): {}",stream->id(),ec.value(),ec.message()));

            HATN_REQUIRE(stream);
            BOOST_CHECK(!ec);

            auto streamDoneCb=[doneCb](const Error& ec,SharedPtr<SecureStreamV> stream)
            {
                G_DEBUG(fmt::format("{} operation done ({}): {}",stream->id(),ec.value(),ec.message()));
                BOOST_CHECK(!ec);
                doneCb();
            };

            auto readCb=[&rdBuf,stream,streamDoneCb,&collectBuf](const common::Error& ec1,size_t size)
            {
                readNext(ec1,size,stream,rdBuf,collectBuf,streamDoneCb,testBufSize);
            };
            stream->read(rdBuf.data(),rdBuf.size(),readCb);

            auto writeCb=[&wrBuf,stream,streamDoneCb](const common::Error& ec1,size_t size)
            {
                writeNext(ec1,size,stream,wrBuf,0,streamDoneCb);
            };
            size_t wrSize=Random::uniform(1,testBufSize);
            stream->write(wrBuf.data(),wrSize,writeCb);
        };

        auto clientCb=[handshakeCb,&clientStream,&clientRd,&clientRecvBuf,&clientWriteBuf](const Error& ec,const SharedPtr<SecureStreamV>& stream)
        {
            clientStream=stream;
            handshakeCb(ec,stream,clientRd,clientRecvBuf,clientWriteBuf);
        };
        auto serverCb=[handshakeCb,&serverStream,&serverRd,&serverRecvBuf,&serverWriteBuf](const Error& ec,const SharedPtr<SecureStreamV>& stream)
        {
            serverStream=stream;
            handshakeCb(ec,stream,serverRd,serverRecvBuf,serverWriteBuf);
        };
        checkHandshake(this,plugin,algName,pathPrefix,std::function<void (TlsConfig&)>(),false,clientCb,false,serverCb,30);

        BOOST_CHECK_EQUAL(doneCount,4);
    };
    checkAlg(algHandler);
}

BOOST_FIXTURE_TEST_CASE(CheckTlsReadWriteUniDir,Env)
{
    auto algHandler=[this](std::shared_ptr<CryptPlugin>& plugin,const std::string& algName,const std::string& pathPrefix)
    {
        ByteArray serverRd;
        serverRd.resize(rdBufSize);
        ByteArray clientRd;
        clientRd.resize(rdBufSize);

        ByteArray clientWriteBuf;
        auto ec=plugin->randContainer(clientWriteBuf,testBufSize);
        HATN_REQUIRE(!ec);
        ByteArray serverWriteBuf;
        ec=plugin->randContainer(serverWriteBuf,testBufSize);
        HATN_REQUIRE(!ec);

        SharedPtr<SecureStreamV> clientStream,serverStream;
        ByteArray clientRecvBuf,serverRecvBuf;

        size_t doneCount=0;

        auto doneCb=[this,&clientStream,&doneCount,&serverRecvBuf,&clientWriteBuf]()
        {
            ++doneCount;
            G_DEBUG(fmt::format("all done {}",doneCount));

            if (doneCount==2)
            {
                BOOST_CHECK(serverRecvBuf==clientWriteBuf);

                if (clientStream)
                {
                    auto closeCb1=[this](const Error& ec)
                    {
                        G_DEBUG(fmt::format("client shutdown done ({}): {}",ec.value(),ec.message()));
                        quit();
                    };
                    clientStream->close(closeCb1);
                }
                else
                {
                    BOOST_CHECK(false);
                    quit();
                }
            }
        };

        auto handshakeCb=[doneCb](const Error& ec,SharedPtr<SecureStreamV> stream,
                    ByteArray& rdBuf,
                    ByteArray& collectBuf,
                    ByteArray& wrBuf
                )
        {
            G_DEBUG(fmt::format("{} handshaking done ({}): {}",stream->id(),ec.value(),ec.message()));

            HATN_REQUIRE(stream);
            BOOST_CHECK(!ec);

            auto streamDoneCb=[doneCb](const Error& ec,SharedPtr<SecureStreamV> stream)
            {
                G_DEBUG(fmt::format("{} operation done ({}): {}",stream->id(),ec.value(),ec.message()));
                BOOST_CHECK(!ec);
                doneCb();
            };

            if (stream->id()=="server")
            {
                auto readCb=[&rdBuf,stream,streamDoneCb,&collectBuf](const common::Error& ec1,size_t size)
                {
                    G_DEBUG(fmt::format("Server readCb size={}, status={} ({})",size,ec1.value(),ec1.message()));
                    readNext(ec1,size,stream,rdBuf,collectBuf,streamDoneCb,testBufSize);
                };
                stream->read(rdBuf.data(),rdBuf.size(),readCb);
            }
            else
            {
                auto readCb=[](const common::Error& ec1,size_t size)
                {
                    G_DEBUG(fmt::format("Client readCb size={}, status={} ({})",size,ec1.value(),ec1.message()));
                };
                stream->read(rdBuf.data(),rdBuf.size(),readCb);
            }

            if (stream->id()=="client")
            {
                auto writeCb=[&wrBuf,stream,streamDoneCb](const common::Error& ec1,size_t size)
                {
                    G_DEBUG(fmt::format("Client writeCb size={}, status={} ({})",size,ec1.value(),ec1.message()));
                    writeNext(ec1,size,stream,wrBuf,0,streamDoneCb);
                };
                size_t wrSize=Random::uniform(1,testBufSize);
                G_DEBUG(fmt::format("Client write size={}",wrSize));
                stream->write(wrBuf.data(),wrSize,writeCb);
            }
        };

        auto clientCb=[handshakeCb,&clientStream,&clientRd,&clientRecvBuf,&clientWriteBuf](const Error& ec,const SharedPtr<SecureStreamV>& stream)
        {
            clientStream=stream;
            handshakeCb(ec,stream,clientRd,clientRecvBuf,clientWriteBuf);
        };
        auto serverCb=[handshakeCb,&serverStream,&serverRd,&serverRecvBuf,&serverWriteBuf](const Error& ec,const SharedPtr<SecureStreamV>& stream)
        {
            serverStream=stream;
            handshakeCb(ec,stream,serverRd,serverRecvBuf,serverWriteBuf);
        };
        checkHandshake(this,plugin,algName,pathPrefix,std::function<void (TlsConfig&)>(),false,clientCb,false,serverCb,30);

        BOOST_CHECK_EQUAL(doneCount,2);
    };
    checkAlg(algHandler);
}

BOOST_FIXTURE_TEST_CASE(CheckTlsReadWriteShutdown,Env)
{
    auto algHandler=[this](std::shared_ptr<CryptPlugin>& plugin,const std::string& algName,const std::string& pathPrefix)
    {
        SharedPtr<SecureStreamV> clientStream;
        {
            ByteArray serverRd;
            serverRd.resize(rdBufSize);
            ByteArray clientRd;
            clientRd.resize(rdBufSize);

            ByteArray clientWriteBuf;
            auto ec=plugin->randContainer(clientWriteBuf,testBufSize);
            HATN_REQUIRE(!ec);
            ByteArray serverWriteBuf;
            ec=plugin->randContainer(serverWriteBuf,testBufSize);
            HATN_REQUIRE(!ec);

            SharedPtr<SecureStreamV> serverStream;
            ByteArray clientRecvBuf,serverRecvBuf;

            size_t doneCount=0;

            TlsConfig* config=nullptr;
            auto setupCfg=[&config](TlsConfig& cfg)
            {
                config=&cfg;
            };

            auto doneCb=[this,&doneCount,&serverRecvBuf,&clientWriteBuf,&clientRecvBuf,&serverWriteBuf]()
            {
                ++doneCount;
                G_DEBUG(fmt::format("doneCb count={}",doneCount));
                if (doneCount==5)
                {
                    BOOST_CHECK(serverRecvBuf!=clientWriteBuf);
                    BOOST_CHECK(clientRecvBuf!=serverWriteBuf);
                    Thread::currentThread()->execAsync(
                        [this]()
                        {
                            quit();
                        }
                    );
                }
            };

            auto handshakeCb=[doneCb](const Error& ec,SharedPtr<SecureStreamV> stream,
                        ByteArray& rdBuf,
                        ByteArray& collectBuf,
                        ByteArray& wrBuf
                    )
            {
                G_DEBUG(fmt::format("{} handshaking done ({}): {}",stream->id(),ec.value(),ec.message()));

                HATN_REQUIRE(stream);
                BOOST_CHECK(!ec);

                auto streamDoneCb=[doneCb](const Error& ec,SharedPtr<SecureStreamV> stream, const std::string& rdwr)
                {
                    G_DEBUG(fmt::format("{} operation {} done ({}): {}",stream->id(),rdwr,ec.value(),ec.message()));
                    BOOST_CHECK(ec);
                    doneCb();
                };

                auto shutdownDoneCb=[doneCb](const Error& ec)
                {
                    G_DEBUG(fmt::format("client operation shutdown done ({}): {}",ec.value(),ec.message()));
                    doneCb();
                };

                auto readCb=[&rdBuf,stream,streamDoneCb,shutdownDoneCb,&collectBuf](const common::Error& ec1,size_t size)
                {
                    size_t shutdownSize=0;
                    if (stream->id()=="client")
                    {
                        shutdownSize=testBufSize/2;
                    }

                    readNext(ec1,size,stream,rdBuf,collectBuf,
                             [streamDoneCb](const Error& ec,const SharedPtr<SecureStreamV>& stream)
                             {streamDoneCb(ec,stream,"read");},
                             testBufSize,shutdownSize,shutdownDoneCb);
                };
                stream->read(rdBuf.data(),rdBuf.size(),readCb);

                auto writeCb=[&wrBuf,stream,streamDoneCb](const common::Error& ec1,size_t size)
                {
                    writeNext(ec1,size,stream,wrBuf,0,
                                [streamDoneCb](const Error& ec,const SharedPtr<SecureStreamV>& stream)
                                {streamDoneCb(ec,stream,"write");}
                    );
                };
                size_t wrSize=Random::uniform(1,testBufSize);
                stream->write(wrBuf.data(),wrSize,writeCb);
            };

            auto clientCb=[handshakeCb,&clientStream,&clientRd,&clientRecvBuf,&clientWriteBuf](const Error& ec,const SharedPtr<SecureStreamV>& stream)
            {
                clientStream=stream;
                handshakeCb(ec,stream,clientRd,clientRecvBuf,clientWriteBuf);
            };
            auto serverCb=[handshakeCb,&serverStream,&serverRd,&serverRecvBuf,&serverWriteBuf](const Error& ec,const SharedPtr<SecureStreamV>& stream)
            {
                serverStream=stream;
                handshakeCb(ec,stream,serverRd,serverRecvBuf,serverWriteBuf);
            };
            checkHandshake(this,plugin,algName,pathPrefix,setupCfg,false,clientCb,false,serverCb,3);

            // sometimes shutdown is not fully completed on the other side
            BOOST_CHECK_GE(doneCount,3u);
        }
    };
    checkAlg(algHandler);
}

BOOST_FIXTURE_TEST_CASE(CheckTlsShutdownTimeout,Env)
{
#if 0
    auto err=makeSystemError(std::errc::connection_aborted);
    BOOST_TEST_MESSAGE(err.message());
#endif
    auto algHandler=[this](std::shared_ptr<CryptPlugin>& plugin,const std::string& algName,const std::string& pathPrefix)
    {
        createThreads();
        thread(0)->start();

        auto setupCfg=[this](TlsConfig& cfg)
        {
            cfg.thread=this->thread(0).get();
        };

        ByteArray serverRd;
        serverRd.resize(rdBufSize);
        ByteArray clientRd;
        clientRd.resize(rdBufSize);

        ByteArray clientWriteBuf;
        auto ec=plugin->randContainer(clientWriteBuf,testBufSize);
        HATN_REQUIRE(!ec);
        ByteArray serverWriteBuf;
        ec=plugin->randContainer(serverWriteBuf,testBufSize);
        HATN_REQUIRE(!ec);

        SharedPtr<SecureStreamV> clientStream,serverStream;
        ByteArray clientRecvBuf,serverRecvBuf;

        size_t doneCount=0;

        auto doneCb=[this,&serverStream,&doneCount,&serverRecvBuf,&clientWriteBuf]()
        {
            ++doneCount;
            G_DEBUG(fmt::format("all done {}",doneCount));

            if (doneCount==2)
            {
                BOOST_CHECK(serverRecvBuf==clientWriteBuf);

                if (serverStream)
                {
                    auto closeCb1=[this](const Error& ec)
                    {
                        G_DEBUG(fmt::format("server shutdown done ({}): {}",ec.value(),ec.message()));
                        BOOST_CHECK(ec);
                        quit();
                    };
                    serverStream->close(closeCb1);
                }
                else
                {
                    BOOST_CHECK(false);
                    quit();
                }
            }
        };

        auto handshakeCb=[doneCb](const Error& ec,SharedPtr<SecureStreamV> stream,
                    ByteArray& rdBuf,
                    ByteArray& collectBuf,
                    ByteArray& wrBuf
                )
        {
            G_DEBUG(fmt::format("{} handshaking done ({}): {}",stream->id(),ec.value(),ec.message()));

            HATN_REQUIRE(stream);
            BOOST_CHECK(!ec);

            auto streamDoneCb=[doneCb](const Error& ec,SharedPtr<SecureStreamV> stream)
            {
                G_DEBUG(fmt::format("{} operation done ({}): {}",stream->id(),ec.value(),ec.message()));
                BOOST_CHECK(!ec);
                doneCb();
            };

            if (stream->id()=="server")
            {
                auto readCb=[&rdBuf,stream,streamDoneCb,&collectBuf](const common::Error& ec1,size_t size)
                {
                    readNext(ec1,size,stream,rdBuf,collectBuf,streamDoneCb,testBufSize);
                };
                stream->read(rdBuf.data(),rdBuf.size(),readCb);
            }

            if (stream->id()=="client")
            {
                auto writeCb=[&wrBuf,stream,streamDoneCb](const common::Error& ec1,size_t size)
                {
                    writeNext(ec1,size,stream,wrBuf,0,streamDoneCb);
                };
                size_t wrSize=Random::uniform(1,testBufSize);
                stream->write(wrBuf.data(),wrSize,writeCb);
            }
        };

        auto clientCb=[handshakeCb,&clientStream,&clientRd,&clientRecvBuf,&clientWriteBuf](const Error& ec,const SharedPtr<SecureStreamV>& stream)
        {
            clientStream=stream;
            handshakeCb(ec,stream,clientRd,clientRecvBuf,clientWriteBuf);
        };
        auto serverCb=[handshakeCb,&serverStream,&serverRd,&serverRecvBuf,&serverWriteBuf](const Error& ec,const SharedPtr<SecureStreamV>& stream)
        {
            serverStream=stream;
            handshakeCb(ec,stream,serverRd,serverRecvBuf,serverWriteBuf);
        };
        checkHandshake(this,plugin,algName,pathPrefix,setupCfg,false,clientCb,false,serverCb,30);

        thread(0)->stop();
        BOOST_CHECK_EQUAL(doneCount,2);
    };
    checkAlg(algHandler);
}

BOOST_AUTO_TEST_SUITE_END()

}
}
