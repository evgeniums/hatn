/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientservertests/test/testhssauth.cpp
  */

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include "hatn_test_config.h"
#include <hatn/test/multithreadfixture.h>

#include <hatn/common/random.h>
#include <hatn/common/containerutils.h>

#include <hatn/logcontext/streamlogger.h>
#include <hatn/logcontext/context.h>

#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>
#include <hatn/dataunit/ipp/objectid.ipp>

#include <hatn/app/app.h>

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

#include <hatn/api/server/plaintcpmicroservice.h>
#include <hatn/api/server/microservicefactory.h>

#include <hatn/clientserver/plaintcpclientwithauth.h>

#include <hatn/serverapp/auth/authservice.h>
#include <hatn/serverapp/auth/authprotocols.h>
#include <hatn/serverapp/auth/sharedsecretprotocol.h>
#include <hatn/serverapp/auth/sessionauthdispatcher.h>
#include <hatn/serverapp/logincontroller.h>
#include <hatn/serverapp/localusercontroller.h>
#include <hatn/serverapp/localsessioncontroller.h>
#include <hatn/serverapp/sessiondbmodelsprovider.h>
#include <hatn/serverapp/userdbmodelsprovider.h>

#include <hatn/api/ipp/auth.ipp>
#include <hatn/api/ipp/message.ipp>
#include <hatn/api/ipp/methodauth.ipp>
#include <hatn/api/ipp/serverresponse.ipp>
#include <hatn/api/ipp/serverservice.ipp>
#include <hatn/api/ipp/makeapierror.ipp>
#include <hatn/api/ipp/networkmicroservice.ipp>
#include <hatn/api/ipp/plaintcpmicroservice.ipp>
#include <hatn/api/ipp/serverenv.ipp>
#include <hatn/api/ipp/session.ipp>

#include <hatn/serverapp/ipp/localsessioncontroller.ipp>
#include <hatn/serverapp/ipp/sharedsecretprotocol.ipp>
#include <hatn/serverapp/ipp/logincontroller.ipp>
#include <hatn/serverapp/ipp/localusercontroller.ipp>
#include <hatn/serverapp/ipp/authservice.ipp>
#include <hatn/serverapp/ipp/sessionauthdispatcher.ipp>

#include <hatn/clientserver/ipp/clientsession.ipp>

using namespace std::chrono_literals;

HATN_TEST_USING

HATN_APP_USING
HATN_API_USING
HATN_COMMON_USING
HATN_LOGCONTEXT_USING
HATN_NETWORK_USING
HATN_SERVERAPP_USING
HATN_CLIENT_SERVER_USING

/********************** TestEnv **************************/

struct TestEnv : public MultiThreadFixture
{
    TestEnv()
    {
        reset();
    }

    ~TestEnv()
    {
        reset();
    }

    void reset()
    {
        testAmount=0;
        testUserTopic.clear();
        testLogin.reset();
        testUser.reset();
    }

    TestEnv(const TestEnv&)=delete;
    TestEnv(TestEnv&&) =delete;
    TestEnv& operator=(const TestEnv&)=delete;
    TestEnv& operator=(TestEnv&&) =delete;

    static int testAmount;
    static HATN_DATAUNIT_NAMESPACE::ObjectId testLogin;
    static HATN_DATAUNIT_NAMESPACE::ObjectId testUser;
    static std::string testUserTopic;
    static ByteArrayShared sessionToken;
};
int TestEnv::testAmount=0;
HATN_DATAUNIT_NAMESPACE::ObjectId TestEnv::testLogin;
HATN_DATAUNIT_NAMESPACE::ObjectId TestEnv::testUser;
std::string TestEnv::testUserTopic;
ByteArrayShared TestEnv::sessionToken;

constexpr const uint32_t TcpPort=53852;

/********************** Client **************************/

auto createClientApp(std::string configFileName)
{
    AppName appName{"authclient","Auth Client"};
    auto app=std::make_shared<App>(appName);
    auto configFile=MultiThreadFixture::assetsFilePath("clientservertests",configFileName);
    auto ec=app->loadConfigFile(configFile);
    HATN_TEST_EC(ec);
    BOOST_REQUIRE(!ec);
    ec=app->init();
    HATN_TEST_EC(ec);
    BOOST_REQUIRE(!ec);
    return app;
}

auto createClient(std::string sharedSecret, std::shared_ptr<App> app, std::string login={}, std::string topic={})
{
    auto thread=app->appThread();

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

    const pmr::AllocatorFactory* factory=app->allocatorFactory().factory();

    auto ctx=makePlainTcpClientWithAuthContext(
        subcontexts(
            subcontext(),
            subcontext(std::move(router),thread,factory),
            subcontext(factory),
            subcontext()
            )
        );
    auto& logContext=ctx->get<HATN_LOGCONTEXT_NAMESPACE::Context>();
    logContext.setLogger(app->logger().logger());

    auto& client=ctx->get<PlainTcpClientWithAuth>();

    auto sharedSecretAuth=client.sharedSecretAuth();
    HATN_CRYPT_NAMESPACE::SecurePlainContainer sharedSecretContainer;
    sharedSecretContainer.loadContent(sharedSecret);
    sharedSecretAuth->setSharedSecret(std::move(sharedSecretContainer));
    sharedSecretAuth->setCipherSuites(app->cipherSuites().suites());

    auto calcSs1=sharedSecretAuth->calculateSharedSecret("login1",sharedSecret,app->defaultCipherSuiteId());
    if (calcSs1)
    {
        HATN_TEST_EC(calcSs1.error())
    }
    BOOST_REQUIRE(!calcSs1);
    std::string calcSs1Str;
    ContainerUtils::rawToBase64(calcSs1.value().content(),calcSs1Str);
    BOOST_TEST_MESSAGE(fmt::format("calculated shared secret for login1: {}",calcSs1Str));

    auto calcSs2=sharedSecretAuth->calculateSharedSecret("login2",sharedSecret,app->defaultCipherSuiteId());
    if (calcSs2)
    {
        HATN_TEST_EC(calcSs2.error())
    }
    BOOST_REQUIRE(!calcSs2);
    BOOST_CHECK(!calcSs2.value().content().empty());
    BOOST_CHECK(!(calcSs1.value()==calcSs2.value()));
    std::string calcSs2Str;
    ContainerUtils::rawToBase64(calcSs2.value().content(),calcSs2Str);
    BOOST_TEST_MESSAGE(fmt::format("calculated shared secret for login2: {}",calcSs2Str));

    auto calcSs3=sharedSecretAuth->calculateSharedSecret("login1",sharedSecret,app->defaultCipherSuiteId());
    if (calcSs3)
    {
        HATN_TEST_EC(calcSs3.error())
    }
    BOOST_REQUIRE(!calcSs3);
    BOOST_CHECK(calcSs1.value()==calcSs3.value());

    auto tokensUpdateCb=[](ByteArrayShared sessionToken,ByteArrayShared refreshToken)
    {
        HATN_TEST_MESSAGE_TS("In tokensUpdateCb");
        if (!sessionToken.isNull())
        {
            HATN_TEST_MESSAGE_TS("Session token updated");
            TestEnv::sessionToken=sessionToken;
            auto ec=sessionToken->saveToFile(MultiThreadFixture::tmpFilePath("session-token.dat"));
            HATN_TEST_EC(ec)
            BOOST_REQUIRE(!ec);
        }
        if (!refreshToken.isNull())
        {
            HATN_TEST_MESSAGE_TS("Refresh token updated");
        }
    };
    auto session=client.session();
    session->sessionImpl().setTokensUpdatedCb(std::move(tokensUpdateCb));
    session->sessionImpl().setLogin(login,topic);

    return std::make_pair(app,ctx);
}

//---------------------------------------------------------------

struct FooBar1;
struct FooBar2;

using EnvFooBar=Env<FooBar1,FooBar2>;

