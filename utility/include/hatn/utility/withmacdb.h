/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file utility/withmacdb.h
  */

/****************************************************************************/

#ifndef HATNWITHMACDB_H
#define HATNWITHMACDB_H

#include <hatn/db/model.h>
#include <hatn/utility/withmac.h>

HATN_UTILITY_NAMESPACE_BEGIN

HATN_DB_INDEX(macPolicyIdx,with_mac_policy::mac_policy)

HATN_UTILITY_NAMESPACE_END

#endif // HATNWITHMACDB_H
