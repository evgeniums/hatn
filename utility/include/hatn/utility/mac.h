/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file utility/mac.h
  */

/****************************************************************************/

#ifndef HATNUTILITYMAC_H
#define HATNUTILITYMAC_H

#include <hatn/utility/utility.h>
#include <hatn/utility/utilityerror.h>
#include <hatn/utility/accesstype.h>

HATN_UTILITY_NAMESPACE_BEGIN

inline Error checkMac(uint32_t objectMacPolicy, uint32_t subjectMacPolicy, AccessType accessType)
{
    if (isDeleteAccess(accessType))
    {
        return OK;
    }

    if (subjectMacPolicy==objectMacPolicy)
    {
        return OK;
    }

    if (subjectMacPolicy<objectMacPolicy)
    {
        if (isReadAccess(accessType))
        {
            return OK;
        }
        return utilityError(UtilityError::MAC_FORBIDDEN);
    }

    if (subjectMacPolicy>objectMacPolicy)
    {
        if (isCreateAccess(accessType))
        {
            return OK;
        }
    }

    return utilityError(UtilityError::MAC_FORBIDDEN);
}

HATN_UTILITY_NAMESPACE_END

#endif // HATNUTILITYMAC_H
