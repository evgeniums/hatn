/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file logcontext/test/testlogcontext.cpp
  */

/****************************************************************************/

#include <fmt/chrono.h>

#include <boost/test/unit_test.hpp>

#include "hatn_test_config.h"

#include <hatn/common/datetime.h>
#include <hatn/common/logger.h>
#include <hatn/common/loggermoduleimp.h>

#include <hatn/logcontext/logcontext.h>
#include <hatn/logcontext/record.h>
#include <hatn/logcontext/context.h>
#include <hatn/logcontext/contextlogger.h>
#include <hatn/logcontext/streamlogger.h>

HATN_COMMON_USING
HATN_LOGCONTEXT_USING

#define NONE_EXPORT
DECLARE_LOG_MODULE_EXPORT(sample_module,NONE_EXPORT)
INIT_LOG_MODULE(sample_module,NONE_EXPORT)

BOOST_AUTO_TEST_SUITE(TestLogContext)

BOOST_AUTO_TEST_CASE(FormatLogValue)
{
    FmtAllocatedBufferChar buf;

    Value v1{int8_t{10}};
    serializeValue(buf,v1);
    auto str=fmtBufToString(buf);
    BOOST_TEST_MESSAGE(fmt::format("int8_t value: {}",str));
    BOOST_CHECK_EQUAL(str,"10");

    buf.clear();
    Value v2{"hello"};
    serializeValue(buf,v2);
    str=fmtBufToString(buf);
    BOOST_TEST_MESSAGE(fmt::format("string value: {}",str));
    BOOST_CHECK_EQUAL(str,"\"hello\"");

    buf.clear();
    Value v3{int8_t{-10}};
    serializeValue(buf,v3);
    str=fmtBufToString(buf);
    BOOST_TEST_MESSAGE(fmt::format("negative int8_t value: {}",str));
    BOOST_CHECK_EQUAL(str,"-10");

    buf.clear();
    Value v4{uint8_t{20}};
    serializeValue(buf,v4);
    str=fmtBufToString(buf);
    BOOST_TEST_MESSAGE(fmt::format("uint8_t value: {}",str));
    BOOST_CHECK_EQUAL(str,"20");

    buf.clear();
    Value v5{int16_t{-30}};
    serializeValue(buf,v5);
    str=fmtBufToString(buf);
    BOOST_TEST_MESSAGE(fmt::format("int16_t value: {}",str));
    BOOST_CHECK_EQUAL(str,"-30");

    buf.clear();
    Value v6{uint16_t{30}};
    serializeValue(buf,v6);
    str=fmtBufToString(buf);
    BOOST_TEST_MESSAGE(fmt::format("uint16_t value: {}",str));
    BOOST_CHECK_EQUAL(str,"30");

    buf.clear();
    Value v7{int32_t{-40}};
    serializeValue(buf,v7);
    str=fmtBufToString(buf);
    BOOST_TEST_MESSAGE(fmt::format("int32_t value: {}",str));
    BOOST_CHECK_EQUAL(str,"-40");

    buf.clear();
    Value v8{uint32_t{40}};
    serializeValue(buf,v8);
    str=fmtBufToString(buf);
    BOOST_TEST_MESSAGE(fmt::format("uint32_t value: {}",str));
    BOOST_CHECK_EQUAL(str,"40");

    buf.clear();
    Value v9{int64_t{-50}};
    serializeValue(buf,v9);
    str=fmtBufToString(buf);
    BOOST_TEST_MESSAGE(fmt::format("int64_t value: {}",str));
    BOOST_CHECK_EQUAL(str,"-50");

    buf.clear();
    Value v10{uint64_t{50}};
    serializeValue(buf,v10);
    str=fmtBufToString(buf);
    BOOST_TEST_MESSAGE(fmt::format("uint64_t value: {}",str));
    BOOST_CHECK_EQUAL(str,"50");

    buf.clear();
    Value v11{int64_t{0xffffffff5}};
    serializeValue(buf,v11);
    str=fmtBufToString(buf);
    BOOST_TEST_MESSAGE(fmt::format("int64_t long value: {}",str));
    BOOST_CHECK_EQUAL(str,"0xffffffff5");

    buf.clear();
    Value v12{-int64_t{0xffffffff5}};
    serializeValue(buf,v12);
    str=fmtBufToString(buf);
    BOOST_TEST_MESSAGE(fmt::format("int64_t long negative value: {}",str));
    BOOST_CHECK_EQUAL(str,"-0xffffffff5");

    buf.clear();
    Value v13{uint64_t{0xffffffff9}};
    serializeValue(buf,v13);
    str=fmtBufToString(buf);
    BOOST_TEST_MESSAGE(fmt::format("uint64_t long value: {}",str));
    BOOST_CHECK_EQUAL(str,"0xffffffff9");

    buf.clear();
    auto dt=DateTime::currentUtc();
    Value v14{dt};
    serializeValue(buf,v14);
    str=fmtBufToString(buf);
    BOOST_TEST_MESSAGE(fmt::format("DateTime value: {}",str));
    BOOST_CHECK_EQUAL(str,dt.toIsoString());

    buf.clear();
    auto d=Date::currentUtc();
    Value v15{d};
    serializeValue(buf,v15);
    str=fmtBufToString(buf);
    BOOST_TEST_MESSAGE(fmt::format("Date value: {}",str));
    BOOST_CHECK_EQUAL(str,d.toString(Date::Format::Number));

    buf.clear();
    auto t=Time::currentUtc();
    Value v16{t};
    serializeValue(buf,v16);
    str=fmtBufToString(buf);
    BOOST_TEST_MESSAGE(fmt::format("Time value: {}",str));
    BOOST_CHECK_EQUAL(str,t.toString());

    buf.clear();
    auto r=DateRange{d};
    Value v17{r};
    serializeValue(buf,v17);
    str=fmtBufToString(buf);
    BOOST_TEST_MESSAGE(fmt::format("DateRange value: {}",str));
    BOOST_CHECK_EQUAL(str,r.toString());
}

