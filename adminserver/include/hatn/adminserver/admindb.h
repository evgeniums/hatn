/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file adminserver/admin.h
  */

/****************************************************************************/

#ifndef HATNSERVERAADMINDB_H
#define HATNSERVERAADMINDB_H

#include <hatn/db/model.h>

#include <hatn/adminclient/admin.h>

#include <hatn/adminserver/adminserver.h>

HATN_ADMIN_SERVER_NAMESPACE_BEGIN

HATN_DB_UNIQUE_INDEX(adminAliasIdx,admin::alias)
HATN_DB_INDEX(adminNameIdx,admin::name)
HATN_DB_MODEL(adminModel,admin,adminAliasIdx(),adminNameIdx())

HATN_DB_UNIQUE_INDEX(adminGroupAdminGroupIdx,admin_group::admin,admin_group::group)
HATN_DB_INDEX(adminGroupGroupIdx,admin_group::group)
HATN_DB_MODEL(adminGroupModel,admin_group,adminGroupAdminGroupIdx(),adminGroupGroupIdx())

HATN_ADMIN_SERVER_NAMESPACE_END

#endif // HATNSERVERAADMINDB_H
