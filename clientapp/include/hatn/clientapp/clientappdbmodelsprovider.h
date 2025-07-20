/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file clientapp/clientappdbmodelsprovider.h
  */

/****************************************************************************/

#ifndef HATNCLIENTAPPDBMODELSPROVIDER_H
#define HATNCLIENTAPPDBMODELSPROVIDER_H

#include <hatn/db/modelsprovider.h>

#include <hatn/clientapp/clientappdefs.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

class ClientAppDbModelsProvider_p;

using ClientAppDbModelsProvider = HATN_DB_NAMESPACE::DbModelsProviderT<ClientAppDbModelsProvider_p>;

HATN_CLIENTAPP_NAMESPACE_END

#endif // HATNCLIENTAPPDBMODELSPROVIDER_H