BOOST_AUTO_TEST_CASE(CreateLogContext)
{
    BOOST_TEST_MESSAGE(fmt::format("Context size {}",sizeof(Context)));

    ThreadSubcontext<TaskSubcontextT<Context>> tlCtx;
    BOOST_CHECK(tlCtx.value()==nullptr);

    TaskSubcontextT<Context> sl{};
    tlCtx.setValue(&sl);
    BOOST_CHECK(tlCtx.value()==&sl);

    tlCtx.reset();
    BOOST_CHECK(tlCtx.value()==nullptr);

    auto ctx=makeTaskContext<Context>();
    ctx->beforeThreadProcessing();
    BOOST_CHECK(tlCtx.value()!=nullptr);

    auto& subctx=ctx->get<Context>();
    BOOST_CHECK(tlCtx.value()==&subctx);

    ctx->afterThreadProcessing();
    BOOST_CHECK(tlCtx.value()==nullptr);
}

#if 0
class TestLoggerHandler : public LoggerHandler
{
    public:

        void log(
            LogLevel level,
            const Context* ctx,
            pmr::string msg,
            lib::string_view module=lib::string_view{},
            pmr::vector<Record> records=pmr::vector<Record>{}
            ) override
        {
#if __cplusplus < 201703L
            auto start=ctx->taskCtx()->startedAt();
#else
            auto start=std::chrono::floor<std::chrono::microseconds>(ctx->taskCtx()->startedAt());
#endif
            auto str=fmt::format("level={}, ctx={}, start={:%Y%m%dT%H:%M:%S}, elapsed={}, msg=\"{}\", module={}, records_count={}",
                                           logLevelName(level),
                                           ctx->taskCtx()->id().c_str(),
                                           start,
                                           ctx->taskCtx()->finishMicroseconds(),
                                           msg,
                                           module,
                                           records.size()
                                           );
            BOOST_TEST_MESSAGE(str);
        }

        void logError(
            LogLevel level,
            const Error& ec,
            const Context* ctx,
            pmr::string msg,
            lib::string_view module=lib::string_view{},
            pmr::vector<Record> records=pmr::vector<Record>{}
        ) override
        {
#if __cplusplus < 201703L
            auto start=ctx->taskCtx()->startedAt();
#else
            auto start=std::chrono::floor<std::chrono::microseconds>(ctx->taskCtx()->startedAt());
#endif
            auto str=fmt::format("level={}, ctx={}, start={:%Y%m%dT%H:%M:%S}, elapsed={}, ec=\"{}\", msg=\"{}\", module={}, records_count={}",
                                           logLevelName(level),
                                           ctx->taskCtx()->id().c_str(),
                                           start,
                                           ctx->taskCtx()->finishMicroseconds(),
                                           ec.message(),
                                           msg,
                                           module,
                                           records.size()
                                           );
            BOOST_TEST_MESSAGE(str);
        }
};
#endif

struct A1
{
    A1(TaskContext*)
    {}

    A1(A1 &&)
    {
        BOOST_TEST_MESSAGE("A1 move construtor");
    }
};

struct A2
{
    A2(TaskContext*)
    {}
};

template <typename ...Types>
struct B1 : public TaskContext
{
    using type=hana::tuple<Types...>;

    static auto replicateThis(TaskContext* self)
    {
        return hana::replicate<hana::tuple_tag>(self,hana::size_t<sizeof...(Types)>{});
    }

