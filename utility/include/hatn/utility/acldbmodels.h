/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file utility/acldbmodels.h
  */

/****************************************************************************/

#ifndef HATNUTILITYACLDBMODELS_H
#define HATNUTILITYACLDBMODELS_H

#include <hatn/db/model.h>
#include <hatn/db/modelswrapper.h>

#include <hatn/utility/aclmodels.h>

HATN_UTILITY_NAMESPACE_BEGIN

HATN_DB_UNIQUE_INDEX(aclRelationObjSubjIdx,acl_relation::object,acl_relation::subject,acl_relation::role)
HATN_DB_INDEX(aclRelationObjRoleIdx,acl_relation::object,acl_relation::role)
HATN_DB_INDEX(aclRelationSubjRoleIdx,acl_relation::subject,acl_relation::role)
HATN_DB_INDEX(aclRelationRoleIdx,acl_relation::role)
HATN_DB_MODEL_PROTOTYPE(aclRelationModel,acl_relation,aclRelationObjSubjIdx(),aclRelationObjRoleIdx(),aclRelationSubjRoleIdx(),aclRelationRoleIdx())

HATN_DB_UNIQUE_INDEX(aclRoleNameIdx,acl_role::name)
HATN_DB_MODEL_PROTOTYPE(aclRoleModel,acl_role,aclRoleNameIdx())

HATN_DB_UNIQUE_INDEX(aclOpFamilyAccessIdx,acl_op_family_access::role,acl_op_family_access::op_family)
HATN_DB_MODEL_PROTOTYPE(aclOpFamilyAccessModel,acl_op_family_access,aclOpFamilyAccessIdx())

HATN_DB_UNIQUE_INDEX(aclRoleOperationIdx,acl_role_operation::role,acl_role_operation::operation)
HATN_DB_MODEL_PROTOTYPE(aclRoleOperationModel,acl_role_operation,aclRoleOperationIdx())


class AclDbModels : public db::ModelsWrapper
{
    public:

        AclDbModels(std::string prefix={}) : db::ModelsWrapper(std::move(prefix))
        {}

        const auto& aclRelationModel() const
        {
            return db::makeModelFromProrotype(prefix(),HATN_UTILITY_NAMESPACE::aclRelationModel);
        }

        const auto& aclRoleModel() const
        {
            return db::makeModelFromProrotype(prefix(),HATN_UTILITY_NAMESPACE::aclRoleModel);
        }

        const auto& aclRoleOperationModel() const
        {
            return db::makeModelFromProrotype(prefix(),HATN_UTILITY_NAMESPACE::aclRoleOperationModel);
        }

        const auto& aclOpFamilyAccessModel() const
        {
            return db::makeModelFromProrotype(prefix(),HATN_UTILITY_NAMESPACE::aclOpFamilyAccessModel);
        }

        auto models()
        {
            return hana::make_tuple(
                [this](){return aclRelationModel();},
                [this](){return aclRoleModel();},
                [this](){return aclRoleOperationModel();},
                [this](){return aclOpFamilyAccessModel();}
            );
        }

    private:

        std::string m_prefix;
};


HATN_UTILITY_NAMESPACE_END

#endif // HATNUTILITYACLDBMODELS_H
