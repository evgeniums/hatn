#include <map>
#include <string>

#include <boost/test/unit_test.hpp>

#include <hatn/common/thread.h>
#include <hatn/common/logger.h>
#include <hatn/test/multithreadfixture.h>
#include <hatn/common/translate.h>

#include <hatn/common/elapsedtimer.h>
#include <hatn/common/pmr/poolmemoryresource.h>
#include <hatn/common/memorypool/newdeletepool.h>

//#define HATN_TEST_LOG_CONSOLE

HATN_USING
HATN_COMMON_USING
HATN_TEST_USING

static const int CUT_PREFIX=14;

namespace {

using MemoryResource=memorypool::NewDeletePoolResource;
using MemoryPool=memorypool::NewDeletePool;

}

void checkStrings(const FmtAllocatedBufferChar &s,int &count,int &index,std::map<int,std::string> &strings)
{

#ifdef HATN_TEST_LOG_CONSOLE
    std::cerr<<lib::toStringView(s)<<std::endl;
#endif

    BOOST_REQUIRE(count<=index);
    auto str=fmtBufToString(s);
    str=str.substr(CUT_PREFIX,str.length());

    BOOST_CHECK_EQUAL(strings[count],str);
    count++;
}

BOOST_AUTO_TEST_SUITE(LoggerTest)

BOOST_FIXTURE_TEST_CASE(Basic,MultiThreadFixture)
{
    HATN_INFO(global,"OK");

    int index=0;
    int count=0;
    std::map<int,std::string> strings;
    strings[index++]="t[main] INFORMATION:global :: Hello world";
    strings[index++]="t[main] INFORMATION:global:Some context :: Hello world";
    strings[index++]="t[main] INFORMATION:global :: Hello world";
    strings[index++]="t[main] INFORMATION:global:Some context :: Hello world";
    strings[index++]="t[main] WARNING:global :: Hello world";
    strings[index++]="t[main] WARNING:global:Some context :: Hello world";
    strings[index++]="t[main] WARNING:global :: Hello world";
    strings[index++]="t[main] WARNING:global:Some context :: Hello world";
    strings[index++]="t[main] ERROR:global :: Hello world";
    strings[index++]="t[main] ERROR:global:Some context :: Hello world";
    strings[index++]="t[main] ERROR:global :: Hello world";
    strings[index++]="t[main] ERROR:global:Some context :: Hello world";
    strings[index++]="t[main] FATAL:global :: Hello world";
    strings[index++]="t[main] FATAL:global:Some context :: Hello world";
    strings[index++]="t[main] FATAL:global :: Hello world";
    strings[index++]="t[main] FATAL:global:Some context :: Hello world";

    auto handler=[&count,&index,&strings](const FmtAllocatedBufferChar &s)
    {
        checkStrings(s,count,index,strings);
    };

    Logger::setOutputHandler(handler);
    Logger::setFatalLogHandler(handler);
    Logger::setFatalTracing(false);

    Logger::start(false);
    pmr::CStringVector tags;
    tags.push_back("Some tag");

    HATN_INFO(global,"Hello world");
    HATN_INFO_CONTEXT(global,"Some context","Hello world");
    HATN_INFO_TAGS(global,tags,"Hello world");
    HATN_INFO_CONTEXT_TAGS(global,"Some context",tags,"Hello world");

    HATN_WARN(global,"Hello world");
    HATN_WARN_CONTEXT(global,"Some context","Hello world");
    HATN_WARN_TAGS(global,tags,"Hello world");
    HATN_WARN_CONTEXT_TAGS(global,"Some context",tags,"Hello world");

    HATN_ERROR(global,"Hello world");
    HATN_ERROR_CONTEXT(global,"Some context","Hello world");
    HATN_ERROR_TAGS(global,tags,"Hello world");
    HATN_ERROR_CONTEXT_TAGS(global,"Some context",tags,"Hello world");

    HATN_FATAL(global,"Hello world");
    HATN_FATAL_CONTEXT(global,"Some context","Hello world");
    HATN_FATAL_TAGS(global,tags,"Hello world");
    HATN_FATAL_CONTEXT_TAGS(global,"Some context",tags,"Hello world");

    Logger::stop();
}

