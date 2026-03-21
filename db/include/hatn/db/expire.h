/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file db/expire.h
  *
  */

/****************************************************************************/

#ifndef HATNDBEXPIRE_H
#define HATNDBEXPIRE_H

#include <hatn/db/db.h>
#include <hatn/db/model.h>

HATN_DB_NAMESPACE_BEGIN

HDU_UNIT(with_expire,
    HDU_FIELD(expire_at,TYPE_DATETIME,77)
)

HATN_DB_TTL_INDEX(expireIdx,1,with_expire::expire_at)

HATN_DB_NAMESPACE_END

#endif // HATNDBEXPIRE_H
