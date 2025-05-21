/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file utility/journaldbmodelsprovider.h
  */

/****************************************************************************/

#ifndef HATNJOURNALDBMODELSPROVIDER_H
#define HATNJOURNALDBMODELSPROVIDER_H

#include <hatn/db/modelsprovider.h>

#include <hatn/utility/journal.h>

HATN_UTILITY_NAMESPACE_BEGIN

class JournalDbModelsProvider_p;

using JournalDbModelsProvider = db::DbModelsProviderT<JournalDbModelsProvider_p>;

HATN_UTILITY_NAMESPACE_END

#endif // HATNJOURNALDBMODELSPROVIDER_H
