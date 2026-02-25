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

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERMODELSRERVISION_H
