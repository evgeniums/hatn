#include <boost/test/unit_test.hpp>

#include <hatn/common/datetime.h>

HATN_USING
HATN_COMMON_USING

BOOST_AUTO_TEST_SUITE(TestDateTime)

BOOST_AUTO_TEST_CASE(TestDate)
{
    // null
    Date dt1;
    BOOST_REQUIRE(dt1.isNull());
    BOOST_REQUIRE(!dt1.isValid());

    // current
    auto dt2=Date::currentUtc();
    BOOST_TEST_MESSAGE(fmt::format("today UTC: {:04d}{:02d}{:02d}",dt2.year(),dt2.month(),dt2.day()));
    BOOST_REQUIRE(!dt2.isNull());
    BOOST_REQUIRE(dt2.isValid());

    dt2=Date::currentLocal();
    BOOST_TEST_MESSAGE(fmt::format("today local: {:04d}{:02d}{:02d}",dt2.year(),dt2.month(),dt2.day()));
    BOOST_REQUIRE(!dt2.isNull());
    BOOST_REQUIRE(dt2.isValid());

    // ctor
    auto dt3=Date{2024,03,31};
    BOOST_TEST_MESSAGE(fmt::format("fixed date: {:04d}{:02d}{:02d}",dt3.year(),dt3.month(),dt3.day()));
    BOOST_REQUIRE(!dt3.isNull());
    BOOST_REQUIRE(dt3.isValid());
    BOOST_CHECK_EQUAL(dt3.year(),2024);
    BOOST_CHECK_EQUAL(dt3.month(),3);
    BOOST_CHECK_EQUAL(dt3.day(),31);

    // to number
    BOOST_CHECK_EQUAL(dt3.toNumber(),20240331);

    // to string
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

    // parse
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

    // set
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

    // reset
    dt3.reset();
    BOOST_CHECK_EQUAL(dt3.year(),0);
    BOOST_CHECK_EQUAL(dt3.month(),0);
    BOOST_CHECK_EQUAL(dt3.day(),0);
    BOOST_REQUIRE(dt3.isNull());
    BOOST_REQUIRE(!dt3.isValid());

    // validate throw
    auto h1=[]{
        auto dt4=Date{2024,03,33};
        std::ignore=dt4;
    };
    BOOST_CHECK_THROW(h1(),std::runtime_error);

    // add days
    auto dt4=Date{2024,01,22};
    dt4.addDays(1);
    BOOST_CHECK_EQUAL(dt4.year(),2024);
    BOOST_CHECK_EQUAL(dt4.month(),1);
    BOOST_CHECK_EQUAL(dt4.day(),23);
    dt4.addDays(-1);
    BOOST_CHECK_EQUAL(dt4.year(),2024);
    BOOST_CHECK_EQUAL(dt4.month(),1);
    BOOST_CHECK_EQUAL(dt4.day(),22);
    dt4.addDays(10);
    BOOST_CHECK_EQUAL(dt4.year(),2024);
    BOOST_CHECK_EQUAL(dt4.month(),2);
    BOOST_CHECK_EQUAL(dt4.day(),1);
    dt4.addDays(-52);
    BOOST_CHECK_EQUAL(dt4.year(),2023);
    BOOST_CHECK_EQUAL(dt4.month(),12);
    BOOST_CHECK_EQUAL(static_cast<int>(dt4.day()),11);

    // leap/day of year/week
    auto dt5=Date{2024,01,22};
    BOOST_CHECK(!dt4.isLeapYear());
    BOOST_CHECK(dt5.isLeapYear());
    BOOST_CHECK_EQUAL(dt4.dayOfYear(),345);
    BOOST_CHECK_EQUAL(dt5.dayOfYear(),22);
    dt4.addDays(1);
    BOOST_CHECK_EQUAL(dt4.dayOfWeek(),2);
    BOOST_CHECK_EQUAL(dt5.dayOfWeek(),1);
    BOOST_CHECK_EQUAL(Date::daysInMonth(2024,1),31);
    BOOST_CHECK_EQUAL(Date::daysInMonth(2024,2),29);
    BOOST_CHECK_EQUAL(Date::daysInMonth(2023,2),28);

    // compare
    auto dt6=dt5;
    BOOST_CHECK(dt5==dt6);
    BOOST_CHECK(dt5>=dt6);
    BOOST_CHECK(dt5<=dt6);
    BOOST_CHECK(!(dt5<dt6));
    BOOST_CHECK(!(dt5>dt6));
    dt5.addDays(1);
    BOOST_CHECK(dt5!=dt6);
    BOOST_CHECK(dt5>dt6);
    BOOST_CHECK(dt6<dt5);
    BOOST_CHECK(dt5>=dt6);
    BOOST_CHECK(dt6<=dt5);
    dt5.addDays(-1);
    dt5.addDays(31);
    BOOST_CHECK(dt5!=dt6);
    BOOST_CHECK(dt5>dt6);
    BOOST_CHECK(dt6<dt5);
    BOOST_CHECK(dt5>=dt6);
    BOOST_CHECK(dt6<=dt5);
    dt5.addDays(-31);
    dt5.addDays(365);
    BOOST_CHECK(dt5!=dt6);
    BOOST_CHECK(dt5>dt6);
    BOOST_CHECK(dt6<dt5);
    BOOST_CHECK(dt5>=dt6);
    BOOST_CHECK(dt6<=dt5);
}

