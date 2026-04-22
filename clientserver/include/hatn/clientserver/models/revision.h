/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/models/revision.h
  */

/****************************************************************************/

#ifndef HATNCLIENTSERVERMODELSRERVISION_H
#define HATNCLIENTSERVERMODELSRERVISION_H

#include <hatn/db/object.h>

#include <hatn/clientserver/clientserver.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

constexpr const int RevisionFieldId=120;

//! Object revision
HDU_UNIT(with_revision,
    HDU_FIELD(revision,TYPE_OBJECT_ID,RevisionFieldId) //!< Object revision
)

constexpr const int LedgerRevFieldId=121;
constexpr const int LedgerTierFieldId=122;

enum class LedgerTier : int8_t
{
    Fast,
    Normal,
    Long
};

HDU_UNIT(with_ledger_rev,
    HDU_FIELD(ledger_rev,TYPE_OBJECT_ID,LedgerRevFieldId)
    HDU_FIELD(ledger_tier,HDU_TYPE_ENUM(LedgerTier),LedgerTierFieldId,false,LedgerTier::Normal)
)

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERMODELSRERVISION_H
