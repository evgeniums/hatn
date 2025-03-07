/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file mq/test/testmq.cpp
  */

/****************************************************************************/

#include <boost/test/unit_test.hpp>

#include "hatn_test_config.h"

#include <hatn/mq/mq.h>

#include <hatn/mq/backgroundworker.h>

struct ContextBuilder
{
    auto makeContext() const
    {
        return HATN_COMMON_NAMESPACE::makeShared<HATN_COMMON_NAMESPACE::TaskContext>();
    }
};

struct Worker
{
    template <typename ContextT>
    void run(HATN_COMMON_NAMESPACE::SharedPtr<ContextT> ctx)
    {
    }
};

BOOST_AUTO_TEST_SUITE(TestBackgroundWorker)

BOOST_AUTO_TEST_CASE(Construct)
{
    HATN_MQ_NAMESPACE::BackgroundWorker<Worker,ContextBuilder> worker{};
    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_SUITE_END()
