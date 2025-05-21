/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file serverapp/userdbmodelsprovider.сpp
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

#include <hatn/serverapp/userdbmodels.h>
#include <hatn/serverapp/userdbmodelsprovider.h>

#include <hatn/db/ipp/modelsprovider.ipp>

HATN_SERVERAPP_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

class UserDbModelsProvider_p : public db::DbModelsProviderT_p<UserDbModels>
{
    public:

        using db::DbModelsProviderT_p<UserDbModels>::DbModelsProviderT_p;
};

//--------------------------------------------------------------------------

HATN_SERVERAPP_NAMESPACE_END

HATN_DB_NAMESPACE_BEGIN

template class HATN_SERVERAPP_EXPORT DbModelsProviderT<HATN_SERVERAPP_NAMESPACE::UserDbModelsProvider_p>;

HATN_DB_NAMESPACE_END