struct FooBar1
{
    int value()
    {
        return 0;
    }
};

struct FooBar2
{
    int value()
    {
        return env->get<FooBar1>().value();
    }

    EnvFooBar* env;
};

struct ServerApp
{
    std::shared_ptr<App> app;
    std::map<std::string,std::shared_ptr<server::MicroService>> microservices;
};

struct ContextTraits;

using EnvWithAuthT = Env<WithAuthProtocols,
                        WithSharedSecretAuthProtocol,
                        LoginController<ContextTraits>,
                        LocalUserController<ContextTraits>,
                        LocalSessionController<ContextTraits>,
                        server::BasicEnv>;

struct RequestWithAuth;

struct ContextTraits
{
    using Context=server::RequestContext<RequestWithAuth>;

    static auto& loginController(const SharedPtr<Context>& ctx);
    static auto& userController(const SharedPtr<Context>& ctx);
    static const auto* factory(const SharedPtr<Context>& ctx);
    static auto& contextDb(const SharedPtr<Context>& ctx);

    static auto& request(const SharedPtr<Context>& ctx);
};

class EnvWithAuth : public EnvWithAuthT
{
    public:

        using EnvWithAuthT::EnvWithAuthT;

        auto& loginController()
        {
            return get<LoginController<ContextTraits>>();
        }

        auto& sessionController()
        {
            return get<LocalSessionController<ContextTraits>>();
        }

        auto& userController()
        {
            return get<LocalUserController<ContextTraits>>();
        }
};

struct RequestWithAuth : public server::Request<EnvWithAuth>
{
    using server::Request<EnvWithAuth>::Request;

    RequestWithAuth& request()
    {
        return *this;
    }

    const RequestWithAuth& request() const
    {
        return *this;
    }
};

auto& ContextTraits::loginController(const SharedPtr<Context>& ctx)
{
    return server::requestEnv<RequestWithAuth>(ctx)->loginController();
}

auto& ContextTraits::userController(const SharedPtr<Context>& ctx)
{
    return server::requestEnv<RequestWithAuth>(ctx)->userController();
}

const auto* ContextTraits::factory(const SharedPtr<Context>& ctx)
{
    return server::request<RequestWithAuth>(ctx).request().factory();
}

auto& ContextTraits::contextDb(const SharedPtr<Context>& ctx)
{
    return server::request<RequestWithAuth>(ctx).request().db();
}

auto& ContextTraits::request(const SharedPtr<Context>& ctx)
{
    return server::request<RequestWithAuth>(ctx);
}

struct EnvWithAuthConfigTraits
{
    using Env=EnvWithAuth;
    using Request=RequestWithAuth;

    static std::shared_ptr<UserDbModels> userDbModels;
    static std::shared_ptr<SessionDbModels> sessionDbModels;
    static SharedPtr<Env> env;

    static Result<SharedPtr<Env>> makeEnv(
        const HATN_APP_NAMESPACE::App& app,
        const HATN_BASE_NAMESPACE::ConfigTree& configTree,
        const HATN_BASE_NAMESPACE::ConfigTreePath& configTreePath
    )
    {
        // allocate
        auto f1=app.allocatorFactory().factory();
        auto allocator=f1->objectAllocator<Env>();
        env=allocateEnvType<Env>(
            allocator,
            contexts(
                context(std::make_shared<AuthProtocols>()),
                context(std::make_shared<SharedSecretAuthProtocol>()),
                context(),
                context(userDbModels),
                context(sessionDbModels)
            ),
            context()
        );

        // init
        auto ec=server::BasicEnvConfig::initEnv(*env,app,configTree,configTreePath);
        HATN_TEST_EC(ec)
        HATN_CHECK_EC(ec)

        auto f2=env->get<AllocatorFactory>().factory();
        BOOST_REQUIRE_EQUAL(f1,f2);

        auto& hssProtocol=env->get<WithSharedSecretAuthProtocol>();

        // load config for SharedSecretAuthProtocol
        ec=hatn::loadLogConfig("configuration of shared secret authentication protocol",hssProtocol.value(),app.configTree(),"auth.hss");
        HATN_TEST_EC(ec)
        HATN_CHECK_EC(ec)

        // init SharedSecretAuthProtocol
        const auto& cipherSuitesCtx=env->get<CipherSuites>();
        ec=hssProtocol.value().init(cipherSuitesCtx.suites()->defaultSuite());
        HATN_TEST_EC(ec)
        HATN_CHECK_EC(ec)

        // load config for session controller
        ec=hatn::loadLogConfig("configuration of authentication sessions",env->sessionController(),app.configTree(),"auth.sessions");
        HATN_TEST_EC(ec)
        HATN_CHECK_EC(ec)

        // init session controller
        ec=env->sessionController().init(cipherSuitesCtx.suites());
        HATN_TEST_EC(ec)
        HATN_CHECK_EC(ec)

        // done
        return env;
    }
};
std::shared_ptr<UserDbModels> EnvWithAuthConfigTraits::userDbModels;
std::shared_ptr<SessionDbModels> EnvWithAuthConfigTraits::sessionDbModels;
SharedPtr<EnvWithAuthConfigTraits::Env> EnvWithAuthConfigTraits::env;

using ServiceDispatcherType=server::ServiceDispatcher<EnvWithAuth,RequestWithAuth>;
using AuthDispatcherType=SessionAuthDispatcher<EnvWithAuth,RequestWithAuth>;

using EnvWithAuthConfig=server::EnvConfigT<EnvWithAuthConfigTraits,RequestWithAuth>;
using MicroserviceBuilder=server::PlainTcpMicroServiceBuilderT<EnvWithAuthConfig,ServiceDispatcherType,AuthDispatcherType>;

HATN_TASK_CONTEXT_DECLARE(RequestWithAuth)
HATN_TASK_CONTEXT_DEFINE(RequestWithAuth,RequestWithAuth)

HATN_TASK_CONTEXT_DECLARE(server::WithEnv<EnvWithAuth>)
HATN_TASK_CONTEXT_DEFINE(server::WithEnv<EnvWithAuth>,EnvWithAuth)

HATN_TASK_CONTEXT_DECLARE(server::PlainTcpServerT<EnvWithAuth>)
HATN_TASK_CONTEXT_DEFINE(server::PlainTcpServerT<EnvWithAuth>,TcpServer)

HDU_UNIT(service1_msg1,
    HDU_FIELD(field1,TYPE_UINT32,1)
    HDU_FIELD(field2,TYPE_STRING,2)
)

class Service1Method1Traits : public server::NoValidatorTraits
{
    public:

        using Request=RequestWithAuth;
        using Message=service1_msg1::managed;

        void exec(
                SharedPtr<server::RequestContext<Request>> request,
                server::RouteCb<Request> callback,
                SharedPtr<Message> msg
            ) const
        {
            BOOST_TEST_MESSAGE(fmt::format("Service1 method1 exec: field1={}, field2={}",msg->fieldValue(service1_msg1::field1),msg->fieldValue(service1_msg1::field2)));

            TestEnv::testAmount+=msg->fieldValue(service1_msg1::field1);

            auto& req=request->get<Request>();

            BOOST_CHECK(req.login==TestEnv::testLogin);
            BOOST_CHECK(req.user==TestEnv::testUser);
            BOOST_CHECK_EQUAL(req.userTopic.topic(),TestEnv::testUserTopic);
            BOOST_CHECK(!req.sessionId.isNull());
            BOOST_CHECK(!req.sessionClientId.isNull());

            req.response.setSuccess();
            callback(std::move(request));
        }
};
class Service1Method1 : public server::ServiceMethodNV<Service1Method1Traits,Service1Method1Traits::Message,RequestWithAuth>
{
    public:

        using Base=server::ServiceMethodNV<Service1Method1Traits,Service1Method1Traits::Message,RequestWithAuth>;

