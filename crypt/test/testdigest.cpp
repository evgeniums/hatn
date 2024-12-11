/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/algorithm/hex.hpp>

#include <hatn_test_config.h>

#include <hatn/common/bytearray.h>
#include <hatn/common/format.h>
#include <hatn/test/multithreadfixture.h>

#include <hatn/crypt/cryptplugin.h>

#include "initcryptplugin.h"

namespace hatn {

using namespace common;
using namespace crypt;

namespace test {

//#define PRINT_CRYPT_ENGINE_ALGS

BOOST_FIXTURE_TEST_SUITE(TestDigest,CryptTestFixture)

void checkDigest(std::shared_ptr<CryptPlugin>& plugin, const std::string& digestName, const std::string& fileName)
{
    static ByteArray data("Hello world from Dracosha! Let's calculate a hash on this phrase.");
    if (boost::filesystem::exists(fileName))
    {
        std::string msg=fmt::format("Checking digest {} with {}",digestName,fileName);
        BOOST_TEST_MESSAGE(msg);

        const CryptAlgorithm* alg=nullptr;
        auto ec=plugin->findAlgorithm(alg,CryptAlgorithm::Type::DIGEST,digestName);
        BOOST_CHECK(!ec);
        if (!ec)
        {
            ByteArray hash;

            ec=Digest::digest(alg,data,hash);
            HATN_REQUIRE(!ec);
            HATN_REQUIRE_EQUAL(hash.size(),alg->hashSize());

            std::string hex;
            boost::algorithm::hex(hash.stringView().begin(),hash.stringView().end(),std::back_inserter(hex));

            auto sample=PluginList::linefromFile(fileName);
            std::string check=sample.substr(0,alg->hashSize()*2);
            HATN_REQUIRE_EQUAL(hex,check);
            ByteArray checkBin;
            ContainerUtils::hexToRaw(check,checkBin);

            ec=Digest::check(alg,data,hash);
            BOOST_CHECK(!ec);

            ec=Digest::digest(alg,data,hash,12,14,alg->hashSize());
            HATN_REQUIRE(!ec);
            HATN_REQUIRE_EQUAL(hash.size(),alg->hashSize()*2);

            boost::algorithm::hex(hash.stringView(alg->hashSize()).begin(),hash.stringView(alg->hashSize()).end(),std::back_inserter(hex));
            HATN_REQUIRE_EQUAL(hex,sample);

            ec=Digest::check(alg,data,hash,12,14,alg->hashSize());
            BOOST_CHECK(!ec);

            ec=Digest::check(alg,data,hash,10,14,alg->hashSize());
            BOOST_CHECK(ec);

            ec=Digest::check(alg,data,hash,12,16,alg->hashSize());
            BOOST_CHECK(ec);

            ec=Digest::check(alg,data,hash,12,14,alg->hashSize()+2);
            BOOST_CHECK(ec);

            ec=Digest::check(alg,data,hash,12,14,alg->hashSize()-2);
            BOOST_CHECK(ec);

            ec=Digest::check(alg,ByteArray(),hash,12,14,alg->hashSize());
            BOOST_CHECK(ec);

            ec=Digest::check(alg,data,ByteArray(),12,14,alg->hashSize());
            BOOST_CHECK(ec);

            ec=Digest::check(alg,ByteArray(),hash,12,14);
            BOOST_CHECK(ec);

            ec=Digest::check(alg,data,ByteArray());
            BOOST_CHECK(ec);

            ec=Digest::digest(alg,ByteArray(),hash,12,14,alg->hashSize());
            HATN_REQUIRE(ec);

            ec=Digest::digest(alg,ByteArray(),hash,12,14);
            HATN_REQUIRE(ec);

            ec=Digest::digest(alg,data,hash,200,14,alg->hashSize());
            HATN_REQUIRE(ec);

            ec=Digest::digest(alg,data,hash,0,500,alg->hashSize());
            HATN_REQUIRE(ec);

            // check SpanBuffer interfaces
            auto digest=plugin->createDigest(alg);
            HATN_REQUIRE(digest);

            ByteArray tag1;
            ec=digest->runFinalize(SpanBuffer{data},tag1);
            BOOST_CHECK(!ec);
            ec=digest->runCheck(SpanBuffer{data},tag1);
            BOOST_CHECK(!ec);
            BOOST_CHECK(checkBin==tag1);

            ByteArray tag2("with offset");
            size_t offset=tag2.size();
            ec=digest->runFinalize(SpanBuffer{data},tag2,offset);
            BOOST_CHECK(!ec);
            ec=digest->runCheck(SpanBuffer{data},tag2.data()+offset,tag2.size()-offset);
            BOOST_CHECK(!ec);
            ec=digest->runCheck(SpanBuffer{data},SpanBuffer{tag2,offset});
            BOOST_CHECK(!ec);
            BOOST_CHECK(checkBin.isEqual(tag2.data()+offset,tag2.size()-offset));

            SpanBuffers buffers=CryptPluginTest::split(data,5);
            ByteArray tag3;
            ec=digest->runFinalize(buffers,tag3);
            BOOST_CHECK(!ec);
            ec=digest->runCheck(buffers,tag3);
            BOOST_CHECK(!ec);
            BOOST_CHECK(checkBin==tag3);
        }
    }
}

BOOST_AUTO_TEST_CASE(CheckDigests)
{
    CryptPluginTest::instance().eachPlugin<CryptTestTraits>(
        [](std::shared_ptr<CryptPlugin>& plugin)
        {
            if (plugin->isFeatureImplemented(Crypt::Feature::Digest))
            {
                const CryptAlgorithm* alg=nullptr;
                auto ec=plugin->findAlgorithm(alg,CryptAlgorithm::Type::DIGEST,"_dummy_");
                BOOST_CHECK(ec);

                auto digests=plugin->listDigests();
                HATN_REQUIRE(!digests.empty());
                for (auto&& it:digests)
                {
#ifdef PRINT_CRYPT_ENGINE_ALGS
                    BOOST_TEST_MESSAGE(it);
#endif
                    std::string path=fmt::format("{}/digest-{}.txt",PluginList::assetsPath("crypt"),it);
                    checkDigest(plugin,it,path);
                    path=fmt::format("{}/digest-{}.txt",PluginList::assetsPath("crypt",plugin->info()->name),it);
                    checkDigest(plugin,it,path);
                }
            }
        }
    );
}

BOOST_AUTO_TEST_SUITE_END()

}
}
