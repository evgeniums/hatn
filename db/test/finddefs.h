/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file db/test/finddefs.h
 *
 *     Helpers for TestFindPlain.
 *
 */
/****************************************************************************/

#ifndef HATNDBTESTFINDDEFS_H
#define HATNDBTESTFINDDEFS_H

#include <boost/test/unit_test.hpp>

#include <hatn/common/datetime.h>
#include <hatn/common/daterange.h>

#include <hatn/test/multithreadfixture.h>

#include "hatn_test_config.h"
#include "initdbplugins.h"
#include "preparedb.h"

namespace {
size_t MaxValIdx=230;
size_t Count=MaxValIdx+1;
std::vector<size_t> CheckValueIndexes{10,20,30,150,233};
size_t Limit=0;
size_t VectorSize=5;
size_t VectorStep=5;
size_t IntervalWidth=7;
}

#endif // HATNDBTESTFINDDEFS_H
