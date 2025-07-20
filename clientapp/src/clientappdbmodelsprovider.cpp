/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientapp/clientappdbmodelsprovider.сpp
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

#include <hatn/clientapp/clientappdbmodels.h>
#include <hatn/clientapp/clientappdbmodelsprovider.h>

#include <hatn/db/ipp/modelsprovider.ipp>

HATN_CLIENTAPP_NAMESPACE_BEGIN

namespace {

std::shared_ptr<ClientAppDbModels>& modelsInstance()
{
    static std::shared_ptr<ClientAppDbModels> inst;
    if (!inst)
    {
        inst=std::make_shared<ClientAppDbModels>();
    }
    return inst;
}

}

std::shared_ptr<ClientAppDbModels> ClientAppDbModels::defaultInstance()
{
    return modelsInstance();
}

void ClientAppDbModels::freeDefaultInstance()
{
    modelsInstance().reset();
}

//--------------------------------------------------------------------------

class ClientAppDbModelsProvider_p : public db::DbModelsProviderT_p<ClientAppDbModels>
{
    public:

        using db::DbModelsProviderT_p<ClientAppDbModels>::DbModelsProviderT_p;
};

//--------------------------------------------------------------------------

HATN_CLIENTAPP_NAMESPACE_END

HATN_DB_NAMESPACE_BEGIN

template class HATN_CLIENTAPP_EXPORT DbModelsProviderT<HATN_CLIENTAPP_NAMESPACE::ClientAppDbModelsProvider_p>;

HATN_DB_NAMESPACE_END
