/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file utility/acloperations.сpp
  *
  */

#include <hatn/utility/aclcontroller.h>

HATN_UTILITY_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

const AclOperations& AclOperations::instance()
{
    static AclOperations inst;
    return inst;
}

//--------------------------------------------------------------------------

const Operation& AclOperations::addRole()
{
    static Operation op{&AclOperations::instance(),"add_role",Access::featureBit(AccessType::Create)};
    return op;
}

//--------------------------------------------------------------------------

const Operation& AclOperations::readRole()
{
    static Operation op{&AclOperations::instance(),"read_role",Access::featureBit(AccessType::Read)};
    return op;
}

//--------------------------------------------------------------------------

const Operation& AclOperations::updateRole()
{
    static Operation op{&AclOperations::instance(),"update_role",Access::featureBit(AccessType::Update)};
    return op;
}

//--------------------------------------------------------------------------

const Operation& AclOperations::removeRole()
{
    static Operation op{&AclOperations::instance(),"remove_role",Access::featureBit(AccessType::Delete)};
    return op;
}

//--------------------------------------------------------------------------

const Operation& AclOperations::listRoles()
{
    static Operation op{&AclOperations::instance(),"list_roles",Access::featureBit(AccessType::Read)};
    return op;
}

//--------------------------------------------------------------------------

const Operation& AclOperations::addRoleOperation()
{
    static Operation op{&AclOperations::instance(),"add_role_operation",Access::featureBit(AccessType::Create)};
    return op;
}

//--------------------------------------------------------------------------

const Operation& AclOperations::readRoleOperation()
{
    static Operation op{&AclOperations::instance(),"read_role_operation",Access::featureBit(AccessType::Read)};
    return op;
}

//--------------------------------------------------------------------------

const Operation& AclOperations::removeRoleOperation()
{
    static Operation op{&AclOperations::instance(),"remove_role_operations",Access::featureBit(AccessType::Delete)};
    return op;
}

//--------------------------------------------------------------------------

const Operation& AclOperations::listRoleOperations()
{
    static Operation op{&AclOperations::instance(),"list_role_operationss",Access::featureBit(AccessType::Read)};
    return op;
}

//--------------------------------------------------------------------------

HATN_UTILITY_NAMESPACE_END
