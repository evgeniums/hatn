/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include <hatn_test_config.h>

#include <hatn/common/bytearray.h>
#include <hatn/crypt/cryptplugin.h>

#include "initcryptplugin.h"

namespace hatn {

using namespace common;
using namespace crypt;

namespace test {

BOOST_FIXTURE_TEST_SUITE(TestRandomGen,CryptTestFixture)

BOOST_AUTO_TEST_CASE(RandomDataGen)
{
    CryptPluginTest::instance().eachPlugin<CryptTestTraits>(
        [](std::shared_ptr<CryptPlugin>& plugin)
        {
            if (plugin->isFeatureImplemented(Crypt::Feature::RandomGenerator))
            {
                auto gen=plugin->createRandomGenerator();

                size_t maxSize=16000;

                ByteArray arr1;
                auto ec=gen->randContainer(arr1,maxSize);
                BOOST_REQUIRE(!ec);
                BOOST_CHECK_EQUAL(arr1.size(),maxSize);

                ByteArray arr2;
                ec=gen->randContainer(arr2,maxSize);
                BOOST_REQUIRE(!ec);
                BOOST_CHECK_EQUAL(arr2.size(),maxSize);
                BOOST_CHECK(arr1!=arr2);

                size_t minSize=1000;

                ByteArray arr3;
                ec=gen->randContainer(arr3,maxSize,minSize);
                BOOST_REQUIRE(!ec);
                BOOST_CHECK((arr3.size()>=minSize && arr3.size()<=maxSize));

                ByteArray arr4;
                ec=gen->randContainer(arr4,maxSize,minSize);
                BOOST_REQUIRE(!ec);
                BOOST_CHECK((arr4.size()>=minSize && arr4.size()<=maxSize));
                BOOST_CHECK(arr3.size()!=arr4.size());
            }
        }
    );
}

BOOST_AUTO_TEST_CASE(RandomPassword)
{
    CryptPluginTest::instance().eachPlugin<CryptTestTraits>(
        [](std::shared_ptr<CryptPlugin>& plugin)
        {
            if (plugin->isFeatureImplemented(Crypt::Feature::PasswordGenerator))
            {
                auto gen=plugin->createPasswordGenerator();

                PasswordGeneratorParameters params;
                std::vector<common::MemoryLockedArray> passwords;
                for (int i=0;i<10;i++)
                {
                    common::MemoryLockedArray pwd;
                    auto ec=gen->generate(pwd,params);
                    BOOST_REQUIRE(!ec);
#if 0
                    BOOST_TEST_MESSAGE(pwd.c_str());
#endif
                    BOOST_CHECK(pwd.size()>=params.minLength && pwd.size()<=params.maxLength);
                    bool ok=true;
                    for (size_t j=0;j<passwords.size();j++)
                    {
                        ok=pwd!=passwords[j];
                        if (!ok)
                        {
                            break;
                        }
                    }
                    BOOST_CHECK(ok);
                    passwords.push_back(pwd);
                }

                auto checkOneType=[&gen](const PasswordGeneratorParameters& params,const std::string& sample)
                {
                    std::vector<common::MemoryLockedArray> passwords;
                    for (int i=0;i<10;i++)
                    {
                        common::MemoryLockedArray pwd;
                        auto ec=gen->generate(pwd,params);
                        BOOST_REQUIRE(!ec);
#if 0
                        BOOST_TEST_MESSAGE(pwd.c_str());
#endif
                        BOOST_CHECK(pwd.size()>=params.minLength && pwd.size()<=params.maxLength);
                        bool ok=true;
                        for (size_t j=0;j<passwords.size();j++)
                        {
                            ok=pwd!=passwords[j];
                            if (!ok)
                            {
                                break;
                            }
                        }
                        BOOST_CHECK(ok);

                        ok=true;
                        for (size_t i=0;i<pwd.size();i++)
                        {
                            ok=sample.find(pwd[i]) != std::string::npos;
                            if (!ok)
                            {
                                break;
                            }
                        }
                        BOOST_CHECK(ok);

                        passwords.push_back(pwd);
                    }
                };

                params.digitsWeight=1;
                params.lettersWeight=0;
                params.specialsWeight=0;
                std::string sample="0123456789";
                checkOneType(params,sample);

                params.digitsWeight=0;
                params.lettersWeight=1;
                params.specialsWeight=0;
                sample="_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
                checkOneType(params,sample);

                params.digitsWeight=0;
                params.lettersWeight=0;
                params.specialsWeight=1;
                sample="~!@#$%^&*(){}+=-:;<>,.|/?";
                checkOneType(params,sample);
            }
        }
    );
}

BOOST_AUTO_TEST_SUITE_END()

}
}
