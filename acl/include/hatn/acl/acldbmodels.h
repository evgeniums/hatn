/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file acl/acldbmodels.h
  */

/****************************************************************************/

#ifndef HATNACLDBMODELS_H
#define HATNACLDBMODELS_H

#include <hatn/db/model.h>
#include <hatn/db/modelsprovider.h>

#include <hatn/acl/aclmodels.h>

HATN_ACL_NAMESPACE_BEGIN

HATN_DB_UNIQUE_INDEX(aclRelationObjSubjIdx,acl_relation::object,acl_relation::subject,acl_relation::role)
HATN_DB_INDEX(aclRelationObjRoleIdx,acl_relation::object,acl_relation::role)
HATN_DB_INDEX(aclRelationSubjRoleIdx,acl_relation::subject,acl_relation::role)
HATN_DB_INDEX(aclRelationRoleIdx,acl_relation::role)
HATN_DB_MODEL_PROTOTYPE(aclRelationModel,acl_relation,aclRelationObjSubjIdx(),aclRelationObjRoleIdx(),aclRelationSubjRoleIdx(),aclRelationRoleIdx())

HATN_DB_UNIQUE_INDEX(aclRoleNameIdx,acl_role::name)
HATN_DB_MODEL_PROTOTYPE(aclRoleModel,acl_role,aclRoleNameIdx())

HATN_DB_UNIQUE_INDEX(aclRoleOperationIdx,acl_role_operation::role,acl_role_operation::operation)
HATN_DB_MODEL_PROTOTYPE(aclRoleOperationModel,acl_role_operation,aclRoleOperationIdx())

class AclDbModels : public db::ModelsWrapper
{
    public:

        AclDbModels(std::string prefix={}) : db::ModelsWrapper(std::move(prefix))
        {}

        const auto& aclRelationModel() const
        {
            return db::makeModelFromProrotype(prefix(),HATN_ACL_NAMESPACE::aclRelationModel);
        }

        const auto& aclRoleModel() const
        {
            return db::makeModelFromProrotype(prefix(),HATN_ACL_NAMESPACE::aclRoleModel);
        }

        const auto& aclRoleOperationModel() const
        {
            return db::makeModelFromProrotype(prefix(),HATN_ACL_NAMESPACE::aclRoleOperationModel);
        }

        auto models()
        {
            return hana::make_tuple(
                [this](){return aclRelationModel();},
                [this](){return aclRoleModel();},
                [this](){return aclRoleOperationModel();}
            );
        }

    private:

        std::string m_prefix;
};


HATN_ACL_NAMESPACE_END

#endif // HATNACLDBMODELS_H