BOOST_FIXTURE_TEST_CASE(Thread,MultiThreadFixture)
{
    Logger::stop();

    int index=0;
    int count=0;
    std::map<int,std::string> strings;
    strings[index++]="t[main] INFORMATION:global :: Hello world 0";
    strings[index++]="t[main] INFORMATION:global:Some context :: Hello world 1";
    strings[index++]="t[main] INFORMATION:global :: Hello world 2";
    strings[index++]="t[main] INFORMATION:global:Some context :: Hello world 3";
    strings[index++]="t[main] WARNING:global :: Hello world 4";
    strings[index++]="t[main] WARNING:global:Some context :: Hello world 5";
    strings[index++]="t[main] WARNING:global :: Hello world 6";
    strings[index++]="t[main] WARNING:global:Some context :: Hello world 7";
    strings[index++]="t[main] ERROR:global :: Hello world 8";
    strings[index++]="t[main] ERROR:global:Some context :: Hello world 9";
    strings[index++]="t[main] ERROR:global :: Hello world 10";
    strings[index++]="t[main] ERROR:global:Some context :: Hello world 11";
    strings[index++]="t[log_testings1] INFORMATION:global :: Hello world 0";
    strings[index++]="t[log_testings1] INFORMATION:global:Some context :: Hello world 1";
    strings[index++]="t[log_testings1] INFORMATION:global :: Hello world 2";
    strings[index++]="t[log_testings1] INFORMATION:global:Some context :: Hello world 3";
    strings[index++]="t[log_testings1] WARNING:global :: Hello world 4";
    strings[index++]="t[log_testings1] WARNING:global:Some context :: Hello world 5";
    strings[index++]="t[log_testings1] WARNING:global :: Hello world 6";
    strings[index++]="t[log_testings1] WARNING:global:Some context :: Hello world 7";
    strings[index++]="t[log_testings1] ERROR:global :: Hello world 8";
    strings[index++]="t[log_testings1] ERROR:global:Some context :: Hello world 9";
    strings[index++]="t[log_testings1] ERROR:global :: Hello world 10";
    strings[index++]="t[log_testings1] ERROR:global:Some context :: Hello world 11";
    strings[index++]="t[log_testings2] INFORMATION:global :: Hello world 0";
    strings[index++]="t[log_testings2] INFORMATION:global:Some context :: Hello world 1";
    strings[index++]="t[log_testings2] INFORMATION:global :: Hello world 2";
    strings[index++]="t[log_testings2] INFORMATION:global:Some context :: Hello world 3";
    strings[index++]="t[log_testings2] WARNING:global :: Hello world 4";
    strings[index++]="t[log_testings2] WARNING:global:Some context :: Hello world 5";
    strings[index++]="t[log_testings2] WARNING:global :: Hello world 6";
    strings[index++]="t[log_testings2] WARNING:global:Some context :: Hello world 7";
    strings[index++]="t[log_testings2] ERROR:global :: Hello world 8";
    strings[index++]="t[log_testings2] ERROR:global:Some context :: Hello world 9";
    strings[index++]="t[log_testings2] ERROR:global :: Hello world 10";
    strings[index++]="t[log_testings2] ERROR:global:Some context :: Hello world 11";
    strings[index++]="t[log_testings3] INFORMATION:global :: Hello world 0";
    strings[index++]="t[log_testings3] INFORMATION:global:Some context :: Hello world 1";
    strings[index++]="t[log_testings3] INFORMATION:global :: Hello world 2";
    strings[index++]="t[log_testings3] INFORMATION:global:Some context :: Hello world 3";
    strings[index++]="t[log_testings3] WARNING:global :: Hello world 4";
    strings[index++]="t[log_testings3] WARNING:global:Some context :: Hello world 5";
    strings[index++]="t[log_testings3] WARNING:global :: Hello world 6";
    strings[index++]="t[log_testings3] WARNING:global:Some context :: Hello world 7";
    strings[index++]="t[log_testings3] ERROR:global :: Hello world 8";
    strings[index++]="t[log_testings3] ERROR:global:Some context :: Hello world 9";
    strings[index++]="t[log_testings3] ERROR:global :: Hello world 10";
    strings[index++]="t[log_testings3] ERROR:global:Some context :: Hello world 11";
    strings[index++]="t[log_testings4] INFORMATION:global :: Hello world 0";
    strings[index++]="t[log_testings4] INFORMATION:global:Some context :: Hello world 1";
    strings[index++]="t[log_testings4] INFORMATION:global :: Hello world 2";
    strings[index++]="t[log_testings4] INFORMATION:global:Some context :: Hello world 3";
    strings[index++]="t[log_testings4] WARNING:global :: Hello world 4";
    strings[index++]="t[log_testings4] WARNING:global:Some context :: Hello world 5";
    strings[index++]="t[log_testings4] WARNING:global :: Hello world 6";
    strings[index++]="t[log_testings4] WARNING:global:Some context :: Hello world 7";
    strings[index++]="t[log_testings4] ERROR:global :: Hello world 8";
    strings[index++]="t[log_testings4] ERROR:global:Some context :: Hello world 9";
    strings[index++]="t[log_testings4] ERROR:global :: Hello world 10";
    strings[index++]="t[log_testings4] ERROR:global:Some context :: Hello world 11";
    strings[index++]="t[main] FATAL:global :: Hello world 12";
    strings[index++]="t[main] FATAL:global:Some context :: Hello world 13";
    strings[index++]="t[main] FATAL:global :: Hello world 14";
    strings[index++]="t[main] FATAL:global:Some context :: Hello world 15";

    auto handler=[&count,&index,&strings](const FmtAllocatedBufferChar &s)
    {
    #ifdef HATN_TEST_LOG_CONSOLE
        std::cerr<<lib::toStringView(s)<<std::endl;
    #endif

        BOOST_REQUIRE(count<=index);
        auto str=fmtBufToString(s);
        str=str.substr(CUT_PREFIX,str.length());

        if (count<12 || (count>(12+12*4)))
        {
            BOOST_CHECK_EQUAL(strings[count],str);
            count++;
        }
        bool found=false;
        for (auto&& it:strings)
        {
            if (it.second==str)
            {
                strings.erase(it.first);
                found=true;
                break;
            }
        }
        BOOST_CHECK(found);
    };

    Logger::setOutputHandler(handler);
    Logger::setFatalLogHandler(handler);
    Logger::setFatalTracing(false);
    Logger::start(true);

    pmr::CStringVector tags;
    tags.push_back("Some tag");

    auto runTest=[&tags]()
    {
        int i=0;

        HATN_INFO(global,"Hello world "<<i++); // 1
        HATN_INFO_CONTEXT(global,"Some context","Hello world "<<i++); // 2
        HATN_INFO_TAGS(global,tags,"Hello world "<<i++); // 3
        HATN_INFO_CONTEXT_TAGS(global,"Some context",tags,"Hello world "<<i++); // 4

        HATN_WARN(global,"Hello world "<<i++); // 5
        HATN_WARN_CONTEXT(global,"Some context","Hello world "<<i++); // 6
        HATN_WARN_TAGS(global,tags,"Hello world "<<i++); // 7
        HATN_WARN_CONTEXT_TAGS(global,"Some context",tags,"Hello world "<<i++); // 8

        HATN_ERROR(global,"Hello world "<<i++); // 9
        HATN_ERROR_CONTEXT(global,"Some context","Hello world "<<i++); // 10
        HATN_ERROR_TAGS(global,tags,"Hello world "<<i++); // 11
        HATN_ERROR_CONTEXT_TAGS(global,"Some context",tags,"Hello world "<<i++); // 12
    };

    int i=0;

    HATN_INFO(global,"Hello world "<<i++); // 1
    HATN_INFO_CONTEXT(global,"Some context","Hello world "<<i++); // 2
    HATN_INFO_TAGS(global,tags,"Hello world "<<i++); // 3
    HATN_INFO_CONTEXT_TAGS(global,"Some context",tags,"Hello world "<<i++); // 4

    HATN_WARN(global,"Hello world "<<i++); // 5
    HATN_WARN_CONTEXT(global,"Some context","Hello world "<<i++); // 6
    HATN_WARN_TAGS(global,tags,"Hello world "<<i++); // 7
    HATN_WARN_CONTEXT_TAGS(global,"Some context",tags,"Hello world "<<i++); // 8

    HATN_ERROR(global,"Hello world "<<i++); // 9
    HATN_ERROR_CONTEXT(global,"Some context","Hello world "<<i++); // 10
    HATN_ERROR_TAGS(global,tags,"Hello world "<<i++); // 11
    HATN_ERROR_CONTEXT_TAGS(global,"Some context",tags,"Hello world "<<i++); // 12

    auto testThread=new common::Thread("log_testings1");
    testThread->start();
    testThread->execAsync(runTest);

    auto testThread2=new common::Thread("log_testings2");
    testThread2->start();
    testThread2->execAsync(runTest);

    auto testThread3=new common::Thread("log_testings3");
    testThread3->start();
    testThread3->execAsync(runTest);

    auto testThread4=new common::Thread("log_testings4");
    testThread4->start();
    testThread4->execAsync(runTest);

    exec(3);

    HATN_FATAL(global,"Hello world "<<i++); // 13
    HATN_FATAL_CONTEXT(global,"Some context","Hello world "<<i++); // 14
    HATN_FATAL_TAGS(global,tags,"Hello world "<<i++); // 15
    HATN_FATAL_CONTEXT_TAGS(global,"Some context",tags,"Hello world "<<i++); // 16

    BOOST_CHECK_EQUAL(strings.size(),0);

    delete testThread;
    delete testThread2;
    delete testThread3;
    delete testThread4;

    Logger::stop();
}

