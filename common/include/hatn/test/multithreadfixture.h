/*
    Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file multithreadfixture.h
  *
  *     Base test fixture for multithreaded tests
  *
  */

/****************************************************************************/

#ifndef HATNMULTITHREADFIXTURE_H
#define HATNMULTITHREADFIXTURE_H

#include <memory>

#include <hatn/common/locker.h>
#include <hatn/common/logger.h>

#include <hatn/common/memorypool/poolcachegen.h>
#include <hatn/common/threadwithqueue.h>

namespace hatn {
namespace test {

class MultiThreadFixture_p;

//! Test fixture for multithreaded tests
class MultiThreadFixture
{
    public:

        //! Ctor
        MultiThreadFixture(
            common::Thread* mainThread=nullptr
        );

        //! Dtor
        virtual ~MultiThreadFixture();

        MultiThreadFixture(const MultiThreadFixture&)=delete;
        MultiThreadFixture(MultiThreadFixture&&) =delete;
        MultiThreadFixture& operator=(const MultiThreadFixture&)=delete;
        MultiThreadFixture& operator=(MultiThreadFixture&&) =delete;

        //! Create test threads
        virtual void createThreads(
            int count=1,
            bool queued=true,
            bool queueWithContext=true,
            bool useMutexThread=true
        );

        void destroyThreads();

        /**
         * @brief exec Execute test
         * @param timeoutSeconds Test timeout in seconds
         * @return true on normal quit or false if timeouted
         */
        bool exec(
            int timeoutSeconds=5
        );

        //! Quit test
        void quit();

        //! Get test thread
        std::shared_ptr<::hatn::common::Thread> thread(int index=0) const;

        //! Get main thread
        static std::shared_ptr<::hatn::common::Thread> mainThread();

        template <typename PoolT>
        std::shared_ptr<::hatn::common::memorypool::PoolCacheGen<PoolT>> poolCacheGen()
        {
            static std::shared_ptr<::hatn::common::memorypool::PoolCacheGen<PoolT>> gen(
                            std::make_shared<::hatn::common::memorypool::PoolCacheGen<PoolT>>()
                        );
            return gen;
        }

        template <size_t size> static void randBuf(std::array<char,size>& buf)
        {
            srand(static_cast<unsigned int>(time(NULL)));
            for (size_t i=0;i<size;i++)
            {
                buf[i]=static_cast<char>(rand());
            }
        }

        template <typename ContainerT>
        static void randBuf(ContainerT& buf)
        {
            srand(static_cast<unsigned int>(time(NULL)));
            for (size_t i=0;i<buf.size();i++)
            {
                buf[i]=static_cast<char>(rand());
            }
        }

        static hatn::common::MutexLock mutex;

        static std::string tmpPath();
        static std::string assetsPath() noexcept
        {
            return ASSETS_PATH;
        }
        static void setTmpPath(std::string path)
        {
            TMP_PATH=std::move(path);
        }
        static void setAssetsPath(std::string path)
        {
            ASSETS_PATH=std::move(path);
        }

    private:

        static std::string ASSETS_PATH;
        static std::string TMP_PATH;

        std::unique_ptr<MultiThreadFixture_p> d;
};

}
}

#define DCS_CHECK_TS(...) \
    { hatn::common::MutexScopedLock l(hatn::test::MultiThreadFixture::mutex); \
    BOOST_CHECK(__VA_ARGS__); }

#define DCS_REQUIRE_TS(...) \
    { hatn::common::MutexScopedLock l(hatn::test::MultiThreadFixture::mutex); \
    BOOST_REQUIRE(__VA_ARGS__); }

#define DCS_CHECK_EQUAL_TS(...) \
    { hatn::common::MutexScopedLock l(hatn::test::MultiThreadFixture::mutex); \
    _DSC_EXPAND(BOOST_CHECK_EQUAL(__VA_ARGS__)); }

#define DCS_TEST_MESSAGE_TS(...) \
    { hatn::common::MutexScopedLock l(hatn::test::MultiThreadFixture::mutex); \
    BOOST_TEST_MESSAGE(__VA_ARGS__); }

#define DCS_REQUIRE(Cond) \
    BOOST_CHECK(Cond); \
    if (!(Cond)) \
    {\
        return;\
    }

#define DCS_CHECK_EXEC_SYNC(Expr) \
    if (Expr) \
    { \
        BOOST_FAIL("Timeout in Thread::execSync"); \
    }

#define DCS_REQUIRE_EQUAL(Val1,Val2) \
    BOOST_CHECK_EQUAL(Val1,Val2); \
    if ((Val1)!=(Val2)) \
    {\
        return;\
    }\

#define DCS_REQUIRE_GE(Val1,Val2) \
    BOOST_CHECK_GE(Val1,Val2); \
    if ((Val1)<(Val2)) \
    {\
        return;\
    }\

#define DCS_REQUIRE_GT(Val1,Val2) \
    BOOST_CHECK_GE(Val1,Val2); \
    if ((Val1)<=(Val2)) \
    {\
        return;\
    }\

#endif // HATNMULTITHREADFIXTURE_H