        Service1Method1() : Base("service1_method1")
        {}
};
using Service1=server::ServerServiceV<server::ServiceSingleMethod<Service1Method1,RequestWithAuth>,RequestWithAuth>;

Result<ServerApp> createServer(std::string configFileName, int expectedErrorCode=0)
{
    std::string expectedFail;
    if (expectedErrorCode!=0)
    {
        expectedFail="expected: ";
    }

    EnvWithAuthConfigTraits::userDbModels=std::make_shared<UserDbModels>("admin");
    auto userDbModels=EnvWithAuthConfigTraits::userDbModels;
    EnvWithAuthConfigTraits::sessionDbModels=std::make_shared<SessionDbModels>("admin");
    auto sessionDbModels=EnvWithAuthConfigTraits::sessionDbModels;

    // init server app
    AppName appName{"authserver","Auth Server"};
    auto app=std::make_shared<App>(appName);
    app->setAppDataFolder(MultiThreadFixture::tmpFilePath("server-data"));
    auto ec=app->createAppDataFolder();
    HATN_TEST_EC(ec)
    BOOST_REQUIRE(!ec);
    auto configFile=MultiThreadFixture::assetsFilePath("clientservertests",configFileName);
    ec=app->loadConfigFile(configFile);
    if (ec)
    {
        BOOST_TEST_MESSAGE(fmt::format("{}failed to load app config: {}",expectedFail,ec.message()));
        if (expectedErrorCode!=0)
        {
            BOOST_CHECK_EQUAL(ec.value(),expectedErrorCode);
            return ec;
        }
    }
    BOOST_REQUIRE(!ec);
    ec=app->init();
    if (ec)
    {
        BOOST_TEST_MESSAGE(fmt::format("{}failed to init app: {}",expectedFail,ec.message()));
        if (expectedErrorCode!=0)
        {
            BOOST_CHECK_EQUAL(ec.value(),expectedErrorCode);
            return ec;
        }
    }
    BOOST_REQUIRE(!ec);

    // create service dispatcher
    auto serviceRouter=std::make_shared<server::ServiceRouter<EnvWithAuth,RequestWithAuth>>();
    auto service1=std::make_shared<Service1>("service1");
    serviceRouter->registerLocalService(std::move(service1));
    auto authService=std::make_shared<AuthService<RequestWithAuth>>();
    serviceRouter->registerLocalService(std::move(authService));    
    auto dispatcher=std::make_shared<ServiceDispatcherType>(serviceRouter);
    auto serviceDispatchers=std::make_shared<server::DispatchersStore<ServiceDispatcherType>>();
    serviceDispatchers->registerDispatcher("default",std::move(dispatcher));

    // create auth dispatcher
    auto authDispatcher=std::make_shared<AuthDispatcherType>();
    auto authDispatchers=std::make_shared<server::DispatchersStore<AuthDispatcherType>>();
    authDispatchers->registerDispatcher("token_session",std::move(authDispatcher));

    // prepare microservice factory
    MicroserviceBuilder microserviceBuilder{serviceDispatchers,authDispatchers};
    server::MicroServiceFactory factory;
    factory.registerBuilder("microservice1",microserviceBuilder);

    // create and run microservices
    auto microservicesR=factory.makeAndRunAll(*app,app->configTree());
    if (microservicesR)
    {
        BOOST_TEST_MESSAGE(fmt::format("{}failed to makeAndRunAll: {}",expectedFail,microservicesR.error().message()));
        if (expectedErrorCode!=0)
        {
            BOOST_CHECK_EQUAL(microservicesR.error().value(),expectedErrorCode);
            app->close();
            return microservicesR.takeError();
        }
    }
    BOOST_REQUIRE(!microservicesR);

    auto microservices=microservicesR.takeValue();
    BOOST_REQUIRE_EQUAL(microservices.size(),1);
    BOOST_CHECK_EQUAL(microservices.begin()->first,"microservice1");

    auto mainSchema=std::make_shared<HATN_DB_NAMESPACE::Schema>("main");
    mainSchema->addModelsProvider(std::make_shared<UserDbModelsProvider>(std::move(userDbModels)));
    mainSchema->addModelsProvider(std::make_shared<UserDbModelsProvider>());
    mainSchema->addModelsProvider(std::make_shared<SessionDbModelsProvider>(std::move(sessionDbModels)));
    mainSchema->addModelsProvider(std::make_shared<SessionDbModelsProvider>());
    app->registerDbSchema(mainSchema);

    ec=app->destroyDb();
    HATN_TEST_EC(ec)
    BOOST_REQUIRE(!ec);
    ec=app->openDb();
    HATN_TEST_EC(ec)
    BOOST_REQUIRE(!ec);

    ec=app->database().setSchema(mainSchema);
    HATN_TEST_EC(ec)
    BOOST_REQUIRE(!ec);

    return ServerApp{std::move(app),std::move(microservices)};
}

template <typename T>
void runCreateServer(std::string configFile, TestEnv* env, T expectedErrorCode)
{
    auto appCtx=createServer(configFile,static_cast<int>(expectedErrorCode));
    if (appCtx)
    {
        return;
    }

    BOOST_REQUIRE(appCtx->app);

    env->exec(3);

    for (auto&& it: appCtx->microservices)
    {
        it.second->close();
    }
    env->exec(1);
    appCtx->app->close();
}

/********************** Tests **************************/

enum class TestMode : int
{
    OK,
    SecondLoginOk,
    OtherUserOk,
    UknownLogin,
    UknownUser,
    InvalidSharedSecret,
    LoginBlocked,
    UserBlocked,
    LoginExpired,
    SessionTokenOk,
    SessionTokenExpired,
    SessionTokenExpiredReloginOk,
    SessionExpired,
    SessionExpiredReloginOk,
    SessionTokenInvalid,
    SessionTokenInvalidOk,
    AuthMissing
};

struct TestConfig
{
    const char* serverConfigFile="hssauthserver.jsonc";
    const char* clientConfigFile="hssauthclient.jsonc";
    const char* clientSharedSecret1="shared_secret1";
    const char* clientSharedSecret2="shared_secret2";
    const char* topic1="topic1";
    const char* topic2="topic2";
    int runningSecs=5;
};