BOOST_AUTO_TEST_CASE(TestTime)
{
    // null
    Time t1;
    BOOST_REQUIRE(t1.isNull());
    BOOST_REQUIRE(!t1.isValid());

    // current
    auto t2=Time::currentUtc();
    BOOST_TEST_MESSAGE(fmt::format("current UTC: {:02d}:{:02d}:{:02d}.{:03d}",t2.hour(),t2.minute(),t2.second(),t2.millisecond()));
    BOOST_REQUIRE(!t2.isNull());
    BOOST_REQUIRE(t2.isValid());

    t2=Time::currentLocal();
    BOOST_TEST_MESSAGE(fmt::format("current local: {:02d}:{:02d}:{:02d}.{:03d}",t2.hour(),t2.minute(),t2.second(),t2.millisecond()));
    BOOST_REQUIRE(!t2.isNull());
    BOOST_REQUIRE(t2.isValid());

    // ctor
    auto t3=Time{11,23,17,254};
    BOOST_TEST_MESSAGE(t3.toString());
    BOOST_REQUIRE(!t3.isNull());
    BOOST_REQUIRE(t3.isValid());
    BOOST_CHECK_EQUAL(t3.hour(),11);
    BOOST_CHECK_EQUAL(t3.minute(),23);
    BOOST_CHECK_EQUAL(t3.second(),17);
    BOOST_CHECK_EQUAL(t3.millisecond(),254);

    // to number
    BOOST_CHECK_EQUAL(t3.toNumber(),112317254);

    // set
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

    // to string
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

    // parse
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

    // reset
    t3.reset();
    BOOST_CHECK_EQUAL(t3.hour(),0);
    BOOST_CHECK_EQUAL(t3.minute(),0);
    BOOST_CHECK_EQUAL(t3.second(),0);
    BOOST_CHECK_EQUAL(t3.millisecond(),0);
    BOOST_REQUIRE(t3.isNull());
    BOOST_REQUIRE(!t3.isValid());

    // validate throw
    auto h1=[]{
        auto t4=Time{24,03,33,900};
        std::ignore=t4;
    };
    BOOST_CHECK_THROW(h1(),std::runtime_error);

    // add
    auto t4=Time{11,23,17,254};
    BOOST_CHECK_EQUAL(t4.toNumber(),112317254);
    t4.addMilliseconds(100);
    BOOST_CHECK_EQUAL(t4.toNumber(),112317354);
    t4.addMilliseconds(700);
    BOOST_CHECK_EQUAL(t4.toNumber(),112318054);
    t4.addMilliseconds(5000);
    BOOST_CHECK_EQUAL(t4.toNumber(),112323054);
    t4.addMilliseconds(37256);
    BOOST_CHECK_EQUAL(t4.toNumber(),112400310);
    t4.addSeconds(60*38);
    BOOST_CHECK_EQUAL(t4.toNumber(),120200310);
    t4.addMinutes(60*14+5);
    BOOST_CHECK_EQUAL(t4.toNumber(),20700310);
    t4.addMinutes(-(60*14+5));
    BOOST_CHECK_EQUAL(t4.toNumber(),120200310);
    t4.addSeconds(-(60*38));
    BOOST_CHECK_EQUAL(t4.toNumber(),112400310);
    t4.addMilliseconds(-37256);
    BOOST_CHECK_EQUAL(t4.toNumber(),112323054);
    t4.addMilliseconds(-5000);
    BOOST_CHECK_EQUAL(t4.toNumber(),112318054);
    t4.addMilliseconds(-700);
    BOOST_CHECK_EQUAL(t4.toNumber(),112317354);
    t4.addMilliseconds(-100);
    BOOST_CHECK_EQUAL(t4.toNumber(),112317254);

    // compare
    auto t5=t4;
    BOOST_CHECK(t4==t5);
    BOOST_CHECK(t4>=t5);
    BOOST_CHECK(t4<=t5);
    BOOST_CHECK(!(t4<t5));
    BOOST_CHECK(!(t4>t5));
    t4.addSeconds(100);
    BOOST_CHECK(t4!=t5);
    BOOST_CHECK(t4>t5);
    BOOST_CHECK(t4>=t5);
    BOOST_CHECK(t5<t4);
    BOOST_CHECK(t5<=t4);
}

