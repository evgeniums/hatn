/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>

#include <hatn_test_config.h>

#include <hatn/common/format.h>

#include <hatn/crypt/cryptplugin.h>
#include <hatn/crypt/ciphersuite.h>
#include <hatn/crypt/cryptcontainer.h>

#include <hatn/clientserver/clientservererror.h>
#include <hatn/clientserver/accountconfigparser.h>

#include <hatn/test/multithreadfixture.h>

#include <initcryptplugin.h>

#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>

#define HATN_SAVE_TEST_FILES

namespace hatn {

namespace test {

HATN_COMMON_USING
HATN_CRYPT_USING
HATN_CLIENT_SERVER_USING

auto prepareCipherSuite(std::shared_ptr<CryptPlugin>& plugin, const std::string& path)
{
    std::string cipherSuiteFile=fmt::format("{}/ciphersuite1.json",path);

    auto engine=std::make_shared<CryptEngine>(plugin.get());
    auto suites=std::make_shared<CipherSuites>();
    suites->setDefaultEngine(engine);

    auto suite=std::make_shared<CipherSuite>();
    auto ec=suite->loadFromFile(cipherSuiteFile);
    HATN_TEST_EC(ec)
    BOOST_REQUIRE(!ec);
    suites->addSuite(suite);
    suites->setDefaultSuite(suite);
    return suites;
}

struct TestConfig
{
    bool encrypted=false;
    bool expired=false;
    bool mailformed=false;
    bool missingPassphrase=false;
    bool invalidPassphrase=false;
    std::string passphrase;
};

BOOST_FIXTURE_TEST_SUITE(TestAccountConfig,CryptTestFixture)

static void checkAccountConfig(std::shared_ptr<CryptPlugin>& plugin, const std::string& path, const TestConfig& cfg)
{
    auto suites=prepareCipherSuite(plugin,path);

    auto validTill1=DateTime::currentUtc();
    if (cfg.expired)
    {
        validTill1.addDays(-1);
    }
    else
    {
        validTill1.addDays(1);
    }

    account_config::type cfg1;
    cfg1.setFieldValue(account_config::valid_till,validTill1);
    cfg1.setFieldValue(account_config::user_name,"user1");
    cfg1.setFieldValue(account_config::server_name,"server1");
    cfg1.setFieldValue(account_config::user_token,"AAAA2222444");
    BOOST_TEST_MESSAGE("cfg1 before parsing");
    auto cfg1Str=cfg1.toString(true);
    BOOST_TEST_MESSAGE(cfg1Str);
    du::WireBufSolid cfgBuf11;
    auto ret=du::io::serialize(cfg1,cfgBuf11);
    BOOST_REQUIRE_GT(ret,0);

    lib::string_view content=cfgBuf11.mainContainer()->stringView();
    ByteArray encryptedData;
    if (cfg.encrypted)
    {
        CryptContainer cryptContainer{suites->defaultSuite()};
        cryptContainer.setCipherSuites(suites.get());
        cryptContainer.setPassphrase(cfg.passphrase);

        auto ec=cryptContainer.pack(cfgBuf11.mainContainer()->stringView(),encryptedData);
        HATN_TEST_EC(ec)
        BOOST_REQUIRE(!ec);
        content=encryptedData.stringView();
    }
    if (cfg.mailformed)
    {
        content=cfg1Str;
    }

    account_config_token::type cfgToken1;
    cfgToken1.setFieldValue(account_config_token::content,content);
    cfgToken1.setFieldValue(account_config_token::encrypted,cfg.encrypted);
    auto cfgToken1Str=cfgToken1.toString(true);
    BOOST_TEST_MESSAGE(cfgToken1Str);
    du::WireBufSolid cfgTokenBuf11;
    ret=du::io::serialize(cfgToken1,cfgTokenBuf11);
    BOOST_REQUIRE_GT(ret,0);

#ifdef HATN_SAVE_TEST_FILES

    if (!cfg.mailformed && !cfg.invalidPassphrase && !cfg.missingPassphrase)
    {
        std::string fileName;

        if (cfg.encrypted)
        {
            if (cfg.expired)
            {
                fileName=fmt::format("{}/account-config-encrypted-expired.dat",path);
            }
            else
            {
                fileName=fmt::format("{}/account-config-encrypted.dat",path);
            }
        }
        else
        {
            if (cfg.expired)
            {
                fileName=fmt::format("{}/account-config-expired.dat",path);
            }
            else
            {
                fileName=fmt::format("{}/account-config.dat",path);
            }
        }

        auto ec=cfgTokenBuf11.mainContainer()->saveToFile(fileName);
        HATN_REQUIRE(!ec);
    }

#endif

    std::string decryptPassphrase;
    if (cfg.invalidPassphrase)
    {
        decryptPassphrase="12345678";
    }
    else if (cfg.encrypted && !cfg.missingPassphrase)
    {
        decryptPassphrase=cfg.passphrase;
    }

    auto r=parseAccountConfig(suites.get(),cfgTokenBuf11.mainContainer()->stringView(),decryptPassphrase);
    if (cfg.encrypted)
    {
        if (cfg.missingPassphrase)
        {
            BOOST_REQUIRE(r);
            BOOST_CHECK(r.error().is(ClientServerError::ACCOUNT_CONFIG_PASSPHRASE_REQUIRED,ClientServerErrorCategory::getCategory()));
            return;
        }
        if (cfg.invalidPassphrase)
        {
            BOOST_REQUIRE(r);
            BOOST_TEST_MESSAGE(fmt::format("error: {}, code {}, category {}",r.error().codeString(),r.error().code(),r.error().category()->name()));
            BOOST_CHECK(r.error().is(ClientServerError::ACCOUNT_CONFIG_DECRYPTION,ClientServerErrorCategory::getCategory()));
            return;
        }
    }
    if (cfg.expired)
    {
        BOOST_REQUIRE(r);
        BOOST_CHECK(r.error().is(ClientServerError::ACCOUNT_CONFIG_EXPIRED,ClientServerErrorCategory::getCategory()));
        return;
    }
    if (cfg.mailformed)
    {
        BOOST_REQUIRE(r);
        BOOST_CHECK(
                r.error().is(ClientServerError::ACCOUNT_CONFIG_DESERIALIZATION,ClientServerErrorCategory::getCategory())
                ||
                r.error().is(ClientServerError::ACCOUNT_CONFIG_DATA_DESERIALIZATION,ClientServerErrorCategory::getCategory())
            );
        return;
    }

    HATN_TEST_RESULT(r)
    BOOST_REQUIRE(!r);

    auto parsedCfg1=r.takeValue();
    BOOST_REQUIRE(parsedCfg1);

    BOOST_TEST_MESSAGE("cfg1 after parsing");
    BOOST_TEST_MESSAGE(parsedCfg1->toString(true));
}

BOOST_AUTO_TEST_CASE(CheckNotEncrypted)
{
    CryptPluginTest::instance().eachPlugin<CryptTestTraits>(
        [](std::shared_ptr<CryptPlugin>& plugin)
        {
            TestConfig cfg;
            checkAccountConfig(plugin,PluginList::assetsPath("clientserver"),cfg);
        }
    );
}

BOOST_AUTO_TEST_CASE(CheckNotEncryptedExpired)
{
    CryptPluginTest::instance().eachPlugin<CryptTestTraits>(
        [](std::shared_ptr<CryptPlugin>& plugin)
        {
            TestConfig cfg;
            cfg.expired=true;
            checkAccountConfig(plugin,PluginList::assetsPath("clientserver"),cfg);
        }
    );
}

BOOST_AUTO_TEST_CASE(CheckEncrypted)
{
    CryptPluginTest::instance().eachPlugin<CryptTestTraits>(
        [](std::shared_ptr<CryptPlugin>& plugin)
        {
            TestConfig cfg;
            cfg.encrypted=true;
            cfg.passphrase="blablablabla";
            checkAccountConfig(plugin,PluginList::assetsPath("clientserver"),cfg);
        }
    );
}

BOOST_AUTO_TEST_CASE(CheckEncryptedMissingPassphrase)
{
    CryptPluginTest::instance().eachPlugin<CryptTestTraits>(
        [](std::shared_ptr<CryptPlugin>& plugin)
        {
            TestConfig cfg;
            cfg.encrypted=true;
            cfg.passphrase="blablablabla";
            cfg.missingPassphrase=true;
            checkAccountConfig(plugin,PluginList::assetsPath("clientserver"),cfg);
        }
    );
}

BOOST_AUTO_TEST_CASE(CheckEncryptedInvalidPassphrase)
{
    CryptPluginTest::instance().eachPlugin<CryptTestTraits>(
        [](std::shared_ptr<CryptPlugin>& plugin)
        {
            TestConfig cfg;
            cfg.encrypted=true;
            cfg.passphrase="blablablabla";
            cfg.invalidPassphrase=true;
            checkAccountConfig(plugin,PluginList::assetsPath("clientserver"),cfg);
        }
    );
}

BOOST_AUTO_TEST_CASE(CheckEncryptedExpired)
{
    CryptPluginTest::instance().eachPlugin<CryptTestTraits>(
        [](std::shared_ptr<CryptPlugin>& plugin)
        {
            TestConfig cfg;
            cfg.encrypted=true;
            cfg.expired=true;
            cfg.passphrase="blablablabla";
            checkAccountConfig(plugin,PluginList::assetsPath("clientserver"),cfg);
        }
    );
}

BOOST_AUTO_TEST_CASE(CheckMailformed)
{
    CryptPluginTest::instance().eachPlugin<CryptTestTraits>(
        [](std::shared_ptr<CryptPlugin>& plugin)
        {
            TestConfig cfg;
            cfg.mailformed=true;
            checkAccountConfig(plugin,PluginList::assetsPath("clientserver"),cfg);
        }
    );
}

BOOST_AUTO_TEST_SUITE_END()

}
}