template <typename TestEnvT, typename CallbackT>
void runTest(TestEnvT testEnv, CallbackT callback, TestMode mode, const TestConfig& config={})
{
    auto serverCtx=createServer(config.serverConfigFile);

    auto usr1=makeShared<user::managed>();
    HATN_DB_NAMESPACE::initObject(*usr1);
    BOOST_TEST_MESSAGE(fmt::format("usr1= {}",usr1->fieldValue(HATN_DB_NAMESPACE::object::_id).toString()));
    auto usr2=makeShared<user::managed>();
    HATN_DB_NAMESPACE::initObject(*usr2);
    BOOST_TEST_MESSAGE(fmt::format("usr2= {}",usr2->fieldValue(HATN_DB_NAMESPACE::object::_id).toString()));
    auto usr4=makeShared<user::managed>();
    HATN_DB_NAMESPACE::initObject(*usr4);
    usr4->setFieldValue(user::blocked,true);
    BOOST_TEST_MESSAGE(fmt::format("usr4= {}",usr4->fieldValue(HATN_DB_NAMESPACE::object::_id).toString()));

    auto login1_1=makeShared<login_profile::managed>();
    HATN_DB_NAMESPACE::initObject(*login1_1);
    login1_1->setFieldValue(with_user::user,usr1->fieldValue(HATN_DB_NAMESPACE::object::_id));
    login1_1->setFieldValue(login_profile::secret1,config.clientSharedSecret1);
    login1_1->setFieldValue(with_user::user_topic,config.topic1);
    BOOST_TEST_MESSAGE(fmt::format("login1_1= {}",login1_1->fieldValue(HATN_DB_NAMESPACE::object::_id).toString()));
    auto login1_2=makeShared<login_profile::managed>();
    HATN_DB_NAMESPACE::initObject(*login1_2);
    login1_2->setFieldValue(with_user::user,usr1->fieldValue(HATN_DB_NAMESPACE::object::_id));
    login1_2->setFieldValue(login_profile::secret1,config.clientSharedSecret2);
    login1_2->setFieldValue(with_user::user_topic,config.topic1);
    BOOST_TEST_MESSAGE(fmt::format("login1_2= {}",login1_2->fieldValue(HATN_DB_NAMESPACE::object::_id).toString()));

    auto login3_0=makeShared<login_profile::managed>();
    HATN_DB_NAMESPACE::initObject(*login3_0);
    login3_0->setFieldValue(with_user::user,HATN_DATAUNIT_NAMESPACE::ObjectId::generateId());
    login3_0->setFieldValue(login_profile::secret1,config.clientSharedSecret1);
    login3_0->setFieldValue(with_user::user_topic,config.topic1);
    BOOST_TEST_MESSAGE(fmt::format("login3_0= {}",login3_0->fieldValue(HATN_DB_NAMESPACE::object::_id).toString()));

    auto login2_1=makeShared<login_profile::managed>();
    HATN_DB_NAMESPACE::initObject(*login2_1);
    login2_1->setFieldValue(with_user::user,usr2->fieldValue(HATN_DB_NAMESPACE::object::_id));
    login2_1->setFieldValue(login_profile::secret1,config.clientSharedSecret1);
    login2_1->setFieldValue(with_user::user_topic,config.topic1);
    BOOST_TEST_MESSAGE(fmt::format("login2_1= {}",login2_1->fieldValue(HATN_DB_NAMESPACE::object::_id).toString()));
    auto login2_2=makeShared<login_profile::managed>();
    HATN_DB_NAMESPACE::initObject(*login2_2);
    login2_2->setFieldValue(with_user::user,usr2->fieldValue(HATN_DB_NAMESPACE::object::_id));
    login2_2->setFieldValue(login_profile::secret1,config.clientSharedSecret2);
    login2_2->setFieldValue(with_user::user_topic,config.topic1);
    login2_2->setFieldValue(login_profile::blocked,true);
    BOOST_TEST_MESSAGE(fmt::format("login2_2= {}",login2_2->fieldValue(HATN_DB_NAMESPACE::object::_id).toString()));
    auto login2_3=makeShared<login_profile::managed>();
    HATN_DB_NAMESPACE::initObject(*login2_3);
    login2_3->setFieldValue(with_user::user,usr2->fieldValue(HATN_DB_NAMESPACE::object::_id));
    login2_3->setFieldValue(login_profile::secret1,config.clientSharedSecret2);
    login2_3->setFieldValue(with_user::user_topic,config.topic1);
    auto expAt=DateTime::currentUtc();
    expAt.addDays(-1);
    login2_3->setFieldValue(login_profile::expire_at,expAt);
    BOOST_TEST_MESSAGE(fmt::format("login2_3= {}",login2_2->fieldValue(HATN_DB_NAMESPACE::object::_id).toString()));

    auto login4_1=makeShared<login_profile::managed>();
    HATN_DB_NAMESPACE::initObject(*login4_1);
    login4_1->setFieldValue(with_user::user,usr4->fieldValue(HATN_DB_NAMESPACE::object::_id));
    login4_1->setFieldValue(login_profile::secret1,config.clientSharedSecret1);
    login4_1->setFieldValue(with_user::user_topic,config.topic1);
    BOOST_TEST_MESSAGE(fmt::format("login4_1= {}",login4_1->fieldValue(HATN_DB_NAMESPACE::object::_id).toString()));

    TestEnv::testUser=usr1->fieldValue(HATN_DB_NAMESPACE::object::_id);
    TestEnv::testUserTopic=config.topic1;

    auto sharedSecret=config.clientSharedSecret1;
    auto loginOid=login1_1->fieldValue(HATN_DB_NAMESPACE::object::_id);    
    if (mode==TestMode::SecondLoginOk)
    {
        loginOid=login1_2->fieldValue(HATN_DB_NAMESPACE::object::_id);
        sharedSecret=config.clientSharedSecret2;
    }
    else if (mode==TestMode::UknownUser)
    {
        loginOid=login3_0->fieldValue(HATN_DB_NAMESPACE::object::_id);
    }
    else if (mode==TestMode::LoginBlocked)
    {
        loginOid=login2_2->fieldValue(HATN_DB_NAMESPACE::object::_id);
        sharedSecret=config.clientSharedSecret2;
    }
    else if (mode==TestMode::LoginExpired)
    {
        loginOid=login2_3->fieldValue(HATN_DB_NAMESPACE::object::_id);
        sharedSecret=config.clientSharedSecret2;
    }
    else if (mode==TestMode::UserBlocked)
    {
        loginOid=login4_1->fieldValue(HATN_DB_NAMESPACE::object::_id);
    }
    else if (mode==TestMode::OtherUserOk)
    {
        loginOid=login2_1->fieldValue(HATN_DB_NAMESPACE::object::_id);
        TestEnv::testUser=usr2->fieldValue(HATN_DB_NAMESPACE::object::_id);
    }

    auto login=loginOid.toString();
    auto loginTopic=config.topic1;
    TestEnv::testLogin=loginOid;
    if (mode==TestMode::UknownLogin)
    {
        login=HATN_DATAUNIT_NAMESPACE::ObjectId::generateIdStr();
    }
    else if (mode==TestMode::InvalidSharedSecret)
    {
        sharedSecret=config.clientSharedSecret2;
    }

    auto addUser1Topic1=[&config,usr1](auto&& next, auto ctx)
    {
        auto cb=[next=std::move(next),usr1](SharedPtr<ContextTraits::Context> ctx, const Error& ec, const du::ObjectId& oid) mutable
        {
            HATN_TEST_EC(ec)
            BOOST_REQUIRE(!ec);
            BOOST_REQUIRE(oid==usr1->fieldValue(HATN_DB_NAMESPACE::object::_id));

            next(std::move(ctx));
        };

        ContextTraits::userController(ctx).addUser(
            std::move(ctx),
            cb,
            std::move(usr1),
            config.topic1
        );
    };

    auto addLogin1_1Topic1=[&config,login1_1](auto&& next, auto ctx)
    {
        auto cb=[login1_1,next=std::move(next)](SharedPtr<ContextTraits::Context> ctx, const Error& ec, const du::ObjectId& oid) mutable
        {
            HATN_TEST_EC(ec)
            BOOST_REQUIRE(!ec);
            BOOST_REQUIRE(oid==login1_1->fieldValue(HATN_DB_NAMESPACE::object::_id));
            next(std::move(ctx));
        };

        ContextTraits::userController(ctx).addLogin(
            std::move(ctx),
            cb,
            std::move(login1_1),
            config.topic1
        );
    };

    auto addLogin1_2Topic1=[&config,login1_2](auto&& next, auto ctx)
    {
        auto cb=[login1_2,next=std::move(next)](SharedPtr<ContextTraits::Context> ctx, const Error& ec, const du::ObjectId& oid) mutable
        {
            HATN_TEST_EC(ec)
            BOOST_REQUIRE(!ec);
            BOOST_REQUIRE(oid==login1_2->fieldValue(HATN_DB_NAMESPACE::object::_id));
            next(std::move(ctx));
        };

        ContextTraits::userController(ctx).addLogin(
            std::move(ctx),
            cb,
            std::move(login1_2),
            config.topic1
            );
    };

    auto addLogin3_0Topic1=[&config,login3_0](auto&& next, auto ctx)
    {
        auto cb=[login3_0,next=std::move(next)](SharedPtr<ContextTraits::Context> ctx, const Error& ec, const du::ObjectId& oid) mutable
        {
            HATN_TEST_EC(ec)
            BOOST_REQUIRE(!ec);
            BOOST_REQUIRE(oid==login3_0->fieldValue(HATN_DB_NAMESPACE::object::_id));
            next(std::move(ctx));
        };

        ContextTraits::userController(ctx).addLogin(
            std::move(ctx),
            cb,
            std::move(login3_0),
            config.topic1
        );
    };

    auto addUser2Topic1=[&config,usr2](auto&& next, auto ctx)
    {
        auto cb=[next=std::move(next),usr2](SharedPtr<ContextTraits::Context> ctx, const Error& ec, const du::ObjectId& oid) mutable
        {
            HATN_TEST_EC(ec)
            BOOST_REQUIRE(!ec);
            BOOST_REQUIRE(oid==usr2->fieldValue(HATN_DB_NAMESPACE::object::_id));

            next(std::move(ctx));
        };

        ContextTraits::userController(ctx).addUser(
            std::move(ctx),
            cb,
            std::move(usr2),
            config.topic1
        );
    };

    auto addUser4Topic1=[&config,usr4](auto&& next, auto ctx)
    {
        auto cb=[next=std::move(next),usr4](SharedPtr<ContextTraits::Context> ctx, const Error& ec, const du::ObjectId& oid) mutable
        {
            HATN_TEST_EC(ec)
            BOOST_REQUIRE(!ec);
            BOOST_REQUIRE(oid==usr4->fieldValue(HATN_DB_NAMESPACE::object::_id));

            next(std::move(ctx));
        };

        ContextTraits::userController(ctx).addUser(
            std::move(ctx),
            cb,
            std::move(usr4),
            config.topic1
        );
    };

    auto addLogin2_1Topic1=[&config,login2_1](auto&& next, auto ctx)
    {
        auto cb=[login2_1,next=std::move(next)](SharedPtr<ContextTraits::Context> ctx, const Error& ec, const du::ObjectId& oid) mutable
        {
            HATN_TEST_EC(ec)
            BOOST_REQUIRE(!ec);
            BOOST_REQUIRE(oid==login2_1->fieldValue(HATN_DB_NAMESPACE::object::_id));
            next(std::move(ctx));
        };

        ContextTraits::userController(ctx).addLogin(
            std::move(ctx),
            cb,
            std::move(login2_1),
            config.topic1
            );
    };

    auto addLogin2_2Topic1=[&config,login2_2](auto&& next, auto ctx)
    {
        auto cb=[login2_2,next=std::move(next)](SharedPtr<ContextTraits::Context> ctx, const Error& ec, const du::ObjectId& oid) mutable
        {
            HATN_TEST_EC(ec)
            BOOST_REQUIRE(!ec);
            BOOST_REQUIRE(oid==login2_2->fieldValue(HATN_DB_NAMESPACE::object::_id));
            next(std::move(ctx));
        };

        ContextTraits::userController(ctx).addLogin(
            std::move(ctx),
            cb,
            std::move(login2_2),
            config.topic1
            );
    };

    auto addLogin2_3Topic1=[&config,login2_3](auto&& next, auto ctx)
    {
        auto cb=[login2_3,next=std::move(next)](SharedPtr<ContextTraits::Context> ctx, const Error& ec, const du::ObjectId& oid) mutable
        {
            HATN_TEST_EC(ec)
            BOOST_REQUIRE(!ec);
            BOOST_REQUIRE(oid==login2_3->fieldValue(HATN_DB_NAMESPACE::object::_id));
            next(std::move(ctx));
        };

        ContextTraits::userController(ctx).addLogin(
            std::move(ctx),
            cb,
            std::move(login2_3),
            config.topic1
            );
    };

    auto addLogin4_1Topic1=[&config,login4_1](auto&& next, auto ctx)
    {
        auto cb=[login4_1,next=std::move(next)](SharedPtr<ContextTraits::Context> ctx, const Error& ec, const du::ObjectId& oid) mutable
        {
            HATN_TEST_EC(ec)
            BOOST_REQUIRE(!ec);
            BOOST_REQUIRE(oid==login4_1->fieldValue(HATN_DB_NAMESPACE::object::_id));
            next(std::move(ctx));
        };

        ContextTraits::userController(ctx).addLogin(
            std::move(ctx),
            cb,
            std::move(login4_1),
            config.topic1
        );
    };

    auto clientApp=createClientApp(config.clientConfigFile);
    if (mode==TestMode::SessionTokenInvalid || mode==TestMode::AuthMissing)
    {
        login="19808c490dc00000154b6311a";
    }
    auto client=createClient(sharedSecret,clientApp,login,loginTopic);
    auto clientCtx=client.second;
    if (mode==TestMode::SessionTokenInvalid || mode==TestMode::SessionTokenInvalidOk)
    {
        ByteArray token;
        auto ec=token.loadFromFile(MultiThreadFixture::assetsFilePath("clientservertests","session-token.dat"));
        HATN_TEST_EC(ec)
        BOOST_REQUIRE(!ec);

        auto& cl=clientCtx->get<PlainTcpClientWithAuth>();
        auto session=cl.session();
        ec=session->sessionImpl().loadSessionToken(token.stringView());
        HATN_TEST_EC(ec)
        BOOST_REQUIRE(!ec);
    }
    auto clientThread=client.first->appThread();
    auto execClient=[clientThread,clientCtx,callback,mode](auto&& next, auto)
    {
        auto invokeTask1=[clientCtx,callback,mode,next=std::move(next)]()
        {
            auto& cl=clientCtx->get<PlainTcpClientWithAuth>();

            auto invokeTask2=[&cl,clientCtx,callback,next=std::move(next)](auto ctx, Error ec, auto resp)
            {
                if (ec)
                {
                    auto msg=ec.message();
                    if (ec.apiError()!=nullptr)
                    {
                        msg+=ec.apiError()->message();
                    }
                    HATN_TEST_MESSAGE_TS(fmt::format("invokeTask1 completed with error, ec: {}/{}",ec.codeString(),msg));
                    callback(std::move(ctx),std::move(ec),std::move(resp));
                    return;
                }

                HATN_TEST_MESSAGE_TS("invokeTask1 sucessfully completed, running invokeTask2");

                auto cb=[next=std::move(next),callback](auto ctx, const Error& ec, auto resp) mutable
                {
                    if (ec)
                    {
                        auto msg=ec.message();
                        if (ec.apiError()!=nullptr)
                        {
                            msg+=ec.apiError()->message();
                        }
                        HATN_TEST_MESSAGE_TS(fmt::format("invokeTask2 completed with error, ec: {}/{}",ec.codeString(),msg));
                        callback(std::move(ctx),std::move(ec),std::move(resp));
                        return;
                    }

                    next(std::move(ctx));
                };

                service1_msg1::type msg;
                msg.setFieldValue(service1_msg1::field1,200);
                msg.setFieldValue(service1_msg1::field2,"third run");
                Message msgData;
                ec=msgData.setContent(msg);
                HATN_TEST_EC(ec);
                BOOST_REQUIRE(!ec);
                ec=cl.exec(
                    ctx,
                    cb,
                    Service{"service1"},
                    Method{"service1_method1"},
                    std::move(msgData),
                    "test_topic"
                );
                HATN_TEST_EC(ec);
                BOOST_REQUIRE(!ec);
            };

            auto ctx1=makeLogCtx();
            service1_msg1::type msg1;
            msg1.setFieldValue(service1_msg1::field1,150);
            msg1.setFieldValue(service1_msg1::field2,"first run");
            Message msgData1;
            auto ec=msgData1.setContent(msg1);
            HATN_TEST_EC(ec);
            BOOST_REQUIRE(!ec);

            if (mode!=TestMode::AuthMissing)
            {
                ec=cl.exec(
                    ctx1,
                    invokeTask2,
                    Service{"service1"},
                    Method{"service1_method1"},
                    std::move(msgData1),
                    "test_topic"
                    );
            }
            else
            {
                ec=cl.execNoAuth(
                    ctx1,
                    invokeTask2,
                    Service{"service1"},
                    Method{"service1_method1"},
                    std::move(msgData1),
                    "test_topic"
                );
            }
            HATN_TEST_EC(ec);
            BOOST_REQUIRE(!ec);

            if (mode!=TestMode::SessionTokenExpired && mode!=TestMode::SessionExpired && mode!=TestMode::SessionTokenInvalid && mode!=TestMode::AuthMissing)
            {
                auto ctx2=makeLogCtx();
                service1_msg1::type msg2;
                msg2.setFieldValue(service1_msg1::field1,330);
                msg2.setFieldValue(service1_msg1::field2,"second run");
                Message msgData2;
                ec=msgData2.setContent(msg2);
                HATN_TEST_EC(ec);
                BOOST_REQUIRE(!ec);
                ec=cl.exec(
                    ctx2,
                    callback,
                    Service{"service1"},
                    Method{"service1_method1"},
                    std::move(msgData2),
                    "test_topic"
                    );
                HATN_TEST_EC(ec);
                BOOST_REQUIRE(!ec);
            }
        };
        clientThread->execAsync(invokeTask1);
    };

    auto client2=createClient(sharedSecret,clientApp);
    auto clientCtx2=client2.second;
    auto clientThread2=client2.first->appThread();
    if (mode==TestMode::SessionTokenExpiredReloginOk || mode==TestMode::SessionExpiredReloginOk)
    {
        auto& client=clientCtx2->get<PlainTcpClientWithAuth>();
        auto session=client.session();
        session->sessionImpl().setLogin(login,loginTopic);
    }
    else if (mode==TestMode::SessionExpired)
    {
        auto& client=clientCtx2->get<PlainTcpClientWithAuth>();
        auto session=client.session();
        session->sessionImpl().setLogin(login,config.topic2);
    }
    auto execClient2=[clientThread2,clientCtx2,callback,mode](auto)
    {
        if (mode!=TestMode::SessionTokenOk
            && mode!=TestMode::SessionTokenExpired
            && mode!=TestMode::SessionTokenExpiredReloginOk
            && mode!=TestMode::SessionExpired
            && mode!=TestMode::SessionExpiredReloginOk
            )
        {
            return;
        }

        if (mode==TestMode::SessionTokenExpired || mode==TestMode::SessionTokenExpiredReloginOk)
        {
            HATN_TEST_MESSAGE_TS("waiting for 5 seconds for session token expiration...")
            std::this_thread::sleep_for(5000ms);
        }

        if (mode==TestMode::SessionExpired || mode==TestMode::SessionExpiredReloginOk)
        {
            HATN_TEST_MESSAGE_TS("waiting for 5 seconds for session expiration...")
            std::this_thread::sleep_for(5000ms);
        }

        BOOST_REQUIRE(TestEnv::sessionToken);
        auto& cl=clientCtx2->get<PlainTcpClientWithAuth>();
        auto session=cl.session();
        auto ec=session->sessionImpl().loadSessionToken(TestEnv::sessionToken->stringView());
        HATN_TEST_EC(ec)
        BOOST_REQUIRE(!ec);

        auto invokeTask=[clientCtx2,callback]()
        {
            auto& cl=clientCtx2->get<PlainTcpClientWithAuth>();

            auto ctx1=makeLogCtx();
            service1_msg1::type msg1;
            msg1.setFieldValue(service1_msg1::field1,550);
            msg1.setFieldValue(service1_msg1::field2,"run with session token");
            Message msgData1;
            auto ec=msgData1.setContent(msg1);
            HATN_TEST_EC(ec);
            BOOST_REQUIRE(!ec);
            ec=cl.exec(
                ctx1,
                callback,
                Service{"service1"},
                Method{"service1_method1"},
                std::move(msgData1),
                "test_topic"
                );
            HATN_TEST_EC(ec);
            BOOST_REQUIRE(!ec);
        };
        clientThread2->execAsync(invokeTask);
    };

    testEnv->createThreads(1);
    auto testThread=testEnv->thread(0);
    testThread->start();
    clientThread->start();

    testThread->execAsync(
        [
         addUser1Topic1=std::move(addUser1Topic1),
         addLogin1_1Topic1=std::move(addLogin1_1Topic1),
         addLogin1_2Topic1=std::move(addLogin1_2Topic1),
         addUser2Topic1=std::move(addUser2Topic1),
         addLogin2_1Topic1=std::move(addLogin2_1Topic1),
         addLogin2_2Topic1=std::move(addLogin2_2Topic1),
         addLogin2_3Topic1=std::move(addLogin2_3Topic1),
         addLogin3_0Topic1=std::move(addLogin3_0Topic1),
         addUser4Topic1=std::move(addUser4Topic1),
         addLogin4_1Topic1=std::move(addLogin4_1Topic1),
         execClient=std::move(execClient),
         execClient2=std::move(execClient2)
        ]()
        {
            auto reqCtx=server::allocateAndInitRequestContext<RequestWithAuth>(EnvWithAuthConfigTraits::env);
            auto chain=hatn::chain(
                std::move(addUser1Topic1),
                std::move(addLogin1_1Topic1),
                std::move(addLogin1_2Topic1),
                std::move(addUser2Topic1),
                std::move(addLogin2_1Topic1),
                std::move(addLogin2_2Topic1),
                std::move(addLogin2_3Topic1),
                std::move(addLogin3_0Topic1),
                std::move(addUser4Topic1),
                std::move(addLogin4_1Topic1),
                std::move(execClient),
                std::move(execClient2)
            );
            chain(std::move(reqCtx));
        }
    );

    BOOST_TEST_MESSAGE(fmt::format("Running test for {} seconds",config.runningSecs));
    testEnv->exec(config.runningSecs);

    for (auto&& it: serverCtx->microservices)
    {
        it.second->close();
    }
    testEnv->exec(1);

    serverCtx->app->close();
    clientApp->close();

    testEnv->exec(1);
    testThread->stop();
    testEnv->exec(1);
}