BOOST_FIXTURE_TEST_CASE(Debug,MultiThreadFixture)
{
    Logger::stop();

    int index=0;
    int count=0;
    std::map<int,std::string> strings;
    strings[index++]="t[main] DEBUG:0:global :: Default level";
    strings[index++]="t[main] DEBUG:5:global :: Level 5";
    strings[index++]="t[main] DEBUG:7:global:Some context :: Level 7 with context";
    strings[index++]="t[main] DEBUG:4:global:Some tag :: Hello world";
    strings[index++]="t[main] DEBUG:4:global:Some context:Some tag :: Hello world";
    strings[index++]="t[main] DEBUG:4:global:Some tag :: Hello world";
    strings[index++]="t[main] DEBUG:5:global :: Hello world";

    auto handler=[&count,&index,&strings](const FmtAllocatedBufferChar &s)
    {
        checkStrings(s,count,index,strings);
    };

    Logger::setOutputHandler(handler);
    Logger::setFatalLogHandler(handler);
    Logger::setFatalTracing(false);
    Logger::start(true);

    std::vector<std::string> contextsStr;
    contextsStr.push_back("Some context");
    std::vector<std::string> tagsStr;
    tagsStr.push_back("Some tag");

    LOG_MODULE(global)::i()->configure(LoggerVerbosity::DEBUG,7,std::move(contextsStr),std::move(tagsStr));

    pmr::CStringVector tags;
    tags.push_back("Some tag");

    HATN_DEBUG(global,"Default level");

    HATN_DEBUG_LVL(global,5,"Level 5");
    HATN_DEBUG_LVL(global,10,"Level 10");

    HATN_DEBUG_CONTEXT(global,"Some context",7,"Level 7 with context");
    HATN_DEBUG_TAGS(global,tags,4,"Hello world");
    HATN_DEBUG_CONTEXT_TAGS(global,"Some context",tags,4,"Hello world");

    tags.push_back("Some tag 3");
    HATN_DEBUG_TAGS(global,tags,4,"Hello world");

    tags.clear();
    tags.push_back("Some tag 2");
    HATN_DEBUG_LVL(global,5,"Hello world");
    HATN_DEBUG_CONTEXT(global,"Some context 1",5,"Hello world");
    HATN_DEBUG_TAGS(global,tags,5,"Hello world");
    HATN_DEBUG_CONTEXT_TAGS(global,"Some context 1",tags,1,"Hello world");

    exec(2);

    Logger::stop();
}

