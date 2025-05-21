/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file section/sectiondbmodelsprovider.h
  */

/****************************************************************************/

#ifndef HATNSECTIONDBMODELSPROVIDER_H
#define HATNSECTIONDBMODELSPROVIDER_H

#include <hatn/db/modelsprovider.h>

#include <hatn/utility/utility.h>

HATN_UTILITY_NAMESPACE_BEGIN

class SectionDbModelsProvider_p;

using SectionDbModelsProvider = db::DbModelsProviderT<SectionDbModelsProvider_p>;

HATN_UTILITY_NAMESPACE_END

#endif // HATNSECTIONDBMODELSPROVIDER_H