BOOST_AUTO_TEST_SUITE(TestHssAuth)

BOOST_FIXTURE_TEST_CASE(EnvFB,TestEnv)
{
    EnvFooBar env;
    auto& fb2=env.get<FooBar2>();
    fb2.env=&env;
    auto ret=fb2.value();
    BOOST_CHECK_EQUAL(ret,0);
}

BOOST_FIXTURE_TEST_CASE(CreateServer,TestEnv)
{
    runCreateServer("hssauthserver.jsonc",this,0);
}

BOOST_FIXTURE_TEST_CASE(CreateClient,TestEnv)
{
    std::string sharedSecret{"shared_secret1"};
    auto app=createClientApp("hssauthclient.jsonc");
    auto clientCtx=createClient(sharedSecret,app,"hssauthclient.jsonc");

    int secs=3;
    BOOST_TEST_MESSAGE(fmt::format("Running test for {} seconds",secs));
    exec(secs);

    app->close();

    exec(1);
}

BOOST_FIXTURE_TEST_CASE(UnknownLogin,TestEnv)
{
    auto cb=[](auto ctx, const Error& ec, auto response)
    {
        if (ec)
        {
            auto msg=ec.message();
            if (ec.apiError()!=nullptr)
            {
                msg+=ec.apiError()->message();
            }
            HATN_TEST_MESSAGE_TS(fmt::format("exec cb, ec: {}/{}",ec.codeString(),msg));
            BOOST_CHECK(ec.is(ApiLibError::SERVER_RESPONDED_WITH_ERROR,ApiLibErrorCategory::getCategory()));
            BOOST_REQUIRE(ec.apiError()!=nullptr);
            BOOST_CHECK(ec.apiError()->is(ApiAuthError::AUTH_FAILED,ApiAuthErrorCategory::getCategory()));

            //! @todo check journal for login try
        }
        else
        {
            BOOST_FAIL("test must fail");
        }
    };

    runTest(this,cb,TestMode::UknownLogin);
    BOOST_CHECK_EQUAL(TestEnv::testAmount,0);
}

