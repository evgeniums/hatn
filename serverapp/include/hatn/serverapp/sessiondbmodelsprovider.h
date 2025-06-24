/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file serverapp/sessiondbmodelsprovider.h
  */

/****************************************************************************/

#ifndef HATNSESSIONDBMODELSPROVIDER_H
#define HATNSESSIONDBMODELSPROVIDER_H

#include <hatn/db/modelsprovider.h>

#include <hatn/serverapp/serverappdefs.h>

HATN_SERVERAPP_NAMESPACE_BEGIN

class SessionDbModelsProvider_p;

using SessionDbModelsProvider = db::DbModelsProviderT<SessionDbModelsProvider_p>;

HATN_SERVERAPP_NAMESPACE_END

#endif // HATNSESSIONDBMODELSPROVIDER_H