    B1() : fields(replicateThis(this))
    {}

    type fields;
};

BOOST_AUTO_TEST_CASE(TestStreamLogger)
{
    B1<A1,A2> b1;
    std::ignore=b1;

    auto handler=std::make_shared<StreamLogger>();
    ContextLogger::init(std::static_pointer_cast<LoggerHandler>(handler));

    auto ctx=makeTaskContext<Context>();

    auto& logCtx=ctx->get<Context>();
    logCtx.mainCtx().setTz(DateTime::localTz());

    ctx->beforeThreadProcessing();

    BOOST_TEST_MESSAGE("Log with context log level=INFO");

    Error ec{CommonError::UNKNOWN};

    HATN_CTX_FATAL(ec,"Fatal without module");
    HATN_CTX_FATAL(ec,"Fatal with module",sample_module);
    HATN_CTX_ERROR(ec,"Error without module");
    HATN_CTX_ERROR(ec,"Error with module",sample_module);

    HATN_CTX_WARN("Warn without module");
    HATN_CTX_WARN("Warn with module",sample_module);
    HATN_CTX_INFO("Info without module");
    HATN_CTX_INFO("Info with module",sample_module);
    HATN_CTX_DEBUG("Debug without module");
    HATN_CTX_DEBUG("Debug with module",sample_module);
    HATN_CTX_TRACE("Trace without module");
    HATN_CTX_TRACE("Trace with module",sample_module);

    BOOST_TEST_MESSAGE("Log with context log level=ANY");
    logCtx.setLogLevel(LogLevel::Any);

    HATN_CTX_FATAL(ec,"Fatal without module");
    HATN_CTX_FATAL(ec,"Fatal with module",sample_module);
    HATN_CTX_ERROR(ec,"Error without module");
    HATN_CTX_ERROR(ec,"Error with module",sample_module);

    HATN_CTX_WARN("Warn without module");
    HATN_CTX_WARN("Warn with module",sample_module);
    HATN_CTX_INFO("Info without module");
    HATN_CTX_INFO("Info with module",sample_module);
    HATN_CTX_DEBUG("Debug without module");
    HATN_CTX_DEBUG("Debug with module",sample_module);
    HATN_CTX_TRACE("Trace without module");
    HATN_CTX_TRACE("Trace with module",sample_module);

    auto r1=makeRecord("r1","hello");
    auto r2=makeRecord("r2",12345);
    auto r3=makeRecord("r3",Date::currentUtc());

    HATN_CTX_FATAL(ec,"Fatal without module");
    HATN_CTX_FATAL_RECORDS(ec,"Fatal with records without module",{"r1","hello"},{"r2",12345},{"r3",Date::currentUtc()});

    HATN_CTX_FATAL_RECORDS_M(ec,"Fatal with records with module",sample_module,r1,r2,r3);
    HATN_CTX_ERROR_RECORDS(ec,"Error with records without module",r1,r2,r3);
    HATN_CTX_ERROR_RECORDS_M(ec,"Error with records with module",sample_module,r1,r2,r3);

    HATN_CTX_WARN_RECORDS("Warn with records without module",r1,r2,r3);
    HATN_CTX_WARN_RECORDS_M("Warn with records with module",sample_module,r1,r2,r3);
    HATN_CTX_INFO_RECORDS("Info with records without module",r1,r2,r3);
    HATN_CTX_INFO_RECORDS_M("Info with records with module",sample_module,r1,r2,r3);
    HATN_CTX_DEBUG_RECORDS("Debug with records without module",r1,r2,r3);
    HATN_CTX_DEBUG_RECORDS_M("Debug with records with module",sample_module,r1,r2,r3);
    HATN_CTX_TRACE_RECORDS("Trace with records without module",r1,r2,r3);
    HATN_CTX_TRACE_RECORDS_M("Trace with records with module",sample_module,r1,r2,r3);

    HATN_CTX_CLOSE(Error{},"Closing context");
    HATN_CTX_CLOSE_API(Error{},"Closing context with API status");

    Error ec1{CommonError::NOT_IMPLEMENTED};
    HATN_CTX_CLOSE(ec1,"Closing context with error");
    HATN_CTX_CLOSE_API(ec1,"Closing context with API status and error");

    ctx->afterThreadProcessing();

    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_CASE(ScopeOperations)
{
    auto handler=std::make_shared<StreamLogger>();
    ContextLogger::init(std::static_pointer_cast<LoggerHandler>(handler));

    auto taskCtx=makeTaskContext<Context>();
    taskCtx->setName("test_task");
    auto& wrapper=taskCtx->get<Context>();
    auto logCtx=&wrapper;

    taskCtx->beforeThreadProcessing();

    logCtx->enterScope("scope1");

    HATN_CTX_INFO("No vars");

    logCtx->setGlobalVar("mapvar1","mapvar1 value");

    HATN_CTX_INFO("With global var");

    logCtx->pushStackVar("stackvar1","stackvar1 value");

    HATN_CTX_INFO("With global var and stack var");

    logCtx->unsetGlobalVar("mapvar1");

    HATN_CTX_INFO("With stack var when global var unset");

    logCtx->popStackVar();

    HATN_CTX_INFO("Without vars when stack var was popped out");

    logCtx->pushStackVar("stackvar1","stackvar1 value scope1");
    logCtx->setGlobalVar("mapvar1","mapvar1 value scope1");

    HATN_CTX_INFO("In scope1 before scope2");

    logCtx->enterScope("scope2");

    HATN_CTX_INFO("In scope 2 begin");

    logCtx->setGlobalVar("mapvar2","mapvar2 value scope2");

    HATN_CTX_INFO("In scope 2 with mapvar2");

    logCtx->pushStackVar("stackvar2","stackvar2 value scope2");

    HATN_CTX_INFO("In scope 2 end: with mapvar2 and stackvar2");

    logCtx->leaveScope(); // scope2

    HATN_CTX_INFO("In scope1 after scope2");

    logCtx->leaveScope(); // scope1

    HATN_CTX_INFO("Out of scopes");

    logCtx->reset();
    logCtx->enterScope("scope1");
    logCtx->pushStackVar("stackvar1","stackvar1 value scope1");
    logCtx->enterScope("scope2");
    logCtx->pushStackVar("stackvar2","stackvar2 value scope2");
    logCtx->setGlobalVar("mapvar2","mapvar2 value scope2");
    Error ec1{CommonError::TIMEOUT};
    logCtx->describeScopeError("something timeouted in scope2");
    logCtx->leaveScope();
    logCtx->describeScopeError("specify error in scope1");
    logCtx->leaveScope();
    HATN_CTX_ERROR(ec1,"Error out of scopes");
    HATN_CTX_CLOSE(ec1,"Closing context with error");

    taskCtx->afterThreadProcessing();

    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_CASE(ScopeMacros)
{
    auto handler=std::make_shared<StreamLogger>();
    ContextLogger::init(std::static_pointer_cast<LoggerHandler>(handler));

    auto taskCtx=makeTaskContext<Context>();
    taskCtx->setName("test_task");
    taskCtx->beforeThreadProcessing();

    {
        HATN_CTX_SCOPE("scope1")

        HATN_CTX_INFO("No vars");

        HATN_CTX_SET_VAR("mapvar1","mapvar1 value")
        HATN_CTX_INFO("With global var");

        HATN_CTX_SCOPE_PUSH("stackvar1","stackvar1 value")
        HATN_CTX_INFO("With global var and stack var");

        HATN_CTX_UNSET_VAR("mapvar1")
        HATN_CTX_INFO("With stack var when global var unset");

        HATN_CTX_SCOPE_POP()
        HATN_CTX_INFO("Without vars when stack var was popped out");

        HATN_CTX_SCOPE_PUSH("stackvar1","stackvar1 value")
        HATN_CTX_SET_VAR("mapvar1","mapvar1 value")

        HATN_CTX_INFO("In scope1 before scope2");

        {
            HATN_CTX_SCOPE("scope2")
            HATN_CTX_INFO("In scope 2 begin");

            HATN_CTX_SET_VAR("mapvar2","mapvar2 value scope2")
            HATN_CTX_INFO("In scope 2 with mapvar2");

            HATN_CTX_SCOPE_PUSH("stackvar2","stackvar2 value scope2")
            HATN_CTX_INFO("In scope 2 end: with mapvar2 and stackvar2");
        }

        HATN_CTX_INFO("In scope1 after scope2");
    }

    HATN_CTX_INFO("Out of scopes");

    HATN_CTX_RESET()
    Error ec1{CommonError::TIMEOUT};
    {
        HATN_CTX_SCOPE("scope1")
        HATN_CTX_SCOPE_PUSH("stackvar1","stackvar1 value")

        {
            HATN_CTX_SCOPE("scope2")
            HATN_CTX_SCOPE_PUSH("stackvar2","stackvar2 value")
            HATN_CTX_SET_VAR("mapvar2","mapvar2 value scope2")

            HATN_CTX_SCOPE_ERROR("something timeouted in scope2")
        }

        HATN_CTX_SCOPE_ERROR("specify error in scope1")
    }

    HATN_CTX_ERROR(ec1,"Error out of scopes");
    HATN_CTX_CLOSE(ec1,"Closing context with error");

    taskCtx->afterThreadProcessing();

    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_SUITE_END()
