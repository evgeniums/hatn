/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file db/crud.h
  *
  * Contains declaration of CRUD.
  *
  */

/****************************************************************************/

#ifndef HATNDBCRUD_H
#define HATNDBCRUD_H

#include <hatn/common/result.h>
#include <hatn/common/objectid.h>

#include <hatn/db/db.h>
#include <hatn/db/namespace.h>

HATN_DB_NAMESPACE_BEGIN

struct Crud
{
    template <typename T>
    Result<common::STR_ID_TYPE> create(const Namespace& ns, const T& obj);

};

HATN_DB_NAMESPACE_END

#endif // HATNDBCRUD_H