BOOST_FIXTURE_TEST_CASE(DebugParse,MultiThreadFixture)
{
    Logger::stop();

    int index=0;
    int count=0;
    std::map<int,std::string> strings;
    strings[index++]="t[main] DEBUG:0:global :: Default level";
    strings[index++]="t[main] DEBUG:5:global :: Level 5";
    strings[index++]="t[main] DEBUG:7:global:Some context :: Level 7 with context";
    strings[index++]="t[log_testings] DEBUG:0:global :: Default level";
    strings[index++]="t[main] DEBUG:4:global:Some tag :: Hello world";
    strings[index++]="t[log_testings] DEBUG:5:global :: Level 5";
    strings[index++]="t[main] DEBUG:4:global:Some context:Some tag :: Hello world";
    strings[index++]="t[log_testings] DEBUG:7:global:Some context :: Level 7 with context";
    strings[index++]="t[main] DEBUG:4:global:Some tag :: Hello world";
    strings[index++]="t[log_testings] DEBUG:4:global:Some tag :: Hello world";
    strings[index++]="t[main] DEBUG:5:global :: Hello world";
    strings[index++]="t[log_testings] DEBUG:4:global:Some context:Some tag :: Hello world";
    strings[index++]="t[main] DEBUG:5:global:Some tag 2 :: Hello world";
    strings[index++]="t[log_testings] DEBUG:4:global:Some tag :: Hello world";
    strings[index++]="t[main] DEBUG:1:global:Some context 2:Some tag 2 :: Hello world";
    strings[index++]="t[log_testings] DEBUG:5:global :: Hello world";
    strings[index++]="t[log_testings] DEBUG:5:global:Some tag 2 :: Hello world";
    strings[index++]="t[log_testings] DEBUG:1:global:Some context 2:Some tag 2 :: Hello world";

    auto handler=[&count,&index,&strings](const FmtAllocatedBufferChar &s)
    {
    #ifdef HATN_TEST_LOG_CONSOLE
        std::cerr<<lib::toStringView(s)<<std::endl;
    #endif

        BOOST_REQUIRE(count<=index);
        auto str=fmtBufToString(s);
        str=str.substr(CUT_PREFIX,str.length());

        bool found=false;
        for (auto&& it:strings)
        {
            if (it.second==str)
            {
                strings.erase(it.first);
                found=true;
                break;
            }
        }
        BOOST_CHECK(found);
    };

    Logger::setOutputHandler(handler);
    Logger::setFatalLogHandler(handler);
    Logger::setFatalTracing(false);
    Logger::start(true);

    std::vector<std::string> modules;

    std::string conf="globalljlj;debug;7;Some context,Some context 2;Some tag,Some tag 2";
    modules.push_back(conf);
    auto ret1=Logger::configureModules(modules);
    BOOST_CHECK_EQUAL(ret1,(_TR("No such log module defined:")+" globalljlj"));
#ifdef HATN_TEST_LOG_CONSOLE
    std::cerr<<ret1<<std::endl;
#endif
    modules.clear();
    conf="oho,43-49-0kjkh ljh ihoihfweef";
    modules.push_back(conf);
    auto ret2=Logger::configureModules(modules);
    BOOST_CHECK_EQUAL(ret2,(_TR("No such log module defined:")+" oho,43-49-0kjkh ljh ihoihfweef"));
#ifdef HATN_TEST_LOG_CONSOLE
    std::cerr<<ret2<<std::endl;
#endif
    modules.clear();
    conf="";
    modules.push_back(conf);
    auto ret3=Logger::configureModules(modules);
    BOOST_CHECK_EQUAL(ret3,(_TR("No such log module defined:")+" "));
#ifdef HATN_TEST_LOG_CONSOLE
    std::cerr<<ret3<<std::endl;
#endif
    modules.clear();
    conf="global;debug;7;Some context,Some context 2;Some tag,Some tag 2";
    modules.push_back(conf);
    Logger::configureModules(modules);

    auto runTest=[]()
    {
        pmr::CStringVector tags;
        tags.push_back("Some tag");

        HATN_DEBUG(global,"Default level");

        HATN_DEBUG_LVL(global,5,"Level 5");
        HATN_DEBUG_LVL(global,10,"Level 10");

        HATN_DEBUG_CONTEXT(global,"Some context",7,"Level 7 with context");
        HATN_DEBUG_TAGS(global,tags,4,"Hello world");
        HATN_DEBUG_CONTEXT_TAGS(global,"Some context",tags,4,"Hello world");

        tags.push_back("Some tag 3");
        HATN_DEBUG_TAGS(global,tags,4,"Hello world");

        tags.clear();
        tags.push_back("Some tag 2");
        HATN_DEBUG_LVL(global,5,"Hello world");
        HATN_DEBUG_CONTEXT(global,"Some context 1",5,"Hello world");
        HATN_DEBUG_TAGS(global,tags,5,"Hello world");
        HATN_DEBUG_CONTEXT_TAGS(global,"Some context 1",tags,1,"Hello world");
        HATN_DEBUG_CONTEXT_TAGS(global,"Some context 2",tags,1,"Hello world");
    };

    Thread::mainThread()->execAsync(runTest);

    auto testThread=new common::Thread("log_testings");
    testThread->start();
    testThread->execAsync(runTest);

    exec(4);

    BOOST_CHECK_EQUAL(strings.size(),0);

    delete testThread;

    Logger::stop();
    Logger::resetModules();
}