BOOST_FIXTURE_TEST_CASE(Login1Ok,TestEnv)
{
    auto cb=[](auto ctx, const Error& ec, auto response)
    {
        if (ec)
        {
            auto msg=ec.message();
            if (ec.apiError()!=nullptr)
            {
                msg+=ec.apiError()->message();
            }
            HATN_TEST_MESSAGE_TS(fmt::format("exec cb, ec: {}/{}",ec.codeString(),msg));
        }
        else
        {
            HATN_TEST_MESSAGE_TS("exec completed");
        }
        BOOST_CHECK(!ec);
    };

    runTest(this,cb,TestMode::OK);

    BOOST_CHECK_EQUAL(TestEnv::testAmount,680);
}

BOOST_FIXTURE_TEST_CASE(Login2Ok,TestEnv)
{
    auto cb=[](auto ctx, const Error& ec, auto response)
    {
        if (ec)
        {
            auto msg=ec.message();
            if (ec.apiError()!=nullptr)
            {
                msg+=ec.apiError()->message();
            }
            HATN_TEST_MESSAGE_TS(fmt::format("exec cb, ec: {}/{}",ec.codeString(),msg));
        }
        else
        {
            HATN_TEST_MESSAGE_TS("exec completed");
        }
        BOOST_CHECK(!ec);
    };

    runTest(this,cb,TestMode::SecondLoginOk);

    BOOST_CHECK_EQUAL(TestEnv::testAmount,680);
}

