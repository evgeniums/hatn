/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/models/employment.h
  */

/****************************************************************************/

#ifndef HATNCLIENTSERVERMODELEMPLOYMENT_H
#define HATNCLIENTSERVERMODELEMPLOYMENT_H

#include <hatn/db/object.h>

#include <hatn/clientserver/clientserver.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

HDU_UNIT(employment,
    HDU_FIELD(organization,TYPE_STRING,1)
    HDU_FIELD(department,TYPE_STRING,2)
    HDU_FIELD(position,TYPE_STRING,3)
)

struct formatEmploymentT
{
    std::string operator()(const employment::type& obj, const employment::type& sample={}) const
    {
        const auto& position=obj.fieldValue(employment::position);
        const auto& organization=obj.fieldValue(employment::organization);
        const auto& department=obj.fieldValue(employment::department);

        const auto& sampleOrganization=sample.fieldValue(employment::organization);
        const auto& sampleDepartment=sample.fieldValue(employment::department);

        bool addOrganization=!organization.empty() && organization!=sampleOrganization;
        bool addDepartment=!department.empty() && department!=sampleDepartment;
        bool addPosition=!position.empty();

        if (addPosition)
        {
            if (addOrganization)
            {
                if (addDepartment)
                {
                    return fmt::format("{}, {}, {}",position,organization,department);
                }
                else
                {
                    return fmt::format("{}, {}",position,organization);
                }
            }
            else if (addDepartment)
            {
                return fmt::format("{}, {}",position,department);
            }
            else
            {
                return std::string{position};
            }
        }
        else
        {
            if (addOrganization)
            {
                if (addDepartment)
                {
                    return fmt::format("{}, {}",organization,department);
                }
                else
                {
                    return std::string{organization};
                }
            }
            else if (addDepartment)
            {
                return std::string{department};
            }
        }

        return std::string{};
    }
};
constexpr formatEmploymentT formatEmployment{};

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNCLIENTSERVERMODELEMPLOYMENT_H
