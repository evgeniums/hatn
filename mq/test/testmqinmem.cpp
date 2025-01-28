/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file mq/test/testmqinmem.cpp
  */

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include "hatn_test_config.h"

#include <hatn/dataunit/objectid.h>
#include <hatn/mq/mqinmem.h>
#include <hatn/mq/mq.h>

#include <hatn/test/multithreadfixture.h>

namespace mq=HATN_MQ_NAMESPACE;
namespace du=HATN_DATAUNIT_NAMESPACE;
HATN_TEST_USING

struct Message
{
    du::ObjectId id;
    du::ObjectId refId;
};

struct Update
{
};

struct MessageTraits : public mq::MessageTraitsBase<Message,Update>
{
    static const auto* messageId(const MessageType& msg)
    {
        return &msg.id;
    }

    static const auto* messageRefId(const MessageType& msg)
    {
        return &msg.refId;
    }
};

using InmemTraits=mq::MqInmemTraits<MessageTraits>;

BOOST_AUTO_TEST_SUITE(TestMqinmem)

BOOST_FIXTURE_TEST_CASE(TraitsInMem,MultiThreadFixture)
{
    InmemTraits traits;

    Message msg1;
    msg1.id.generate();
    msg1.refId.generate();
    Message msg2;
    msg2.id.generate();
    msg2.refId.generate();

    BOOST_CHECK(msg1.id==*traits.messageId(msg1));
    BOOST_CHECK(msg1.refId==*traits.messageRefId(msg1));
    BOOST_CHECK(msg1.id!=*traits.messageId(msg2));
    BOOST_CHECK(msg1.refId!=*traits.messageRefId(msg2));
}

BOOST_FIXTURE_TEST_CASE(MqInmem,MultiThreadFixture)
{
    mq::Mq<InmemTraits> mq1;

    Message msg1;
    msg1.id.generate();
    msg1.refId.generate();
    Message msg2;
    msg2.id.generate();
    msg2.refId.generate();

    BOOST_CHECK(msg1.id==*InmemTraits::messageId(msg1));
    BOOST_CHECK(msg1.refId==*InmemTraits::messageRefId(msg1));
    BOOST_CHECK(msg1.id!=*InmemTraits::messageId(msg2));
    BOOST_CHECK(msg1.refId!=*InmemTraits::messageRefId(msg2));
}

BOOST_AUTO_TEST_SUITE_END()
