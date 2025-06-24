/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file serverapp/userdbmodelsprovider.h
  */

/****************************************************************************/

#ifndef HATNUSERDBMODELSPROVIDER_H
#define HATNUSERDBMODELSPROVIDER_H

#include <hatn/db/modelsprovider.h>

#include <hatn/serverapp/serverappdefs.h>

HATN_SERVERAPP_NAMESPACE_BEGIN

class UserDbModelsProvider_p;

using UserDbModelsProvider = db::DbModelsProviderT<UserDbModelsProvider_p>;

HATN_SERVERAPP_NAMESPACE_END

#endif // HATNUSERDBMODELSPROVIDER_H
