/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/test/initcryptplugin.h
 *
 *     Init crypt plugin.
 *
 */
/****************************************************************************/

#ifndef HATNINITCRYPTPLUGIN_H
#define HATNINITCRYPTPLUGIN_H

#include <functional>

#include <hatn_test_config.h>

#include <hatn/crypt/cryptplugin.h>
#include <hatn/test/pluginlist.h>

namespace hatn {
namespace test {

struct CryptTestTraits
{
    constexpr static const char* module="crypt";
    using plugin_type=crypt::CryptPlugin;
};

class CryptPluginTest : public PluginTest
{
    public:

        void init()
        {
            auto handler=[](std::shared_ptr<crypt::CryptPlugin>& plugin)
            {
                auto ec=plugin->init();
                if (ec)
                {
                    std::string msg="Failed to init plugin "+plugin->info()->name+": "+ec.message();
                    plugin.reset();
                    BOOST_FAIL(msg.c_str());
                }
            };

            loadPlugins<CryptTestTraits>();
            eachPlugin<CryptTestTraits>(handler);
        }

        void cleanup()
        {
            auto handler=[](std::shared_ptr<crypt::CryptPlugin>& plugin)
            {
                auto ec=plugin->cleanup();
                if (ec)
                {
                    std::string msg="Failed to cleanup plugin "+plugin->info()->name+": "+ec.message();
                    BOOST_WARN_MESSAGE(false,msg.c_str());
                }
            };
            eachPlugin<CryptTestTraits>(handler);
            reset();
        }

        static CryptPluginTest& instance();

        static void initOnce();

        static void cleanUpOnce() noexcept
        {
            if (m_initialized)
            {
                m_initialized=false;
                instance().cleanup();
            }
        }

        static bool isInitialized() noexcept
        {
            return m_initialized;
        }

        template <typename ContainerT>
        static common::SpanBuffers split(const ContainerT& container, size_t count, size_t firstMinSize=0)
        {
            BOOST_REQUIRE(firstMinSize<=container.size());

            common::SpanBuffers buffers;
            if (container.size()==firstMinSize)
            {
                buffers.emplace_back(container);
                return buffers;
            }

            size_t size=(container.size()-firstMinSize)/count;
            if (size==0)
            {
                buffers.emplace_back(container);
                return buffers;
            }

            size_t lastPadding=container.size()-firstMinSize-(size*count);
            size_t offset=0;
            if (firstMinSize!=0)
            {
                buffers.emplace_back(container.data(),size+firstMinSize);
                --count;
                offset+=size+firstMinSize;
            }

            for (size_t i=0;i<count;i++)
            {
                size_t padding=0;
                if (i==(count-1))
                {
                    padding=lastPadding;
                }
                buffers.emplace_back(container.data()+offset,size+padding);
                offset+=size+padding;
            }
            BOOST_REQUIRE_EQUAL(offset,container.size());
            return buffers;
        }

    private:

        static bool m_initialized;
};

class CryptTestFixture : public MultiThreadFixture
{
    public:

        using MultiThreadFixture::MultiThreadFixture;

        void setup();
        void teardown();
};

}
}

#endif // HATNINITCRYPTPLUGIN_H
