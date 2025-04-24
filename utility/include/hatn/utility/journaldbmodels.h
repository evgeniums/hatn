/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file utility/journaldbmodels.h
  */

/****************************************************************************/

#ifndef HATNJOURNALDBMODELS_H
#define HATNJOURNALDBMODELS_H

#include <hatn/db/model.h>
#include <hatn/db/modelswrapper.h>

#include <hatn/utility/journalmodels.h>

HATN_UTILITY_NAMESPACE_BEGIN

HATN_DB_INDEX(objectIdx,event::object,event::op_family,event::op,event::status)
HATN_DB_INDEX(objectTopicIdx,event::object_topic,event::object,event::op_family,event::op,event::status)
HATN_DB_INDEX(objectModelIdx,event::object_model,event::object_topic,event::op_family,event::op,event::status)
HATN_DB_INDEX(objectAccessIdx,event::object_topic,event::object,event::access_type,event::status)
HATN_DB_INDEX(objectModelAccessIdx,event::object_model,event::object_topic,event::access_type,event::status)

HATN_DB_INDEX(subjectIdx,event::subject,event::op_family,event::op,event::status)
HATN_DB_INDEX(subjectObjIdx,event::subject,event::object_topic,event::object,event::op_family,event::op,event::status)
HATN_DB_INDEX(subjectAccessIdx,event::subject,event::access_type,event::status)
HATN_DB_INDEX(subjectObjAccessIdx,event::subject,event::object_topic,event::object,event::access_type,event::status)

HATN_DB_INDEX(originIdx,event::origin,event::op_family,event::op,event::status)
HATN_DB_INDEX(originObjIdx,event::origin,event::object_topic,event::object,event::op_family,event::op,event::status)
HATN_DB_INDEX(originAccessIdx,event::origin,event::access_type,event::status)
HATN_DB_INDEX(originObjAccessIdx,event::origin,event::object_topic,event::object,event::access_type,event::status)
HATN_DB_INDEX(originSubjIdx,event::origin,event::subject,event::object,event::op_family,event::op,event::status)

HATN_DB_INDEX(serviceIdx,event::service,event::service_method,event::object_topic,event::object,event::status)
HATN_DB_INDEX(serviceSubjIdx,event::service,event::service_method,event::subject,event::status)
HATN_DB_INDEX(serviceSubjObjIdx,event::service,event::service_method,event::subject,event::object_topic,event::object,event::status)

HATN_DB_INDEX(ctxIdx,event::ctx)

//! @todo critical: Enable parameter index when map indexes are implemented in db
#if 0
HATN_DB_INDEX(paramIdx,db::nested(event::parameters,parameter::name),db::nested(event::parameters,parameter::value))
#endif

HATN_DB_OID_PARTITION_MODEL_PROTOTYPE(eventModel,event,
                            objectIdx(),
                            objectTopicIdx(),
                            objectModelIdx(),
                            objectAccessIdx(),
                            objectModelAccessIdx(),
                            subjectIdx(),
                            subjectObjIdx(),
                            subjectAccessIdx(),
                            subjectObjAccessIdx(),
                            originIdx(),
                            originObjIdx(),
                            originAccessIdx(),
                            originObjAccessIdx(),
                            originSubjIdx(),
                            serviceIdx(),
                            serviceSubjIdx(),
                            serviceSubjObjIdx(),
                            ctxIdx()
#if 0
                            ,paramIdx()
#endif
                        )

class JournalDbModels : public db::ModelsWrapper
{
    public:

        JournalDbModels(std::string prefix={}) : db::ModelsWrapper(std::move(prefix))
        {}

        const auto& eventModel() const
        {
            return db::makeModelFromProrotype(prefix(),HATN_UTILITY_NAMESPACE::eventModel);
        }

        auto models()
        {
            return hana::make_tuple(
                [this](){return eventModel();}
            );
        }

    private:

        std::string m_prefix;
};

HATN_UTILITY_NAMESPACE_END

#endif // HATNJOURNALDBMODELS_H