BOOST_FIXTURE_TEST_CASE(User2Ok,TestEnv)
{
    auto cb=[](auto ctx, const Error& ec, auto response)
    {
        if (ec)
        {
            auto msg=ec.message();
            if (ec.apiError()!=nullptr)
            {
                msg+=ec.apiError()->message();
            }
            HATN_TEST_MESSAGE_TS(fmt::format("exec cb, ec: {}/{}",ec.codeString(),msg));
        }
        else
        {
            HATN_TEST_MESSAGE_TS("exec completed");
        }
        BOOST_CHECK(!ec);
    };

    runTest(this,cb,TestMode::OtherUserOk);

    BOOST_CHECK_EQUAL(TestEnv::testAmount,680);
}

BOOST_FIXTURE_TEST_CASE(InvalidSharedSecret,TestEnv)
{
    auto cb=[](auto ctx, const Error& ec, auto response)
    {
        if (ec)
        {
            auto msg=ec.message();
            if (ec.apiError()!=nullptr)
            {
                msg+=ec.apiError()->message();
            }
            HATN_TEST_MESSAGE_TS(fmt::format("exec cb, ec: {}/{}",ec.codeString(),msg));
            BOOST_CHECK(ec.is(ApiLibError::SERVER_RESPONDED_WITH_ERROR,ApiLibErrorCategory::getCategory()));
            BOOST_REQUIRE(ec.apiError()!=nullptr);
            BOOST_CHECK(ec.apiError()->is(ApiAuthError::AUTH_FAILED,ApiAuthErrorCategory::getCategory()));

            //! @todo check journal for login try
        }
        else
        {
            BOOST_FAIL("test must fail");
        }
    };

    runTest(this,cb,TestMode::InvalidSharedSecret);
    BOOST_CHECK_EQUAL(TestEnv::testAmount,0);
}

BOOST_FIXTURE_TEST_CASE(UnknownUser,TestEnv)
{
    auto cb=[](auto ctx, const Error& ec, auto response)
    {
        if (ec)
        {
            auto msg=ec.message();
            if (ec.apiError()!=nullptr)
            {
                msg+=ec.apiError()->message();
            }
            HATN_TEST_MESSAGE_TS(fmt::format("exec cb, ec: {}/{}",ec.codeString(),msg));
            BOOST_CHECK(ec.is(ApiLibError::SERVER_RESPONDED_WITH_ERROR,ApiLibErrorCategory::getCategory()));
            BOOST_REQUIRE(ec.apiError()!=nullptr);
            BOOST_CHECK(ec.apiError()->is(ApiAuthError::ACCESS_DENIED,ApiAuthErrorCategory::getCategory()));

            //! @todo check journal for login try
        }
        else
        {
            BOOST_FAIL("test must fail");
        }
    };

    runTest(this,cb,TestMode::UknownUser);
    BOOST_CHECK_EQUAL(TestEnv::testAmount,0);
}

BOOST_FIXTURE_TEST_CASE(LoginBlocked,TestEnv)
{
    auto cb=[](auto ctx, const Error& ec, auto response)
    {
        if (ec)
        {
            auto msg=ec.message();
            if (ec.apiError()!=nullptr)
            {
                msg+=ec.apiError()->message();
            }
            HATN_TEST_MESSAGE_TS(fmt::format("exec cb, ec: {}/{}",ec.codeString(),msg));
            BOOST_CHECK(ec.is(ApiLibError::SERVER_RESPONDED_WITH_ERROR,ApiLibErrorCategory::getCategory()));
            BOOST_REQUIRE(ec.apiError()!=nullptr);
            BOOST_CHECK(ec.apiError()->is(ApiAuthError::ACCESS_DENIED,ApiAuthErrorCategory::getCategory()));

            //! @todo check journal for login try
        }
        else
        {
            BOOST_FAIL("test must fail");
        }
    };

    runTest(this,cb,TestMode::LoginBlocked);
    BOOST_CHECK_EQUAL(TestEnv::testAmount,0);
}

BOOST_FIXTURE_TEST_CASE(LoginExpired,TestEnv)
{
    auto cb=[](auto ctx, const Error& ec, auto response)
    {
        if (ec)
        {
            auto msg=ec.message();
            if (ec.apiError()!=nullptr)
            {
                msg+=ec.apiError()->message();
            }
            HATN_TEST_MESSAGE_TS(fmt::format("exec cb, ec: {}/{}",ec.codeString(),msg));
            BOOST_CHECK(ec.is(ApiLibError::SERVER_RESPONDED_WITH_ERROR,ApiLibErrorCategory::getCategory()));
            BOOST_REQUIRE(ec.apiError()!=nullptr);
            BOOST_CHECK(ec.apiError()->is(ApiAuthError::AUTH_FAILED,ApiAuthErrorCategory::getCategory()));

            //! @todo check journal for login try
        }
        else
        {
            BOOST_FAIL("test must fail");
        }
    };

    runTest(this,cb,TestMode::LoginExpired);
    BOOST_CHECK_EQUAL(TestEnv::testAmount,0);
}

BOOST_FIXTURE_TEST_CASE(UserBlocked,TestEnv)
{
    auto cb=[](auto ctx, const Error& ec, auto response)
    {
        if (ec)
        {
            auto msg=ec.message();
            if (ec.apiError()!=nullptr)
            {
                msg+=ec.apiError()->message();
            }
            HATN_TEST_MESSAGE_TS(fmt::format("exec cb, ec: {}/{}",ec.codeString(),msg));
            BOOST_CHECK(ec.is(ApiLibError::SERVER_RESPONDED_WITH_ERROR,ApiLibErrorCategory::getCategory()));
            BOOST_REQUIRE(ec.apiError()!=nullptr);
            BOOST_CHECK(ec.apiError()->is(ApiAuthError::ACCESS_DENIED,ApiAuthErrorCategory::getCategory()));

            //! @todo check journal for login try
        }
        else
        {
            BOOST_FAIL("test must fail");
        }
    };

    runTest(this,cb,TestMode::UserBlocked);
    BOOST_CHECK_EQUAL(TestEnv::testAmount,0);
}

