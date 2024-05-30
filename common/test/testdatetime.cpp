#include <boost/test/unit_test.hpp>

#include <hatn/common/datetime.h>

HATN_USING
HATN_COMMON_USING

BOOST_AUTO_TEST_SUITE(TestDateTime)

BOOST_AUTO_TEST_CASE(TestDate)
{
    Date dt1;
    BOOST_REQUIRE(dt1.isNull());
    BOOST_REQUIRE(!dt1.isValid());

    auto dt2=Date::currentUtc();
    BOOST_TEST_MESSAGE(fmt::format("today UTC: {:04d}{:02d}{:02d}",dt2.year(),dt2.month(),dt2.day()));
    BOOST_REQUIRE(!dt2.isNull());
    BOOST_REQUIRE(dt2.isValid());

    dt2=Date::currentLocal();
    BOOST_TEST_MESSAGE(fmt::format("today local: {:04d}{:02d}{:02d}",dt2.year(),dt2.month(),dt2.day()));
    BOOST_REQUIRE(!dt2.isNull());
    BOOST_REQUIRE(dt2.isValid());

    auto dt3=Date{2024,03,31};
    BOOST_TEST_MESSAGE(fmt::format("fixed date: {:04d}{:02d}{:02d}",dt3.year(),dt3.month(),dt3.day()));
    BOOST_REQUIRE(!dt3.isNull());
    BOOST_REQUIRE(dt3.isValid());
    BOOST_CHECK_EQUAL(dt3.year(),2024);
    BOOST_CHECK_EQUAL(dt3.month(),3);
    BOOST_CHECK_EQUAL(dt3.day(),31);

    BOOST_CHECK_EQUAL(dt3.toString(Date::Format::Iso),"2024-03-31");
    BOOST_CHECK_EQUAL(dt3.toString(Date::Format::IsoSlash),"2024/03/31");
    BOOST_CHECK_EQUAL(dt3.toString(Date::Format::Number),"20240331");
    BOOST_CHECK_EQUAL(dt3.toString(Date::Format::UsDot),"03.31.2024");
    BOOST_CHECK_EQUAL(dt3.toString(Date::Format::EuropeDot),"31.03.2024");
    BOOST_CHECK_EQUAL(dt3.toString(Date::Format::UsSlash),"03/31/2024");
    BOOST_CHECK_EQUAL(dt3.toString(Date::Format::EuropeSlash),"31/03/2024");
    BOOST_CHECK_EQUAL(dt3.toString(Date::Format::UsShortDot),"03.31.24");
    BOOST_CHECK_EQUAL(dt3.toString(Date::Format::EuropeShortDot),"31.03.24");
    BOOST_CHECK_EQUAL(dt3.toString(Date::Format::UsShortSlash),"03/31/24");
    BOOST_CHECK_EQUAL(dt3.toString(Date::Format::EuropeShortSlash),"31/03/24");

    auto checkParse=[](const std::string& str, Date::Format format)
    {
        auto r=Date::parse(str,format);
        BOOST_REQUIRE(!r);
        auto dt=r.takeValue();
        BOOST_CHECK_EQUAL(dt.toString(format),str);
    };
    BOOST_TEST_CONTEXT("Iso"){checkParse("2024-03-31",Date::Format::Iso);}
    BOOST_TEST_CONTEXT("IsoSlash"){checkParse("2024/03/31",Date::Format::IsoSlash);}
    BOOST_TEST_CONTEXT("Number"){checkParse("20240331",Date::Format::Number);}
    BOOST_TEST_CONTEXT("UsDot"){checkParse("03.31.2024",Date::Format::UsDot);}
    BOOST_TEST_CONTEXT("EuropeDot"){checkParse("31.03.2024",Date::Format::EuropeDot);}
    BOOST_TEST_CONTEXT("UsSlash"){checkParse("03/31/2024",Date::Format::UsSlash);}
    BOOST_TEST_CONTEXT("EuropeSlash"){checkParse("31/03/2024",Date::Format::EuropeSlash);}
    BOOST_TEST_CONTEXT("UsShortDot"){checkParse("03.31.24",Date::Format::UsShortDot);}
    BOOST_TEST_CONTEXT("EuropeShortDot"){checkParse("31.03.24",Date::Format::EuropeShortDot);}
    BOOST_TEST_CONTEXT("UsShortSlash"){checkParse("03/31/24",Date::Format::UsShortSlash);}
    BOOST_TEST_CONTEXT("EuropeShortSlash"){checkParse("31/03/24",Date::Format::EuropeShortSlash);}

    auto r=Date::parse("20240331",Date::Format::Iso);
    BOOST_REQUIRE(r);

    auto ec=dt3.setYear(2025);
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(dt3.year(),2025);
    ec=dt3.setMonth(12);
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(dt3.month(),12);
    ec=dt3.setDay(15);
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(dt3.day(),15);
    ec=dt3.set(20240205);
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(dt3.year(),2024);
    BOOST_CHECK_EQUAL(dt3.month(),2);
    BOOST_CHECK_EQUAL(dt3.day(),5);

    ec=dt3.setYear(0);
    BOOST_CHECK(ec);
    ec=dt3.setMonth(15);
    BOOST_CHECK(ec);
    ec=dt3.setMonth(0);
    BOOST_CHECK(ec);
    ec=dt3.setDay(32);
    BOOST_CHECK(ec);
    ec=dt3.setDay(0);
    BOOST_CHECK(ec);
    ec=dt3.set(0);
    BOOST_CHECK(ec);
    ec=dt3.set(100);
    BOOST_CHECK(ec);
    ec=dt3.set(20351555);
    BOOST_CHECK(ec);

    BOOST_CHECK_EQUAL(dt3.year(),2024);
    BOOST_CHECK_EQUAL(dt3.month(),2);
    BOOST_CHECK_EQUAL(dt3.day(),5);

    dt3.reset();
    BOOST_CHECK_EQUAL(dt3.year(),0);
    BOOST_CHECK_EQUAL(dt3.month(),0);
    BOOST_CHECK_EQUAL(dt3.day(),0);
    BOOST_REQUIRE(dt3.isNull());
    BOOST_REQUIRE(!dt3.isValid());

    auto h1=[]{
        auto dt4=Date{2024,03,33};
        std::ignore=dt4;
    };
    BOOST_CHECK_THROW(h1(),std::runtime_error);
}

