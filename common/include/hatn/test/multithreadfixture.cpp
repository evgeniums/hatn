/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file multithreadfixture.сpp
  *
  *     Base test fixture for multithreaded tests
  *
  */

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"

#endif

#include <boost/asio/deadline_timer.hpp>
#include <boost/filesystem.hpp>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#endif

#include <hatn/common/format.h>

#include <hatn/common/utils.h>
#include <hatn/common/weakptr.h>
#include <hatn/common/bytearray.h>

#include <hatn/common/pointers/mempool/weakpool.h>

#include <hatn/common/mutexqueue.h>
#include <hatn/common/mpscqueue.h>

#include <hatn/test/pluginlist.h>
#include <hatn/test/multithreadfixture.h>

namespace hatn {
using namespace common;
namespace test {

hatn::common::MutexLock MultiThreadFixture::mutex;

class MultiThreadFixture_p
{
    public:

        std::map<int,std::shared_ptr<Thread>> threads;
        bool timeouted=false;
};

//---------------------------------------------------------------
MultiThreadFixture::MultiThreadFixture(
        common::Thread* mThread
    ) : d(std::make_unique<MultiThreadFixture_p>())
{
    std::shared_ptr<Thread> mainThread(mThread);
    if (!mainThread)
    {
        mainThread=std::make_shared<Thread>("main",false);
    }
    Thread::setMainThread(mainThread);

    pointers_mempool::WeakPool::init();
}

//---------------------------------------------------------------
MultiThreadFixture::~MultiThreadFixture()
{
    d->threads.clear();
    Thread::releaseMainThread();

    pointers_mempool::WeakPool::free();
}

//---------------------------------------------------------------
void MultiThreadFixture::createThreads(int count, bool queued, bool queueWithContext, bool useMutexThread)
{
    Assert(count>0,"You can't create negative number of threads");
    Assert(d->threads.empty(),"You have already created test threads");
    for (int i=0;i<count;i++)
    {
        std::string id=fmt::format("test_{}",i);
        std::shared_ptr<Thread> thread;
        if (queued)
        {
            if (queueWithContext)
            {
                if (useMutexThread)
                {
                    auto queue=new MutexQueue<TaskWithContext>();
                    thread=std::make_shared<ThreadWithQueue<TaskWithContext>>(id.c_str(),queue);
                }
                else
                {
                    auto queue=new MPSCQueue<TaskWithContext>();
                    thread=std::make_shared<ThreadWithQueue<TaskWithContext>>(id.c_str(),queue);
                }
            }
            else
            {
                if (useMutexThread)
                {
                    auto queue=new MutexQueue<Task>();
                    thread=std::make_shared<ThreadWithQueue<Task>>(id.c_str(),queue);
                }
                else
                {
                    auto queue=new MPSCQueue<Task>();
                    thread=std::make_shared<ThreadWithQueue<Task>>(id.c_str(),queue);
                }
            }
        }
        else
        {
            thread=std::make_shared<Thread>(id.c_str());
        }
        d->threads[i]=thread;
    }
}

//---------------------------------------------------------------
void MultiThreadFixture::destroyThreads()
{
    d->threads.clear();
}

//---------------------------------------------------------------
void MultiThreadFixture::quit()
{
    mainThread()->stop();
    d->timeouted=false;
}

//---------------------------------------------------------------
std::shared_ptr<Thread> MultiThreadFixture::mainThread()
{
    return Thread::mainThread();
}

//---------------------------------------------------------------
std::shared_ptr<Thread> MultiThreadFixture::thread(int index) const
{
    auto it=d->threads.find(index);
    if (it!=d->threads.end())
    {
        return it->second;
    }
    return std::shared_ptr<Thread>();
}

//---------------------------------------------------------------
bool MultiThreadFixture::exec(
        int timeoutSeconds
    )
{
    if (mainThread()->isStarted())
    {
        return false;
    }
    mainThread()->installTimer(timeoutSeconds*1000*1000,
                               [this]()
                                {
                                    d->timeouted=true;
                                    mainThread()->stop();
                                    return false;
                                },true
                               );
    mainThread()->start();
    return d->timeouted;
}

//---------------------------------------------------------------
std::string MultiThreadFixture::tmpPath()
{
    if (!boost::filesystem::exists(TMP_PATH))
    {
        boost::filesystem::create_directory(TMP_PATH);
    }
    return TMP_PATH;
}

#ifdef TEST_ASSETS_PATH
std::string MultiThreadFixture::ASSETS_PATH=TEST_ASSETS_PATH;
#else
std::string MultiThreadFixture::ASSETS_PATH=".";
#endif

#ifdef TEST_TMP_PATH
std::string MultiThreadFixture::TMP_PATH=TEST_TMP_PATH;
#else
std::string MultiThreadFixture::TMP_PATH="./tmp";
#endif
//---------------------------------------------------------------
} // namespace test
} // namespace hatn