BOOST_AUTO_TEST_CASE(TestDateTime)
{
    // null
    DateTime dt1;
    BOOST_REQUIRE(dt1.isNull());
    BOOST_REQUIRE(!dt1.isValid());

    // current
    auto dt2=DateTime::currentUtc();
    BOOST_TEST_MESSAGE(fmt::format("datetime UTC with ms: {}",dt2.toIsoString()));
    BOOST_TEST_MESSAGE(fmt::format("datetime UTC without ms: {}",dt2.toIsoString(false)));
    BOOST_REQUIRE(!dt2.isNull());
    BOOST_REQUIRE(dt2.isValid());

    dt2=DateTime::currentLocal();
    BOOST_TEST_MESSAGE(fmt::format("datetime local: {}",dt2.toIsoString()));
    BOOST_REQUIRE(!dt2.isNull());
    BOOST_REQUIRE(dt2.isValid());

    auto msSinceEpoch=DateTime::millisecondsSinceEpoch();
    auto dt2_1ms=DateTime::fromEpochMs(msSinceEpoch);
    BOOST_TEST_MESSAGE(fmt::format("datetime UTC ms since epoch: {}",dt2_1ms.value().toIsoString()));
    BOOST_REQUIRE(!dt2_1ms.value().isNull());
    auto secsSinceEpoch=DateTime::secondsSinceEpoch();
    auto dt2_1=DateTime::fromEpoch(secsSinceEpoch);
    BOOST_TEST_MESSAGE(fmt::format("datetime UTC since epoch: {}",dt2_1.value().toIsoString()));
    BOOST_REQUIRE(!dt2_1.value().isNull());

    // ctor
    auto dt3=DateTime{Date{2024,03,31},Time{11,23,17,254},-5};
    BOOST_TEST_MESSAGE(fmt::format("fixed datetime 3: {}",dt3.toIsoString()));
    BOOST_REQUIRE(!dt3.isNull());
    BOOST_REQUIRE(dt3.isValid());
    BOOST_CHECK_EQUAL(dt3.date().year(),2024);
    BOOST_CHECK_EQUAL(dt3.date().month(),3);
    BOOST_CHECK_EQUAL(dt3.date().day(),31);
    BOOST_CHECK_EQUAL(dt3.time().hour(),11);
    BOOST_CHECK_EQUAL(dt3.time().minute(),23);
    BOOST_CHECK_EQUAL(dt3.time().second(),17);
    BOOST_CHECK_EQUAL(dt3.time().millisecond(),254);
    BOOST_CHECK_EQUAL(dt3.tz(),-5);

    // since epoch
    auto dt4=DateTime{Date{2024,03,31},Time{11,23,17,254},0};
    auto ep4=dt4.toEpoch();
    BOOST_TEST_MESSAGE(fmt::format("fixed datetime 4 UTC: {}, {}",dt4.toIsoString(),ep4));
    auto dt5=DateTime::fromEpoch(ep4);
    BOOST_TEST_MESSAGE(fmt::format("fixed datetime 5 UTC: {}, {}",dt5.value().toIsoString(),dt5.value().toEpoch()));
    auto dt5_1=DateTime::utcFromEpoch(ep4);
    BOOST_TEST_MESSAGE(fmt::format("fixed datetime 5_1 UTC: {}, {}",dt5_1.value().toIsoString(),dt5_1.value().toEpoch()));
    auto ep4Ms=dt4.toEpochMs();
    BOOST_TEST_MESSAGE(fmt::format("fixed datetime 4 UTC ms: {}, {}",dt4.toIsoString(),ep4Ms));
    auto dt4Ms1=DateTime::utcFromEpochMs(ep4Ms);
    BOOST_TEST_MESSAGE(fmt::format("fixed datetime 4_1 UTC ms: {}, {}",dt4Ms1.value().toIsoString(),dt4Ms1.value().toEpochMs()));
    BOOST_CHECK_EQUAL(dt4Ms1.value().toEpochMs(),ep4Ms);
    auto dt5Ms=DateTime::fromEpochMs(ep4Ms);
    BOOST_TEST_MESSAGE(fmt::format("fixed datetime 5 UTC ms: {}, {}",dt5Ms.value().toIsoString(),dt5Ms.value().toEpochMs()));

    uint64_t ep3Ms=dt3.toEpochMs();
    BOOST_TEST_MESSAGE(fmt::format("fixed datetime 3 tz -05: {}, {}",dt3.toIsoString(),ep3Ms));
    auto r6=DateTime::fromEpochMs(ep3Ms,dt3.tz());
    BOOST_REQUIRE(!r6);
    auto dt6=r6.takeValue();
    BOOST_REQUIRE(!dt6.isNull());
    BOOST_REQUIRE(dt6.isValid());
    BOOST_TEST_MESSAGE(fmt::format("fixed datetime 6: {}, {}",dt6.toIsoString(),dt6.toEpochMs()));
    BOOST_CHECK_EQUAL(dt6.date().year(),2024);
    BOOST_CHECK_EQUAL(dt6.date().month(),3);
    BOOST_CHECK_EQUAL(dt6.date().day(),31);
    BOOST_CHECK_EQUAL(dt6.time().hour(),11);
    BOOST_CHECK_EQUAL(dt6.time().minute(),23);
    BOOST_CHECK_EQUAL(dt6.time().second(),17);
    BOOST_CHECK_EQUAL(dt6.time().millisecond(),254);
    BOOST_CHECK_EQUAL(dt6.tz(),-5);

    // convert tz
    auto dt7Utc=dt6.toUtc();
    BOOST_REQUIRE(!dt7Utc.isNull());
    BOOST_REQUIRE(dt7Utc.isValid());
    BOOST_TEST_MESSAGE(fmt::format("converted datetime 7 UTC: {}, {}",dt7Utc.toIsoString(),dt7Utc.toEpochMs()));
    BOOST_CHECK_EQUAL(dt6.toEpochMs(),dt7Utc.toEpochMs());
    auto dt8Local=dt7Utc.toLocal();
    BOOST_TEST_MESSAGE(fmt::format("converted datetime 8 local: {}, {}",dt8Local.toIsoString(),dt8Local.toEpochMs()));
    auto localMs=dt8Local.toEpochMs();
    auto localS=dt8Local.toEpoch();
    auto dt8LocalMs=DateTime::localFromEpochMs(localMs);
    BOOST_TEST_MESSAGE(fmt::format("local datetime 8 ms: {}, {}",dt8LocalMs.value().toIsoString(),dt8LocalMs.value().toEpochMs()));
    BOOST_CHECK_EQUAL(dt8LocalMs.value().toEpochMs(),localMs);
    auto dt8LocalS=DateTime::localFromEpoch(localS);
    BOOST_TEST_MESSAGE(fmt::format("local datetime 8: {}, {}",dt8LocalS.value().toIsoString(),dt8LocalS.value().toEpoch()));
    BOOST_CHECK_EQUAL(dt8LocalS.value().toEpoch(),localS);

    // set tz
    dt3.setTz(-6);
    BOOST_TEST_MESSAGE(fmt::format("fixed datetime 3 with tz -06: {}, {}",dt3.toIsoString(),dt3.toEpochMs()));
    BOOST_CHECK_EQUAL(dt3.date().year(),2024);
    BOOST_CHECK_EQUAL(dt3.date().month(),3);
    BOOST_CHECK_EQUAL(dt3.date().day(),31);
    BOOST_CHECK_EQUAL(dt3.time().hour(),11);
    BOOST_CHECK_EQUAL(dt3.time().minute(),23);
    BOOST_CHECK_EQUAL(dt3.time().second(),17);
    BOOST_CHECK_EQUAL(dt3.time().millisecond(),254);
    BOOST_CHECK_EQUAL(dt3.tz(),-6);
    BOOST_CHECK_EQUAL(dt3.toEpochMs(),1711905797254);

    // local tz
    BOOST_TEST_MESSAGE(fmt::format("local tz: {}",DateTime::localTz()));

    // before/after
    auto dt9=DateTime{Date{2024,03,31},Time{11,23,17,254},-5};
    auto dt10=dt9;
    BOOST_CHECK(dt9.equal(dt10));
    BOOST_CHECK(!dt9.after(dt10));
    BOOST_CHECK(dt9.afterOrEqual(dt10));
    BOOST_CHECK(!dt9.before(dt10));
    BOOST_CHECK(dt9.beforeOrEqual(dt10));
    BOOST_CHECK(!dt10.after(dt9));
    BOOST_CHECK(dt10.afterOrEqual(dt9));
    BOOST_CHECK(!dt10.before(dt9));
    BOOST_CHECK(dt10.beforeOrEqual(dt9));
    BOOST_CHECK(dt9==dt10);
    BOOST_CHECK(!(dt9<dt10));
    BOOST_CHECK(dt9<=dt10);
    BOOST_CHECK(!(dt9>dt10));
    BOOST_CHECK(dt9>=dt10);
    dt10.addSeconds(100);
    BOOST_CHECK(!dt9.equal(dt10));
    BOOST_CHECK(!dt9.after(dt10));
    BOOST_CHECK(!dt9.afterOrEqual(dt10));
    BOOST_CHECK(dt9.before(dt10));
    BOOST_CHECK(dt9.beforeOrEqual(dt10));
    BOOST_CHECK(dt10.after(dt9));
    BOOST_CHECK(dt10.afterOrEqual(dt9));
    BOOST_CHECK(!dt10.before(dt9));
    BOOST_CHECK(!dt10.beforeOrEqual(dt9));
    BOOST_CHECK(dt9!=dt10);
    BOOST_CHECK(dt9<dt10);
    BOOST_CHECK(dt9<=dt10);
    BOOST_CHECK(!(dt9>dt10));
    BOOST_CHECK(!(dt9>=dt10));

    // add/diff
    BOOST_TEST_MESSAGE(fmt::format("dt9: {}, {}",dt9.toIsoString(),dt9.toEpochMs()));
    BOOST_TEST_MESSAGE(fmt::format("dt10: {}, {}",dt10.toIsoString(),dt10.toEpochMs()));
    BOOST_CHECK_EQUAL(dt9.diffSeconds(dt10),-100);
    BOOST_CHECK_EQUAL(dt10.diffSeconds(dt9),100);
    BOOST_CHECK_EQUAL(dt9.diffMilliseconds(dt10),-100000);
    BOOST_CHECK_EQUAL(dt10.diffMilliseconds(dt9),100000);
    auto diff1=dt9.diff(dt10);
    BOOST_CHECK_EQUAL(diff1.hours,0);
    BOOST_CHECK_EQUAL(diff1.minutes,-1);
    BOOST_CHECK_EQUAL(diff1.seconds,-40);
    dt10.addSeconds(-100);
    BOOST_CHECK(dt9==dt10);
    BOOST_CHECK_EQUAL(dt9.diffSeconds(dt10),0);
    BOOST_CHECK_EQUAL(dt10.diffSeconds(dt9),0);
    BOOST_CHECK_EQUAL(dt9.diffMilliseconds(dt10),0);
    BOOST_CHECK_EQUAL(dt10.diffMilliseconds(dt9),0);
    dt10.addMilliseconds(1010);
    BOOST_CHECK_EQUAL(dt9.diffSeconds(dt10),-1);
    BOOST_CHECK_EQUAL(dt10.diffSeconds(dt9),1);
    BOOST_CHECK_EQUAL(dt9.diffMilliseconds(dt10),-1010);
    BOOST_CHECK_EQUAL(dt10.diffMilliseconds(dt9),1010);
    dt10.addMilliseconds(-1010);
    dt10.addMinutes(3);
    BOOST_CHECK_EQUAL(dt9.diffSeconds(dt10),-180);
    BOOST_CHECK_EQUAL(dt10.diffSeconds(dt9),180);
    BOOST_CHECK_EQUAL(dt9.diffMilliseconds(dt10),-180000);
    BOOST_CHECK_EQUAL(dt10.diffMilliseconds(dt9),180000);
    auto diff2=dt9.diff(dt10);
    BOOST_CHECK_EQUAL(diff2.hours,0);
    BOOST_CHECK_EQUAL(diff2.minutes,-3);
    BOOST_CHECK_EQUAL(diff2.seconds,0);
    auto diff2_1=dt10.diff(dt9);
    BOOST_CHECK_EQUAL(diff2_1.hours,0);
    BOOST_CHECK_EQUAL(diff2_1.minutes,3);
    BOOST_CHECK_EQUAL(diff2_1.seconds,0);
    dt10.addMinutes(-3);
    dt10.addHours(2);
    BOOST_CHECK_EQUAL(dt9.diffSeconds(dt10),-7200);
    BOOST_CHECK_EQUAL(dt10.diffSeconds(dt9),7200);
    BOOST_CHECK_EQUAL(dt9.diffMilliseconds(dt10),-7200000);
    BOOST_CHECK_EQUAL(dt10.diffMilliseconds(dt9),7200000);
    diff2=dt9.diff(dt10);
    BOOST_CHECK_EQUAL(diff2.hours,-2);
    BOOST_CHECK_EQUAL(diff2.minutes,0);
    BOOST_CHECK_EQUAL(diff2.seconds,0);
    diff2_1=dt10.diff(dt9);
    BOOST_CHECK_EQUAL(diff2_1.hours,2);
    BOOST_CHECK_EQUAL(diff2_1.minutes,0);
    BOOST_CHECK_EQUAL(diff2_1.seconds,0);
    dt10.addHours(-2);
    dt10.addSeconds(7432);
    BOOST_CHECK_EQUAL(dt9.diffSeconds(dt10),-7432);
    BOOST_CHECK_EQUAL(dt10.diffSeconds(dt9),7432);
    BOOST_CHECK_EQUAL(dt9.diffMilliseconds(dt10),-7432000);
    BOOST_CHECK_EQUAL(dt10.diffMilliseconds(dt9),7432000);
    diff2=dt9.diff(dt10);
    BOOST_CHECK_EQUAL(diff2.hours,-2);
    BOOST_CHECK_EQUAL(diff2.minutes,-3);
    BOOST_CHECK_EQUAL(diff2.seconds,-52);
    diff2_1=dt10.diff(dt9);
    BOOST_CHECK_EQUAL(diff2_1.hours,2);
    BOOST_CHECK_EQUAL(diff2_1.minutes,3);
    BOOST_CHECK_EQUAL(diff2_1.seconds,52);
    dt10.addSeconds(-7432);
    dt10.addDays(5);
    BOOST_CHECK_EQUAL(dt9.diffSeconds(dt10),-432000);
    BOOST_CHECK_EQUAL(dt10.diffSeconds(dt9),432000);
    BOOST_CHECK_EQUAL(dt9.diffMilliseconds(dt10),-432000000);
    BOOST_CHECK_EQUAL(dt10.diffMilliseconds(dt9),432000000);
    diff2=dt9.diff(dt10);
    BOOST_CHECK_EQUAL(diff2.hours,-120);
    BOOST_CHECK_EQUAL(diff2.minutes,0);
    BOOST_CHECK_EQUAL(diff2.seconds,0);
    diff2_1=dt10.diff(dt9);
    BOOST_CHECK_EQUAL(diff2_1.hours,120);
    BOOST_CHECK_EQUAL(diff2_1.minutes,0);
    BOOST_CHECK_EQUAL(diff2_1.seconds,0);

    // parsing
    auto p1Str="2023-10-05T21:35:47.123Z";
    auto p1=DateTime::parseIsoString(p1Str);
    BOOST_CHECK(!p1);
    BOOST_REQUIRE(p1.value().isValid());
    BOOST_TEST_MESSAGE(fmt::format("parse 1: {} to {}",p1Str,p1.value().toIsoString()));
    BOOST_CHECK_EQUAL("2023-10-05T21:35:47.123Z",p1.value().toIsoString());

    auto p2Str="20231005T21:35:47.123Z";
    auto p2=DateTime::parseIsoString(p2Str);
    BOOST_CHECK(!p2);
    BOOST_REQUIRE(p2.value().isValid());
    BOOST_TEST_MESSAGE(fmt::format("parse 2: {} to {}",p2Str,p2.value().toIsoString()));
    BOOST_CHECK_EQUAL("2023-10-05T21:35:47.123Z",p2.value().toIsoString());

    auto p3Str="2023-10-05T21:35:47Z";
    auto p3=DateTime::parseIsoString(p3Str);
    BOOST_CHECK(!p3);
    BOOST_REQUIRE(p3.value().isValid());
    BOOST_TEST_MESSAGE(fmt::format("parse 3: {} to {}",p3Str,p3.value().toIsoString()));
    BOOST_CHECK_EQUAL("2023-10-05T21:35:47Z",p3.value().toIsoString());

    auto p4Str="20231005T21:35:47Z";
    auto p4=DateTime::parseIsoString(p4Str);
    BOOST_CHECK(!p4);
    BOOST_REQUIRE(p4.value().isValid());
    BOOST_TEST_MESSAGE(fmt::format("parse 4: {} to {}",p4Str,p4.value().toIsoString()));
    BOOST_CHECK_EQUAL("2023-10-05T21:35:47Z",p4.value().toIsoString());

    auto p5Str="20231005T21:35:47.15Z";
    auto p5=DateTime::parseIsoString(p5Str);
    BOOST_CHECK(!p5);
    BOOST_REQUIRE(p5.value().isValid());
    BOOST_TEST_MESSAGE(fmt::format("parse 5: {} to {}",p5Str,p5.value().toIsoString()));
    BOOST_CHECK_EQUAL("2023-10-05T21:35:47.150Z",p5.value().toIsoString());

    auto p6Str="20231005T21:35:47.156789Z";
    auto p6=DateTime::parseIsoString(p6Str);
    BOOST_CHECK(!p6);
    BOOST_REQUIRE(p6.value().isValid());
    BOOST_TEST_MESSAGE(fmt::format("parse 6: {} to {}",p6Str,p6.value().toIsoString()));
    BOOST_CHECK_EQUAL("2023-10-05T21:35:47.156Z",p6.value().toIsoString());

    auto p7Str="2023-10-05T21:35:47.123+03:00";
    auto p7=DateTime::parseIsoString(p7Str);
    BOOST_CHECK(!p7);
    BOOST_REQUIRE(p7.value().isValid());
    BOOST_TEST_MESSAGE(fmt::format("parse 7: {} to {}",p7Str,p7.value().toIsoString()));
    BOOST_CHECK_EQUAL("2023-10-05T21:35:47.123+03:00",p7.value().toIsoString());

    auto p8Str="20231005T21:35:47.123-03:00";
    auto p8=DateTime::parseIsoString(p8Str);
    BOOST_CHECK(!p8);
    BOOST_REQUIRE(p8.value().isValid());
    BOOST_TEST_MESSAGE(fmt::format("parse 8: {} to {}",p8Str,p8.value().toIsoString()));
    BOOST_CHECK_EQUAL("2023-10-05T21:35:47.123-03:00",p8.value().toIsoString());

    auto p9Str="2023-10-05T21:35:47+03:00";
    auto p9=DateTime::parseIsoString(p9Str);
    BOOST_CHECK(!p9);
    BOOST_REQUIRE(p9.value().isValid());
    BOOST_TEST_MESSAGE(fmt::format("parse 9: {} to {}",p9Str,p9.value().toIsoString()));
    BOOST_CHECK_EQUAL("2023-10-05T21:35:47+03:00",p9.value().toIsoString());

    auto p10Str="20231005T21:35:47-03:00";
    auto p10=DateTime::parseIsoString(p10Str);
    BOOST_CHECK(!p10);
    BOOST_REQUIRE(p10.value().isValid());
    BOOST_TEST_MESSAGE(fmt::format("parse 10: {} to {}",p10Str,p10.value().toIsoString()));
    BOOST_CHECK_EQUAL("2023-10-05T21:35:47-03:00",p10.value().toIsoString());

    auto p11Str="202310005T21:35:47-03:00";
    auto p11=DateTime::parseIsoString(p11Str);
    BOOST_CHECK(p11);
}

