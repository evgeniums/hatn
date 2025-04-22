/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file journal/journaldbmodelsprovider.сpp
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

#include <hatn/journal/journal.h>
#include <hatn/journal/journaldbmodels.h>
#include <hatn/journal/journaldbmodelsprovider.h>

#include <hatn/db/ipp/modelsprovider.ipp>

HATN_JOURNAL_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

class JournalDbModelsProvider_p : public DbModelsProviderT_p<JournalDbModels>
{
    public:

        using DbModelsProviderT_p<JournalDbModels>::DbModelsProviderT_p;
};

//--------------------------------------------------------------------------

HATN_JOURNAL_NAMESPACE_END

HATN_DB_NAMESPACE_BEGIN

template class HATN_JOURNAL_EXPORT DbModelsProviderT<HATN_JOURNAL_NAMESPACE::JournalDbModelsProvider_p>;

HATN_DB_NAMESPACE_END
