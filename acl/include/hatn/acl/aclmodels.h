/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file acl/aclmodels.h
  */

/****************************************************************************/

#ifndef HATNACLMODELS_H
#define HATNACLMODELS_H

#include <hatn/db/object.h>

#include <hatn/acl/acl.h>
#include <hatn/acl/aclconstants.h>

HATN_ACL_NAMESPACE_BEGIN

HDU_UNIT_WITH(acl_role,(HDU_BASE(db::object)),
    HDU_FIELD(name,TYPE_STRING,1)
    HDU_FIELD(description,TYPE_STRING,3)
)

HDU_UNIT_WITH(acl_role_operation,(HDU_BASE(db::object)),
    HDU_FIELD(role,TYPE_OBJECT_ID,1)
    HDU_FIELD(operation,HDU_TYPE_FIXED_STRING(OperationNameLength),2)
    HDU_FIELD(grant_ndeny,TYPE_BOOL,3)
)

HDU_UNIT_WITH(acl_subject_role,(HDU_BASE(db::object)),
    HDU_FIELD(object,TYPE_STRING,1)
    HDU_FIELD(subject,TYPE_STRING,2)
    HDU_FIELD(role,TYPE_OBJECT_ID,3)
)

HATN_ACL_NAMESPACE_END

#endif // HATNACLMODELS_H
