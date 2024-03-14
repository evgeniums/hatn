#include <boost/test/unit_test.hpp>

#include <hatn/common/logger.h>
#include <hatn/test/multithreadfixture.h>

#include <hatn/common/pmr/withstaticallocator.h>
#include <hatn/common/pmr/withstaticallocator.ipp>
#define HATN_WITH_STATIC_ALLOCATOR_INLINE HATN_WITH_STATIC_ALLOCATOR_INLINE_SRC
#define HDU_DATAUNIT_EXPORT

#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/detail/syntax.ipp>

#include <hatn/dataunit/readunitfieldatpath.h>
#include <hatn/dataunit/updateunitfieldatpath.h>
#include <hatn/dataunit/prevalidatedupdate.h>

#include <hatn/validator/operators/lexicographical.hpp>

HATN_COMMON_USING
HATN_DATAUNIT_USING
//#define HATN_TEST_LOG_CONSOLE

static void setLogHandler()
{
    auto handler=[](const ::hatn::common::FmtAllocatedBufferChar &s)
    {
        #ifdef HATN_TEST_LOG_CONSOLE
            std::cout<<::hatn::common::lib::toStringView(s)<<std::endl;
        #else
            std::ignore=s;
        #endif
    };

    ::hatn::common::Logger::setOutputHandler(handler);
    ::hatn::common::Logger::setFatalLogHandler(handler);
    ::hatn::common::Logger::setDefaultVerbosity(::hatn::common::LoggerVerbosity::DEBUG);
    ::hatn::common::Logger::setDefaultDebugLevel(1);
}

struct Env : public ::hatn::test::MultiThreadFixture
{
    Env()
    {
        if (!::hatn::common::Logger::isRunning())
        {
            setLogHandler();
            ::hatn::common::Logger::setFatalTracing(false);
            ::hatn::common::Logger::start(false);
        }
    }

    ~Env()
    {
        ::hatn::common::Logger::stop();
    }

    Env(const Env&)=delete;
    Env(Env&&) =delete;
    Env& operator=(const Env&)=delete;
    Env& operator=(Env&&) =delete;
};