BOOST_FIXTURE_TEST_CASE(Format,MultiThreadFixture)
{
    Logger::stop();

    int index=0;
    int count=0;
    std::map<int,std::string> strings;
    strings[index++]="t[main] INFORMATION:global :: Hello world";
    strings[index++]="t[main] INFORMATION:global :: Hello world 1 2.20000 -3 4 how are you";
    strings[index++]="t[main] WARNING:global :: Hello world";
    strings[index++]="t[main] WARNING:global :: Hello world 1 2.20000 -3 4 how are you";
    strings[index++]="t[main] ERROR:global :: Hello world";
    strings[index++]="t[main] ERROR:global :: Hello world 1 2.20000 -3 4 how are you";

    auto handler=[&count,&index,&strings](const FmtAllocatedBufferChar &s)
    {
        checkStrings(s,count,index,strings);
    };

    Logger::setOutputHandler(handler);
    Logger::setFatalLogHandler(handler);
    Logger::setFatalTracing(false);
    Logger::start(false);

    HATN_INFO(global,"Hello world");
    HATN_INFO(global,HATN_FORMAT("Hello world {} {:.5f} {} {} {}",1,2.2f,-3,4,"how are you"));

    HATN_WARN(global,"Hello world");
    HATN_WARN(global,HATN_FORMAT("Hello world {} {:.5f} {} {} {}",1,2.2f,-3,4,"how are you"));

    HATN_ERROR(global,"Hello world");
    HATN_ERROR(global,HATN_FORMAT("Hello world {} {:.5f} {} {} {}",1,2.2f,-3,4,"how are you"));

    Logger::stop();
}