BOOST_AUTO_TEST_CASE(TestTime)
{
    Time t1;
    BOOST_REQUIRE(t1.isNull());
    BOOST_REQUIRE(!t1.isValid());

    auto t2=Time::currentUtc();
    BOOST_TEST_MESSAGE(fmt::format("current UTC: {:02d}:{:02d}:{:02d}.{:03d}",t2.hour(),t2.minute(),t2.second(),t2.millisecond()));
    BOOST_REQUIRE(!t2.isNull());
    BOOST_REQUIRE(t2.isValid());

    t2=Time::currentLocal();
    BOOST_TEST_MESSAGE(fmt::format("current local: {:02d}:{:02d}:{:02d}.{:03d}",t2.hour(),t2.minute(),t2.second(),t2.millisecond()));
    BOOST_REQUIRE(!t2.isNull());
    BOOST_REQUIRE(t2.isValid());

    auto t3=Time{11,23,17,254};
    BOOST_TEST_MESSAGE(t3.toString());
    BOOST_REQUIRE(!t3.isNull());
    BOOST_REQUIRE(t3.isValid());
    BOOST_CHECK_EQUAL(t3.hour(),11);
    BOOST_CHECK_EQUAL(t3.minute(),23);
    BOOST_CHECK_EQUAL(t3.second(),17);
    BOOST_CHECK_EQUAL(t3.millisecond(),254);

    auto ec=t3.setHour(22);
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(t3.hour(),22);
    ec=t3.setMinute(12);
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(t3.minute(),12);
    ec=t3.setSecond(59);
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(t3.second(),59);
    ec=t3.setMillisecond(118);
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(t3.millisecond(),118);
    ec=t3.set(193524209);
    BOOST_REQUIRE(!ec);
    BOOST_CHECK_EQUAL(t3.hour(),19);
    BOOST_CHECK_EQUAL(t3.minute(),35);
    BOOST_CHECK_EQUAL(t3.second(),24);
    BOOST_CHECK_EQUAL(t3.millisecond(),209);

    ec=t3.setHour(24);
    BOOST_CHECK(ec);
    ec=t3.setMinute(60);
    BOOST_CHECK(ec);
    ec=t3.setSecond(60);
    BOOST_CHECK(ec);
    ec=t3.setMillisecond(1000);
    BOOST_CHECK(ec);
    ec=t3.set(253524209);
    BOOST_CHECK(ec);

    BOOST_CHECK_EQUAL(t3.hour(),19);
    BOOST_CHECK_EQUAL(t3.minute(),35);
    BOOST_CHECK_EQUAL(t3.second(),24);
    BOOST_CHECK_EQUAL(t3.millisecond(),209);

    BOOST_CHECK_EQUAL(t3.toString(Time::FormatPrecision::Second),"19:35:24");
    BOOST_CHECK_EQUAL(t3.toString(Time::FormatPrecision::Millisecond),"19:35:24.209");
    BOOST_CHECK_EQUAL(t3.toString(Time::FormatPrecision::Minute),"19:35");
    BOOST_CHECK_EQUAL(t3.toString(Time::FormatPrecision::Second,true),"7:35:24 p.m.");
    BOOST_CHECK_EQUAL(t3.toString(Time::FormatPrecision::Millisecond,true),"7:35:24.209 p.m.");
    BOOST_CHECK_EQUAL(t3.toString(Time::FormatPrecision::Minute,true),"7:35 p.m.");

    t3.setHour(12);
    BOOST_CHECK_EQUAL(t3.toString(Time::FormatPrecision::Second,true),"12:35:24 p.m.");
    BOOST_CHECK_EQUAL(t3.toString(Time::FormatPrecision::Millisecond,true),"12:35:24.209 p.m.");
    BOOST_CHECK_EQUAL(t3.toString(Time::FormatPrecision::Minute,true),"12:35 p.m.");

    t3.setHour(0);
    BOOST_CHECK_EQUAL(t3.toString(Time::FormatPrecision::Second,true),"12:35:24 a.m.");
    BOOST_CHECK_EQUAL(t3.toString(Time::FormatPrecision::Millisecond,true),"12:35:24.209 a.m.");
    BOOST_CHECK_EQUAL(t3.toString(Time::FormatPrecision::Minute,true),"12:35 a.m.");

    auto checkParse=[](const std::string& str, Time::FormatPrecision precision, bool ampm)
    {
        auto r=Time::parse(str,precision,ampm);
        BOOST_REQUIRE(!r);
        auto t=r.takeValue();
        BOOST_CHECK_EQUAL(t.toString(precision,ampm),str);
    };
    BOOST_TEST_CONTEXT("Second"){checkParse("19:35:24",Time::FormatPrecision::Second,false);}
    BOOST_TEST_CONTEXT("Millisecond"){checkParse("19:35:24.209",Time::FormatPrecision::Millisecond,false);}
    BOOST_TEST_CONTEXT("Minute"){checkParse("19:35",Time::FormatPrecision::Minute,false);}
    BOOST_TEST_CONTEXT("Second a.m."){checkParse("7:35:24 a.m.",Time::FormatPrecision::Second,true);}
    BOOST_TEST_CONTEXT("Minute a.m."){checkParse("7:35 a.m.",Time::FormatPrecision::Minute,true);}
    BOOST_TEST_CONTEXT("Second a.m."){checkParse("7:35:24 p.m.",Time::FormatPrecision::Second,true);}
    BOOST_TEST_CONTEXT("Minute a.m."){checkParse("7:35 p.m.",Time::FormatPrecision::Minute,true);}
    BOOST_TEST_CONTEXT("Midnight a.m."){checkParse("12:35:24 a.m.",Time::FormatPrecision::Second,true);}
    BOOST_TEST_CONTEXT("Midnight a.m."){checkParse("12:35 a.m.",Time::FormatPrecision::Minute,true);}
    BOOST_TEST_CONTEXT("Noon p.m."){checkParse("12:35:24 p.m.",Time::FormatPrecision::Second,true);}
    BOOST_TEST_CONTEXT("Noon p.m."){checkParse("12:35 p.m.",Time::FormatPrecision::Minute,true);}

    t3.reset();
    BOOST_CHECK_EQUAL(t3.hour(),0);
    BOOST_CHECK_EQUAL(t3.minute(),0);
    BOOST_CHECK_EQUAL(t3.second(),0);
    BOOST_CHECK_EQUAL(t3.millisecond(),0);
    BOOST_REQUIRE(t3.isNull());
    BOOST_REQUIRE(!t3.isValid());

    auto h1=[]{
        auto t4=Time{24,03,33,900};
        std::ignore=t4;
    };
    BOOST_CHECK_THROW(h1(),std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()
