/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file utility/sectiondbmodelsprovider.сpp
  *
  */

#include <hatn/db/modelswrapper.h>

#ifdef HATN_ENABLE_PLUGIN_ROCKSDB

#include <hatn/dataunit/ipp/objectid.ipp>
#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>

#include <hatn/db/plugins/rocksdb/ipp/fieldvaluetobuf.ipp>
#include <hatn/db/plugins/rocksdb/ipp/rocksdbmodels.ipp>

#endif

#include <hatn/utility/utility.h>
#include <hatn/utility/sectiondbmodels.h>
#include <hatn/utility/sectiondbmodelsprovider.h>

#include <hatn/db/ipp/modelsprovider.ipp>

HATN_UTILITY_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

class SectionDbModelsProvider_p : public db::DbModelsProviderT_p<SectionDbModels>
{
    public:

        using db::DbModelsProviderT_p<SectionDbModels>::DbModelsProviderT_p;
};

//--------------------------------------------------------------------------

HATN_UTILITY_NAMESPACE_END

HATN_DB_NAMESPACE_BEGIN

template class HATN_UTILITY_EXPORT DbModelsProviderT<HATN_UTILITY_NAMESPACE::SectionDbModelsProvider_p>;

HATN_DB_NAMESPACE_END
