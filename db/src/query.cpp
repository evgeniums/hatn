/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/** @file db/query.cpp
  *
  *   Definition of db query helpers.
  *
  */

/****************************************************************************/

#include <hatn/db/query.h>
#include <hatn/db/indexquery.h>

HATN_DB_NAMESPACE_BEGIN

/*********************** IndexQuery **************************/

//---------------------------------------------------------------

size_t IndexQuery::DefaultLimit=100;


void a()
{
    query::Interval<query::NullT> interval{query::Null,query::IntervalType::Open,query::Null,query::IntervalType::Open};
    query::Operand val{interval};

    query::Field f(nullptr,query::nin,interval);
}

//---------------------------------------------------------------

HATN_DB_NAMESPACE_END
