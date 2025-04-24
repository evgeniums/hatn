/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file utility/aclmodels.h
  */

/****************************************************************************/

#ifndef HATNUTILITYACLMODELS_H
#define HATNUTILITYACLMODELS_H

#include <hatn/db/object.h>

#include <hatn/utility/utility.h>
#include <hatn/utility/aclconstants.h>

HATN_UTILITY_NAMESPACE_BEGIN

HDU_UNIT_WITH(acl_role,(HDU_BASE(db::object)),
    HDU_FIELD(name,TYPE_STRING,1)
    HDU_FIELD(description,TYPE_STRING,3)
)

HDU_UNIT_WITH(acl_role_operation,(HDU_BASE(db::object)),
    HDU_FIELD(role,TYPE_OBJECT_ID,1)
    HDU_FIELD(operation,HDU_TYPE_FIXED_STRING(OperationNameLength),2)
    HDU_FIELD(grant,TYPE_BOOL,3)
)

HDU_UNIT_WITH(acl_op_family_access,(HDU_BASE(db::object)),
    HDU_FIELD(role,TYPE_OBJECT_ID,1)
    HDU_FIELD(op_family,HDU_TYPE_FIXED_STRING(OperationNameLength),2)
    HDU_FIELD(access,TYPE_UINT32,3)
)

HDU_UNIT_WITH(acl_relation,(HDU_BASE(db::object)),
    HDU_FIELD(object,TYPE_STRING,1)
    HDU_FIELD(subject,TYPE_STRING,2)
    HDU_FIELD(role,TYPE_OBJECT_ID,3)
)

HATN_UTILITY_NAMESPACE_END

#endif // HATNUTILITYACLMODELS_H
