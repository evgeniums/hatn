/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file callgraph/test/teststacklog.cpp
  */

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include "hatn_test_config.h"

#include <hatn/callgraph/callgraph.h>
#include <hatn/callgraph/stacklogrecord.h>

HATN_COMMON_USING
HATN_CALLGRAPH_USING

BOOST_AUTO_TEST_SUITE(TestStackLog)

BOOST_AUTO_TEST_CASE(FormatLogValue)
{
    FmtAllocatedBufferChar buf;

    StackLogValue v1{int8_t{10}};
    serializeValue(buf,v1);
    auto str=fmtBufToString(buf);
    BOOST_TEST_MESSAGE(fmt::format("int8_t value: {}",str));
    BOOST_CHECK_EQUAL(str,"10");

    buf.clear();
    StackLogValue v2{"hello"};
    serializeValue(buf,v2);
    str=fmtBufToString(buf);
    BOOST_TEST_MESSAGE(fmt::format("string value: {}",str));
    BOOST_CHECK_EQUAL(str,"hello");

    buf.clear();
    StackLogValue v3{int8_t{-10}};
    serializeValue(buf,v3);
    str=fmtBufToString(buf);
    BOOST_TEST_MESSAGE(fmt::format("negative int8_t value: {}",str));
    BOOST_CHECK_EQUAL(str,"-10");

    buf.clear();
    StackLogValue v4{uint8_t{20}};
    serializeValue(buf,v4);
    str=fmtBufToString(buf);
    BOOST_TEST_MESSAGE(fmt::format("uint8_t value: {}",str));
    BOOST_CHECK_EQUAL(str,"20");

    buf.clear();
    StackLogValue v5{int16_t{-30}};
    serializeValue(buf,v5);
    str=fmtBufToString(buf);
    BOOST_TEST_MESSAGE(fmt::format("int16_t value: {}",str));
    BOOST_CHECK_EQUAL(str,"-30");

    buf.clear();
    StackLogValue v6{uint16_t{30}};
    serializeValue(buf,v6);
    str=fmtBufToString(buf);
    BOOST_TEST_MESSAGE(fmt::format("uint16_t value: {}",str));
    BOOST_CHECK_EQUAL(str,"30");

    buf.clear();
    StackLogValue v7{int32_t{-40}};
    serializeValue(buf,v7);
    str=fmtBufToString(buf);
    BOOST_TEST_MESSAGE(fmt::format("int32_t value: {}",str));
    BOOST_CHECK_EQUAL(str,"-40");

    buf.clear();
    StackLogValue v8{uint32_t{40}};
    serializeValue(buf,v8);
    str=fmtBufToString(buf);
    BOOST_TEST_MESSAGE(fmt::format("uint32_t value: {}",str));
    BOOST_CHECK_EQUAL(str,"40");

    buf.clear();
    StackLogValue v9{int64_t{-50}};
    serializeValue(buf,v9);
    str=fmtBufToString(buf);
    BOOST_TEST_MESSAGE(fmt::format("int64_t value: {}",str));
    BOOST_CHECK_EQUAL(str,"-50");

    buf.clear();
    StackLogValue v10{uint64_t{50}};
    serializeValue(buf,v10);
    str=fmtBufToString(buf);
    BOOST_TEST_MESSAGE(fmt::format("uint64_t value: {}",str));
    BOOST_CHECK_EQUAL(str,"50");

    buf.clear();
    StackLogValue v11{int64_t{0xffffffff5}};
    serializeValue(buf,v11);
    str=fmtBufToString(buf);
    BOOST_TEST_MESSAGE(fmt::format("int64_t long value: {}",str));
    BOOST_CHECK_EQUAL(str,"0xffffffff5");

    buf.clear();
    StackLogValue v12{-int64_t{0xffffffff5}};
    serializeValue(buf,v12);
    str=fmtBufToString(buf);
    BOOST_TEST_MESSAGE(fmt::format("int64_t long negative value: {}",str));
    BOOST_CHECK_EQUAL(str,"-0xffffffff5");

    buf.clear();
    StackLogValue v13{uint64_t{0xffffffff9}};
    serializeValue(buf,v13);
    str=fmtBufToString(buf);
    BOOST_TEST_MESSAGE(fmt::format("uint64_t long value: {}",str));
    BOOST_CHECK_EQUAL(str,"0xffffffff9");

    buf.clear();
    auto dt=DateTime::currentUtc();
    StackLogValue v14{dt};
    serializeValue(buf,v14);
    str=fmtBufToString(buf);
    BOOST_TEST_MESSAGE(fmt::format("DateTime value: {}",str));
    BOOST_CHECK_EQUAL(str,dt.toIsoString());

    buf.clear();
    auto d=Date::currentUtc();
    StackLogValue v15{d};
    serializeValue(buf,v15);
    str=fmtBufToString(buf);
    BOOST_TEST_MESSAGE(fmt::format("Date value: {}",str));
    BOOST_CHECK_EQUAL(str,d.toString(Date::Format::Number));

    buf.clear();
    auto t=Time::currentUtc();
    StackLogValue v16{t};
    serializeValue(buf,v16);
    str=fmtBufToString(buf);
    BOOST_TEST_MESSAGE(fmt::format("Time value: {}",str));
    BOOST_CHECK_EQUAL(str,t.toString());

    buf.clear();
    auto r=DateRange{d};
    StackLogValue v17{r};
    serializeValue(buf,v17);
    str=fmtBufToString(buf);
    BOOST_TEST_MESSAGE(fmt::format("DateRange value: {}",str));
    BOOST_CHECK_EQUAL(str,r.toString());
}

// BOOST_AUTO_TEST_CASE(StackLogSingleThread)
// {
// }

// BOOST_AUTO_TEST_CASE(StackLogMultiThread)
// {
// }

BOOST_AUTO_TEST_SUITE_END()
