/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
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

#ifndef HATN_COMMON_TEST_EXPORT

#  if defined(_WIN32)
#        ifdef EXPORT_HATN_COMMON_TEST
#            define HATN_COMMON_TEST_EXPORT __declspec(dllexport)
#        else
#            ifdef IMPORT_HATN_COMMON_TEST
#               define HATN_COMMON_EXPORT __declspec(dllimport)
#            else
#               define HATN_COMMON_TEST_EXPORT
#            endif
#        endif
#  else
#    HATN_COMMON_TEST_EXPORT
#  endif

#endif

namespace hatn {
namespace test {

class MultiThreadFixture_p;

//! Test fixture for multithreaded tests
class HATN_COMMON_TEST_EXPORT MultiThreadFixture
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
            for (size_t i=0;i<buf.size();i++)
            {
                buf[i]=static_cast<char>(rand());
            }
        }

        static void randSizeTVector(std::vector<size_t>& buf)
        {
            std::array<uint8_t,sizeof(size_t)> wordArr;
            for (size_t i=0;i<buf.size();i++)
            {
                randBuf(wordArr);
                size_t word;
                memcpy(&word,wordArr.data(),sizeof(word));
                buf[i]=word;
            }
        }

        static hatn::common::MutexLock mutex;

        static std::string tmpPath();

        static std::string assetsPath() noexcept
        {
            return ASSETS_PATH;
        }
        static std::string assetsFilePath(const std::string& relPath) noexcept
        {
            return fmt::format("{}/{}",ASSETS_PATH,relPath);
        }

        static void setTmpPath(std::string path)
        {
            TMP_PATH=std::move(path);
        }

        static void setAssetsPath(std::string path)
        {
            ASSETS_PATH=std::move(path);
        }

        static std::string tmpFilePath(const std::string& relPath) noexcept
        {
            return fmt::format("{}/{}",tmpPath(),relPath);
        }

    private:

        static std::string ASSETS_PATH;
        static std::string TMP_PATH;

        std::unique_ptr<MultiThreadFixture_p> d;
};

}
}

#define HATN_CHECK_TS(...) \
    { hatn::common::MutexScopedLock l(hatn::test::MultiThreadFixture::mutex); \
    BOOST_CHECK(__VA_ARGS__); }

#define HATN_REQUIRE_TS(...) \
    { hatn::common::MutexScopedLock l(hatn::test::MultiThreadFixture::mutex); \
    BOOST_REQUIRE(__VA_ARGS__); }

#define HATN_CHECK_EQUAL_TS(...) \
    { hatn::common::MutexScopedLock l(hatn::test::MultiThreadFixture::mutex); \
    _DSC_EXPAND(BOOST_CHECK_EQUAL(__VA_ARGS__)); }

#define HATN_TEST_MESSAGE_TS(...) \
    { hatn::common::MutexScopedLock l(hatn::test::MultiThreadFixture::mutex); \
    BOOST_TEST_MESSAGE(__VA_ARGS__); }

#define HATN_REQUIRE(Cond) \
    BOOST_CHECK(Cond); \
    if (!(Cond)) \
    {\
        return;\
    }

#define HATN_CHECK_EXEC_SYNC(Expr) \
    if (Expr) \
    { \
        BOOST_FAIL("Timeout in Thread::execSync"); \
    }

#define HATN_REQUIRE_EQUAL(Val1,Val2) \
    BOOST_CHECK_EQUAL(Val1,Val2); \
    if ((Val1)!=(Val2)) \
    {\
        return;\
    }

#define HATN_REQUIRE_GE(Val1,Val2) \
    BOOST_CHECK_GE(Val1,Val2); \
    if ((Val1)<(Val2)) \
    {\
        return;\
    }

#define HATN_REQUIRE_GT(Val1,Val2) \
    BOOST_CHECK_GE(Val1,Val2); \
    if ((Val1)<=(Val2)) \
    {\
        return;\
    }

#define HATN_TEST_EC(ec) \
if (ec) \
{\
    BOOST_ERROR(ec.message());\
}

#define HATN_TEST_RESULT(r) \
if (r) \
{\
    BOOST_ERROR(r.error().message());\
}


#endif // HATNMULTITHREADFIXTURE_H
