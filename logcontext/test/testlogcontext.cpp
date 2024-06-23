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
    BOOST_CHECK_EQUAL(str,"hello");

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

BOOST_AUTO_TEST_CASE(LogContext)
{
    HATN_COMMON_NAMESPACE::ThreadLocalContext<Context> tlCtx;
    BOOST_CHECK(tlCtx.value()==nullptr);

    Context sl{nullptr};
    tlCtx.setValue(&sl);
    BOOST_CHECK(tlCtx.value()==&sl);

    tlCtx.reset();
    BOOST_CHECK(tlCtx.value()==nullptr);

    auto ctx=HATN_COMMON_NAMESPACE::makeTaskContext<ContextWrapper>();
    ctx->beforeThreadProcessing();
    BOOST_CHECK(tlCtx.value()!=nullptr);

    auto& wrapper=ctx->get<ContextWrapper>();
    auto wrappedCtx=wrapper.value();
    BOOST_CHECK(tlCtx.value()==wrappedCtx);

    ctx->afterThreadProcessing();
    BOOST_CHECK(tlCtx.value()==nullptr);
}

class TestLoggerHandler : public LoggerHandler
{
    public:

        void log(
            LogLevel level,
            const Context* ctx,
            HATN_COMMON_NAMESPACE::pmr::string msg,
            lib::string_view module=lib::string_view{},
            HATN_COMMON_NAMESPACE::pmr::vector<Record> records=HATN_COMMON_NAMESPACE::pmr::vector<Record>{}
            ) override
        {
#if __cplusplus < 201703L
            auto start=ctx->taskCtx()->startedAt();
#else
            auto start=std::chrono::floor<std::chrono::microseconds>(ctx->taskCtx()->startedAt());
#endif
            auto str=HATN_CTX_MSG_FORMAT("level={}, ctx={}, start={:%Y%m%dT%H:%M:%S}, elapsed={}, msg=\"{}\", module={}, records_count={}",
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
            HATN_COMMON_NAMESPACE::pmr::string msg,
            lib::string_view module=lib::string_view{},
            HATN_COMMON_NAMESPACE::pmr::vector<Record> records=HATN_COMMON_NAMESPACE::pmr::vector<Record>{}
        ) override
        {
#if __cplusplus < 201703L
            auto start=ctx->taskCtx()->startedAt();
#else
            auto start=std::chrono::floor<std::chrono::microseconds>(ctx->taskCtx()->startedAt());
#endif
            auto str=HATN_CTX_MSG_FORMAT("level={}, ctx={}, start={:%Y%m%dT%H:%M:%S}, elapsed={}, ec=\"{}\", msg=\"{}\", module={}, records_count={}",
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

BOOST_AUTO_TEST_CASE(Logger)
{
    B1<A1,A2> b1;
    std::ignore=b1;

    auto handler=std::make_shared<TestLoggerHandler>();
    ContextLogger::init(std::static_pointer_cast<LoggerHandler>(handler));

    auto ctx=HATN_COMMON_NAMESPACE::makeTaskContext<ContextWrapper>();

    auto& wrapper=ctx->get<ContextWrapper>();
    auto logCtx=wrapper.value();

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
    logCtx->setLogLevel(LogLevel::Any);

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
    auto r3=makeRecord("r3",HATN_COMMON_NAMESPACE::Date::currentUtc());

    HATN_CTX_FATAL_RECORDS(ec,"Fatal with records without module",r1,r2,r3);
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

    ctx->afterThreadProcessing();
}

BOOST_AUTO_TEST_SUITE_END()