BOOST_AUTO_TEST_CASE(TestDateRange)
{
    DateRange r0;
    BOOST_CHECK(!r0.isValid());

    auto dt1=Date{2023,10,25};
    auto dt2=Date{2024,03,31};

    auto r1=DateRange::dateToRange(dt1,DateRange::Type::Year);
    BOOST_REQUIRE(r1.isValid());
    BOOST_CHECK_EQUAL(static_cast<int>(r1.type()),static_cast<int>(DateRange::Type::Year));
    BOOST_CHECK_EQUAL(r1.year(),2023);
    BOOST_CHECK_EQUAL(r1.range(),1);
    BOOST_CHECK_EQUAL(r1.value(),2023001);
    BOOST_CHECK(r1.contains(dt1));
    BOOST_CHECK(!r1.contains(dt2));
    auto bd=Date{2023,1,1};
    BOOST_CHECK_EQUAL(r1.begin().toNumber(),bd.toNumber());
    auto ed=Date{2023,12,31};
    BOOST_CHECK_EQUAL(r1.end().toNumber(),ed.toNumber());

    auto r2=DateRange::dateToRange(dt1,DateRange::Type::HalfYear);
    BOOST_REQUIRE(r2.isValid());
    BOOST_CHECK_EQUAL(static_cast<int>(r2.type()),static_cast<int>(DateRange::Type::HalfYear));
    BOOST_CHECK_EQUAL(r2.year(),2023);
    BOOST_CHECK_EQUAL(r2.range(),2);
    BOOST_CHECK_EQUAL(r2.value(),12023002);
    r2=DateRange::dateToRange(dt2,DateRange::Type::HalfYear);
    BOOST_REQUIRE(r2.isValid());
    BOOST_CHECK_EQUAL(static_cast<int>(r2.type()),static_cast<int>(DateRange::Type::HalfYear));
    BOOST_CHECK_EQUAL(r2.year(),2024);
    BOOST_CHECK_EQUAL(r2.range(),1);
    BOOST_CHECK_EQUAL(r2.value(),12024001);
    BOOST_CHECK(r2.contains(dt2));
    BOOST_CHECK(!r2.contains(dt1));
    bd=Date{2024,1,1};
    BOOST_CHECK_EQUAL(r2.begin().toNumber(),bd.toNumber());
    ed=Date{2024,06,30};
    BOOST_CHECK_EQUAL(r2.end().toNumber(),ed.toNumber());

    auto r3=DateRange::dateToRange(dt1,DateRange::Type::Quarter);
    BOOST_REQUIRE(r3.isValid());
    BOOST_CHECK_EQUAL(static_cast<int>(r3.type()),static_cast<int>(DateRange::Type::Quarter));
    BOOST_CHECK_EQUAL(r3.year(),2023);
    BOOST_CHECK_EQUAL(r3.range(),4);
    BOOST_CHECK_EQUAL(r3.value(),22023004);
    BOOST_CHECK(r3.contains(dt1));
    BOOST_CHECK(!r3.contains(dt2));
    bd=Date{2023,10,1};
    BOOST_CHECK_EQUAL(r3.begin().toNumber(),bd.toNumber());
    ed=Date{2023,12,31};
    BOOST_CHECK_EQUAL(r3.end().toNumber(),ed.toNumber());
    r2=DateRange::dateToRange(dt2,DateRange::Type::Quarter);
    BOOST_REQUIRE(r3.isValid());
    BOOST_CHECK_EQUAL(static_cast<int>(r2.type()),static_cast<int>(DateRange::Type::Quarter));
    BOOST_CHECK_EQUAL(r2.year(),2024);
    BOOST_CHECK_EQUAL(r2.range(),1);
    BOOST_CHECK_EQUAL(r2.value(),22024001);
    BOOST_CHECK(!r2.contains(dt1));
    BOOST_CHECK(r2.contains(dt2));
    bd=Date{2024,1,1};
    BOOST_CHECK_EQUAL(r2.begin().toNumber(),bd.toNumber());
    ed=Date{2024,3,31};
    BOOST_CHECK_EQUAL(r2.end().toNumber(),ed.toNumber());
    dt2.setMonth(5);
    r2=DateRange::dateToRange(dt2,DateRange::Type::Quarter);
    BOOST_CHECK_EQUAL(r2.range(),2);
    BOOST_CHECK(!r2.contains(dt1));
    BOOST_CHECK(r2.contains(dt2));
    bd=Date{2024,4,1};
    BOOST_CHECK_EQUAL(r2.begin().toNumber(),bd.toNumber());
    ed=Date{2024,6,30};
    BOOST_CHECK_EQUAL(r2.end().toNumber(),ed.toNumber());
    dt2.setMonth(8);
    r2=DateRange::dateToRange(dt2,DateRange::Type::Quarter);
    BOOST_CHECK_EQUAL(r2.range(),3);
    BOOST_CHECK(!r2.contains(dt1));
    BOOST_CHECK(r2.contains(dt2));
    bd=Date{2024,7,1};
    BOOST_CHECK_EQUAL(r2.begin().toNumber(),bd.toNumber());
    ed=Date{2024,9,30};
    BOOST_CHECK_EQUAL(r2.end().toNumber(),ed.toNumber());

    dt2.setDay(15);
    dt2.setMonth(1);
    r2=DateRange::dateToRange(dt2,DateRange::Type::Month);
    BOOST_CHECK_EQUAL(static_cast<int>(r2.type()),static_cast<int>(DateRange::Type::Month));
    BOOST_CHECK_EQUAL(r2.year(),2024);
    BOOST_CHECK_EQUAL(r2.range(),1);
    BOOST_CHECK_EQUAL(r2.value(),32024001);
    BOOST_CHECK(!r2.contains(dt1));
    BOOST_CHECK(r2.contains(dt2));
    bd=Date{2024,1,1};
    BOOST_CHECK_EQUAL(r2.begin().toNumber(),bd.toNumber());
    ed=Date{2024,1,31};
    BOOST_CHECK_EQUAL(r2.end().toNumber(),ed.toNumber());

    dt2.setMonth(2);
    r2=DateRange::dateToRange(dt2,DateRange::Type::Month);
    BOOST_CHECK_EQUAL(r2.range(),2);
    BOOST_CHECK(!r2.contains(dt1));
    BOOST_CHECK(r2.contains(dt2));
    bd=Date{2024,2,1};
    BOOST_CHECK_EQUAL(r2.begin().toNumber(),bd.toNumber());
    ed=Date{2024,2,29};
    BOOST_CHECK_EQUAL(r2.end().toNumber(),ed.toNumber());

    dt2.setMonth(3);
    r2=DateRange::dateToRange(dt2,DateRange::Type::Month);
    BOOST_CHECK_EQUAL(r2.range(),3);
    BOOST_CHECK(!r2.contains(dt1));
    BOOST_CHECK(r2.contains(dt2));
    bd=Date{2024,3,1};
    BOOST_CHECK_EQUAL(r2.begin().toNumber(),bd.toNumber());
    ed=Date{2024,3,31};
    BOOST_CHECK_EQUAL(r2.end().toNumber(),ed.toNumber());

    dt2.setMonth(4);
    r2=DateRange::dateToRange(dt2,DateRange::Type::Month);
    BOOST_CHECK_EQUAL(r2.range(),4);
    BOOST_CHECK(!r2.contains(dt1));
    BOOST_CHECK(r2.contains(dt2));
    bd=Date{2024,4,1};
    BOOST_CHECK_EQUAL(r2.begin().toNumber(),bd.toNumber());
    ed=Date{2024,4,30};
    BOOST_CHECK_EQUAL(r2.end().toNumber(),ed.toNumber());

    dt2.setMonth(5);
    r2=DateRange::dateToRange(dt2,DateRange::Type::Month);
    BOOST_CHECK_EQUAL(r2.range(),5);
    BOOST_CHECK(!r2.contains(dt1));
    BOOST_CHECK(r2.contains(dt2));
    bd=Date{2024,5,1};
    BOOST_CHECK_EQUAL(r2.begin().toNumber(),bd.toNumber());
    ed=Date{2024,5,31};
    BOOST_CHECK_EQUAL(r2.end().toNumber(),ed.toNumber());

    dt2.setMonth(6);
    r2=DateRange::dateToRange(dt2,DateRange::Type::Month);
    BOOST_CHECK_EQUAL(r2.range(),6);
    BOOST_CHECK(!r2.contains(dt1));
    BOOST_CHECK(r2.contains(dt2));
    bd=Date{2024,6,1};
    BOOST_CHECK_EQUAL(r2.begin().toNumber(),bd.toNumber());
    ed=Date{2024,6,30};
    BOOST_CHECK_EQUAL(r2.end().toNumber(),ed.toNumber());

    dt2.setMonth(7);
    r2=DateRange::dateToRange(dt2,DateRange::Type::Month);
    BOOST_CHECK_EQUAL(r2.range(),7);
    BOOST_CHECK(!r2.contains(dt1));
    BOOST_CHECK(r2.contains(dt2));
    bd=Date{2024,7,1};
    BOOST_CHECK_EQUAL(r2.begin().toNumber(),bd.toNumber());
    ed=Date{2024,7,31};
    BOOST_CHECK_EQUAL(r2.end().toNumber(),ed.toNumber());

    dt2.setMonth(8);
    r2=DateRange::dateToRange(dt2,DateRange::Type::Month);
    BOOST_CHECK_EQUAL(r2.range(),8);
    BOOST_CHECK(!r2.contains(dt1));
    BOOST_CHECK(r2.contains(dt2));
    bd=Date{2024,8,1};
    BOOST_CHECK_EQUAL(r2.begin().toNumber(),bd.toNumber());
    ed=Date{2024,8,31};
    BOOST_CHECK_EQUAL(r2.end().toNumber(),ed.toNumber());

    dt2.setMonth(9);
    r2=DateRange::dateToRange(dt2,DateRange::Type::Month);
    BOOST_CHECK_EQUAL(r2.range(),9);
    BOOST_CHECK(!r2.contains(dt1));
    BOOST_CHECK(r2.contains(dt2));
    bd=Date{2024,9,1};
    BOOST_CHECK_EQUAL(r2.begin().toNumber(),bd.toNumber());
    ed=Date{2024,9,30};
    BOOST_CHECK_EQUAL(r2.end().toNumber(),ed.toNumber());

    dt2.setMonth(10);
    r2=DateRange::dateToRange(dt2,DateRange::Type::Month);
    BOOST_CHECK_EQUAL(r2.range(),10);
    BOOST_CHECK(!r2.contains(dt1));
    BOOST_CHECK(r2.contains(dt2));
    bd=Date{2024,10,1};
    BOOST_CHECK_EQUAL(r2.begin().toNumber(),bd.toNumber());
    ed=Date{2024,10,31};
    BOOST_CHECK_EQUAL(r2.end().toNumber(),ed.toNumber());

    dt2.setMonth(11);
    r2=DateRange::dateToRange(dt2,DateRange::Type::Month);
    BOOST_CHECK_EQUAL(r2.range(),11);
    BOOST_CHECK(!r2.contains(dt1));
    BOOST_CHECK(r2.contains(dt2));
    bd=Date{2024,11,1};
    BOOST_CHECK_EQUAL(r2.begin().toNumber(),bd.toNumber());
    ed=Date{2024,11,30};
    BOOST_CHECK_EQUAL(r2.end().toNumber(),ed.toNumber());

    dt2.setMonth(12);
    r2=DateRange::dateToRange(dt2,DateRange::Type::Month);
    BOOST_CHECK_EQUAL(r2.range(),12);
    BOOST_CHECK(!r2.contains(dt1));
    BOOST_CHECK(r2.contains(dt2));
    bd=Date{2024,12,1};
    BOOST_CHECK_EQUAL(r2.begin().toNumber(),bd.toNumber());
    ed=Date{2024,12,31};
    BOOST_CHECK_EQUAL(r2.end().toNumber(),ed.toNumber());

    dt2.setDay(15);
    dt2.setMonth(1);
    r2=DateRange::dateToRange(dt2,DateRange::Type::Week);
    BOOST_CHECK_EQUAL(static_cast<int>(r2.type()),static_cast<int>(DateRange::Type::Week));
    BOOST_CHECK_EQUAL(r2.year(),2024);
    BOOST_CHECK_EQUAL(r2.range(),3);
    BOOST_CHECK_EQUAL(r2.value(),42024003);
    BOOST_CHECK(!r2.contains(dt1));
    BOOST_CHECK(r2.contains(dt2));
    bd=Date{2024,1,15};
    BOOST_CHECK_EQUAL(bd.weekNumber(),3);
    BOOST_CHECK_EQUAL(r2.begin().toNumber(),bd.toNumber());
    bd=Date{2024,1,14};
    BOOST_CHECK_EQUAL(bd.weekNumber(),2);
    ed=Date{2024,1,21};
    BOOST_CHECK_EQUAL(ed.weekNumber(),3);
    BOOST_CHECK_EQUAL(r2.end().toNumber(),ed.toNumber());
    ed=Date{2024,1,22};
    BOOST_CHECK_EQUAL(ed.weekNumber(),4);

    dt2.setDay(2);
    dt2.setMonth(2);
    r2=DateRange::dateToRange(dt2,DateRange::Type::Day);
    BOOST_CHECK_EQUAL(static_cast<int>(r2.type()),static_cast<int>(DateRange::Type::Day));
    BOOST_CHECK_EQUAL(r2.year(),2024);
    BOOST_CHECK_EQUAL(r2.range(),33);
    BOOST_CHECK_EQUAL(r2.value(),52024033);
    BOOST_CHECK(!r2.contains(dt1));
    BOOST_CHECK(r2.contains(dt2));
    bd=Date{2024,2,2};
    BOOST_CHECK_EQUAL(r2.begin().toNumber(),bd.toNumber());
    ed=Date{2024,2,2};
    BOOST_CHECK_EQUAL(r2.end().toNumber(),ed.toNumber());
    auto d=Date{2024,2,1};
    BOOST_CHECK(!r2.contains(d));
    d=Date{2024,2,2};
    BOOST_CHECK(r2.contains(d));
    BOOST_CHECK_EQUAL(d.dayOfYear(),33);
    d=Date{2024,2,3};
    BOOST_CHECK(!r2.contains(d));

    auto bdt=DateTime(Date(2024,2,2),Time(0,0,1));
    BOOST_CHECK(r2.beginDateTime()==bdt);
    auto edt=DateTime(Date(2024,2,2),Time(23,59,59));
    BOOST_CHECK(r2.endDateTime()==edt);
    BOOST_CHECK(r2.contains(bdt));
    BOOST_CHECK(r2.contains(edt));
    bdt.addHours(5);
    BOOST_CHECK(r2.contains(bdt));
    edt.addHours(5);
    BOOST_CHECK(!r2.contains(edt));
}

BOOST_AUTO_TEST_SUITE_END()