BOOST_FIXTURE_TEST_CASE(SessionTokenOk,TestEnv)
{
    auto cb=[](auto ctx, const Error& ec, auto response)
    {
        if (ec)
        {
            auto msg=ec.message();
            if (ec.apiError()!=nullptr)
            {
                msg+=ec.apiError()->message();
            }
            HATN_TEST_MESSAGE_TS(fmt::format("exec cb, ec: {}/{}",ec.codeString(),msg));
        }
        else
        {
            HATN_TEST_MESSAGE_TS("exec completed");
        }
        BOOST_CHECK(!ec);
    };

    runTest(this,cb,TestMode::SessionTokenOk);

    BOOST_CHECK_EQUAL(TestEnv::testAmount,1230);
}

BOOST_FIXTURE_TEST_CASE(SessionTokenExpired,TestEnv)
{
    auto cb=[](auto ctx, const Error& ec, auto response)
    {
        if (ec)
        {
            auto msg=ec.message();
            HATN_TEST_MESSAGE_TS(fmt::format("exec cb, ec: {}/{}",ec.codeString(),msg));
            BOOST_CHECK(ec.is(ClientServerError::LOGIN_NOT_SET,ClientServerErrorCategory::getCategory()));

            //! @todo check journal for login try
        }
        else
        {
            BOOST_FAIL("test must fail");
        }
    };

    TestConfig config;
    config.serverConfigFile="hssauthserver-sess-token-exp.jsonc";
    config.clientSharedSecret1="shared_secret11";
    config.clientSharedSecret2="shared_secret22";
    config.runningSecs=10;
    runTest(this,cb,TestMode::SessionTokenExpired,config);
    BOOST_CHECK_EQUAL(TestEnv::testAmount,350);
}

BOOST_FIXTURE_TEST_CASE(SessionTokenExpiredReloginOk,TestEnv)
{
    auto cb=[](auto ctx, const Error& ec, auto response)
    {
        if (ec)
        {
            auto msg=ec.message();
            if (ec.apiError()!=nullptr)
            {
                msg+=ec.apiError()->message();
            }
            HATN_TEST_MESSAGE_TS(fmt::format("exec cb, ec: {}/{}",ec.codeString(),msg));
        }
        else
        {
            HATN_TEST_MESSAGE_TS("exec completed");
        }
        BOOST_CHECK(!ec);
    };

    TestConfig config;
    config.serverConfigFile="hssauthserver-sess-token-exp.jsonc";
    config.clientSharedSecret1="shared_secret11";
    config.clientSharedSecret2="shared_secret22";
    config.runningSecs=10;
    runTest(this,cb,TestMode::SessionTokenExpiredReloginOk,config);
    BOOST_CHECK_EQUAL(TestEnv::testAmount,1230);
}

BOOST_FIXTURE_TEST_CASE(SessionExpired,TestEnv)
{
    auto cb=[](auto ctx, const Error& ec, auto response)
    {
        if (ec)
        {
            auto msg=ec.message();
            if (ec.apiError()!=nullptr)
            {
                msg+=ec.apiError()->message();
            }
            HATN_TEST_MESSAGE_TS(fmt::format("exec cb, ec: {}/{}",ec.codeString(),msg));
            BOOST_CHECK(ec.is(ApiLibError::SERVER_RESPONDED_WITH_ERROR,ApiLibErrorCategory::getCategory()));
            BOOST_REQUIRE(ec.apiError()!=nullptr);
            BOOST_CHECK(ec.apiError()->is(ApiAuthError::AUTH_FAILED,ApiAuthErrorCategory::getCategory()));

            //! @todo check journal for login try
        }
        else
        {
            BOOST_FAIL("test must fail");
        }
    };

    TestConfig config;
    config.serverConfigFile="hssauthserver-sess-exp.jsonc";
    config.clientSharedSecret1="shared_secret11";
    config.clientSharedSecret2="shared_secret22";
    config.runningSecs=10;
    runTest(this,cb,TestMode::SessionExpired,config);
    BOOST_CHECK_EQUAL(TestEnv::testAmount,350);
}

BOOST_FIXTURE_TEST_CASE(SessionExpiredReloginOk,TestEnv)
{
    auto cb=[](auto ctx, const Error& ec, auto response)
    {
        if (ec)
        {
            auto msg=ec.message();
            if (ec.apiError()!=nullptr)
            {
                msg+=ec.apiError()->message();
            }
            HATN_TEST_MESSAGE_TS(fmt::format("exec cb, ec: {}/{}",ec.codeString(),msg));
        }
        else
        {
            HATN_TEST_MESSAGE_TS("exec completed");
        }
        BOOST_CHECK(!ec);
    };

    TestConfig config;
    config.serverConfigFile="hssauthserver-sess-exp.jsonc";
    config.clientSharedSecret1="shared_secret11";
    config.clientSharedSecret2="shared_secret22";
    config.runningSecs=10;
    runTest(this,cb,TestMode::SessionExpiredReloginOk,config);
    BOOST_CHECK_EQUAL(TestEnv::testAmount,1230);
}

BOOST_FIXTURE_TEST_CASE(SessionTokenInvalid,TestEnv)
{
    auto cb=[](auto ctx, const Error& ec, auto response)
    {
        if (ec)
        {
            auto msg=ec.message();
            if (ec.apiError()!=nullptr)
            {
                msg+=ec.apiError()->message();
            }
            HATN_TEST_MESSAGE_TS(fmt::format("exec cb, ec: {}/{}",ec.codeString(),msg));
            BOOST_CHECK(ec.is(ApiLibError::SERVER_RESPONDED_WITH_ERROR,ApiLibErrorCategory::getCategory()));
            BOOST_REQUIRE(ec.apiError()!=nullptr);
            BOOST_CHECK(ec.apiError()->is(ApiAuthError::AUTH_FAILED,ApiAuthErrorCategory::getCategory()));

            //! @todo check journal for login try
        }
        else
        {
            BOOST_FAIL("test must fail");
        }
    };

    runTest(this,cb,TestMode::SessionTokenInvalid);
    BOOST_CHECK_EQUAL(TestEnv::testAmount,0);
}

BOOST_FIXTURE_TEST_CASE(SessionTokenInvalidOk,TestEnv)
{
    auto cb=[](auto ctx, const Error& ec, auto response)
    {
        if (ec)
        {
            auto msg=ec.message();
            if (ec.apiError()!=nullptr)
            {
                msg+=ec.apiError()->message();
            }
            HATN_TEST_MESSAGE_TS(fmt::format("exec cb, ec: {}/{}",ec.codeString(),msg));
        }
        else
        {
            HATN_TEST_MESSAGE_TS("exec completed");
        }
        BOOST_CHECK(!ec);
    };

    TestConfig config;
    runTest(this,cb,TestMode::SessionTokenInvalidOk,config);

    BOOST_CHECK_EQUAL(TestEnv::testAmount,680);
}

BOOST_FIXTURE_TEST_CASE(AuthMissing,TestEnv)
{
    auto cb=[](auto ctx, const Error& ec, auto response)
    {
        if (ec)
        {
            auto msg=ec.message();
            if (ec.apiError()!=nullptr)
            {
                msg+=ec.apiError()->message();
            }
            HATN_TEST_MESSAGE_TS(fmt::format("exec cb, ec: {}/{}",ec.codeString(),msg));
            BOOST_CHECK(ec.is(ApiLibError::SERVER_RESPONDED_WITH_ERROR,ApiLibErrorCategory::getCategory()));
            BOOST_REQUIRE(ec.apiError()!=nullptr);
            BOOST_CHECK(ec.apiError()->is(ApiAuthError::AUTH_MISSING,ApiAuthErrorCategory::getCategory()));

            //! @todo check journal for login try
        }
        else
        {
            BOOST_FAIL("test must fail");
        }
    };

    runTest(this,cb,TestMode::AuthMissing);
    BOOST_CHECK_EQUAL(TestEnv::testAmount,0);
}

BOOST_AUTO_TEST_SUITE_END()

/**
 * @todo Test auth:
 *
 * 1. Invalid challenge token.
 * 2. Expired challenge token.
 * 3. Missing challenge token.
 * 4. Session tags.
 * 5. Inactive session.
 * 6. Client auth API logs.
 */
