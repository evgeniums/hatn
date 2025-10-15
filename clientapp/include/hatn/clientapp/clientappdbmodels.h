/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file clientapp/clientappdbmodels.h
  */

/****************************************************************************/

#ifndef HATNCLIENTAPPDBMODELS_H
#define HATNCLIENTAPPDBMODELS_H

#include <hatn/db/object.h>
#include <hatn/db/model.h>
#include <hatn/db/modelswrapper.h>

#include <hatn/clientapp/clientappdefs.h>
#include <hatn/clientapp/defaulttopic.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

HDU_UNIT_WITH(clientapp_data,(HDU_BASE(HATN_DB_NAMESPACE::object)),
    HDU_FIELD(name,TYPE_STRING,1)
    HDU_FIELD(data,TYPE_BYTES,2)
)

HATN_DB_UNIQUE_INDEX(clientAppDataIdx,clientapp_data::name)
HATN_DB_MODEL_PROTOTYPE(clientAppDataModel,clientapp_data,clientAppDataIdx())

class ClientAppDbModels : public db::ModelsWrapper
{
    public:

        constexpr static const char* DefaultTopic=HATN_CLIENTAPP_NAMESPACE::DefaultTopic;

        ClientAppDbModels(std::string prefix={}) : db::ModelsWrapper(std::move(prefix))
        {}

        const auto& clientAppDataModel() const
        {
            return db::makeModelFromPrototype(prefix(),HATN_CLIENTAPP_NAMESPACE::clientAppDataModel);
        }

        auto models()
        {
            return hana::make_tuple(
                [this](){return clientAppDataModel();}
            );
        }

        static std::shared_ptr<ClientAppDbModels> defaultInstance();
        static void freeDefaultInstance();
};

HATN_CLIENTAPP_NAMESPACE_END

#endif // HATNCLIENTAPPDBMODELS_H
