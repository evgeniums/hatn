/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file section/withmacdb.h
  */

/****************************************************************************/

#ifndef HATNWITHMACDB_H
#define HATNWITHMACDB_H

//! @todo Implement Manadatory Access Control

#ifdef HATN_MANDATORY_CONTROL
#include <hatn/mac/withmacdb.h>

#else

#include <hatn/db/model.h>
#include <hatn/section/withmac.h>

HATN_MAC_NAMESPACE_BEGIN

HATN_DB_INDEX(macPolicyIdx,with_mac_policy::mac_policy)

HATN_MAC_NAMESPACE_END

#endif

#endif // HATNWITHMACDB_H
