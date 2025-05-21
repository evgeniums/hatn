/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file utility/acldbmodelsprovider.h
  */

/****************************************************************************/

#ifndef HATNUTILITYACLDBMODELSPROVIDER_H
#define HATNUTILITYACLDBMODELSPROVIDER_H

#include <hatn/db/modelsprovider.h>

#include <hatn/utility/utility.h>

HATN_UTILITY_NAMESPACE_BEGIN

class AclDbModelsProvider_p;

using AclDbModelsProvider = db::DbModelsProviderT<AclDbModelsProvider_p>;

HATN_UTILITY_NAMESPACE_END

#endif // HATNUTILITYACLDBMODELSPROVIDER_H