BOOST_FIXTURE_TEST_CASE(PerformanceInline,MultiThreadFixture,* boost::unit_test::disabled())
{
    Logger::stop();

    std::string str;
    int ccc=0;
    int count=0;
    auto handler=[&count,&str,&ccc](const FmtAllocatedBufferChar &msg)
    {
        std::stringstream s;
        s<<lib::toStringView(msg);
        str=s.str();

        int i=30;
        while(i--)
        {
            ccc++;
            str+=std::to_string(i);
        }
        ++count;
    };

    std::vector<std::string> modules;
    std::string conf="global;debug;7;Dummy context;";
    modules.push_back(conf);
    Logger::configureModules(modules);

    int cycles=2;
    while(cycles--)
    {
        auto resource=std::make_unique<MemoryResource>(std::make_shared<memorypool::PoolCacheGen<MemoryPool>>());
        LogModuleTable::setBufferMemoryResource(resource.get());

        if (cycles==0)
        {
            std::cerr<<"Log to separate thread"<<std::endl;
        }
        else
        {
            std::cerr<<"Log to the same thread"<<std::endl;
        }

        Logger::setOutputHandler(handler);
        Logger::setFatalLogHandler(handler);
        Logger::setFatalTracing(false);
        Logger::start(cycles==0);

        int runsCount=0;
        int runs=1000000;

        ElapsedTimer elapsed;

        auto perSecond=[&runs,&elapsed]()
        {
            auto ms=elapsed.elapsed().totalMilliseconds;
            if (ms==0)
            {
                return 1000*runs;
            }
            // codechecker_intentional [all] Don't care
            return static_cast<int>(round(1000*(runs/ms)));
        };

        std::cerr<<"Empty cycle start"<<std::endl;

        count=0;
        elapsed.reset();
        for (int i=0;i<runs;++i)
        {
            ++runsCount;
        }

        auto duration=elapsed.elapsed().totalMilliseconds;
        std::cerr<<"Duration "<<duration<<" milliseconds, perSecond="<<perSecond()<<std::endl;

        std::cerr<<"Simple text cycle start"<<std::endl;

        count=0;
        elapsed.reset();
        for (int i=0;i<runs;++i)
        {
            ++runsCount;
            HATN_INFO(global,"Hello world");
        }
        duration=elapsed.elapsed().totalMilliseconds;
        std::cerr<<"Duration "<<duration<<" milliseconds, count "<<count<<", perSecond="<<perSecond()<<std::endl;

        std::cerr<<"Streamed text cycle start"<<std::endl;

        count=0;
        elapsed.reset();
        for (int i=0;i<runs;++i)
        {
            HATN_INFO(global,"Hello world "<<1<<2<<3<<++runsCount<<"how are you");
        }
        duration=elapsed.elapsed().totalMilliseconds;
        std::cerr<<"Duration "<<duration<<" milliseconds, count "<<count<<", perSecond="<<perSecond()<<std::endl;

        std::cerr<<"Formatted text cycle start"<<std::endl;

        count=0;
        elapsed.reset();
        for (int i=0;i<runs;++i)
        {
            HATN_INFO(global,HATN_FORMAT("Hello world {} {} {} {} {}",1,2,3,++runsCount,"how are you"));
        }
        duration=elapsed.elapsed().totalMilliseconds;
        std::cerr<<"Duration "<<duration<<" milliseconds, count "<<count<<", perSecond="<<perSecond()<<std::endl;

        std::cerr<<"Disabled log cycle start"<<std::endl;

        count=0;
        elapsed.reset();
        for (int i=0;i<runs;++i)
        {
            HATN_INFO_CONTEXT(global,"Some context","Hello world "<<1<<2<<3<<++runsCount<<"how are you");
        }
        duration=elapsed.elapsed().totalMilliseconds;
        std::cerr<<"Duration "<<duration<<" milliseconds, count "<<count<<", perSecond="<<perSecond()<<std::endl;

        exec(5);

        std::cerr<<"Count "<<count<<std::endl;

        Logger::stop();
        LogModuleTable::setBufferMemoryResource(pmr::get_default_resource());
    }
}

BOOST_AUTO_TEST_SUITE_END()
