/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file db/find.h
  *
  */

/****************************************************************************/

#ifndef HATNDBFIND_H
#define HATNDBFIND_H

#include <functional>

#include <hatn/common/error.h>

#include <hatn/db/db.h>
#include <hatn/db/object.h>

HATN_DB_NAMESPACE_BEGIN

using FindCb=std::function<bool (DbObject obj, Error& ec)>;

HATN_DB_NAMESPACE_END

#endif // HATNDBTRANSACTION_H
