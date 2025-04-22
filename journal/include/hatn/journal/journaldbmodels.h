/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file journal/journaldbmodels.h
  */

/****************************************************************************/

#ifndef HATNJOURNALDBMODELS_H
#define HATNJOURNALDBMODELS_H

#include <hatn/db/model.h>
#include <hatn/db/modelswrapper.h>

#include <hatn/journal/journalmodels.h>

HATN_JOURNAL_NAMESPACE_BEGIN

HATN_DB_INDEX(familyIdx,record::family)
HATN_DB_INDEX(operationIdx,record::operation)
HATN_DB_INDEX(objectIdx,record::object)
HATN_DB_INDEX(objectModelIdx,record::object_model)
HATN_DB_INDEX(subjectIdx,record::subject)
HATN_DB_INDEX(subjectModelIdx,record::subject_model)
HATN_DB_INDEX(originIdx,record::origin)
HATN_DB_INDEX(accessTypeIdx,record::access_type)
HATN_DB_INDEX(serviceIdx,record::service,record::service_method)
HATN_DB_INDEX(appIdx,record::app_id)
HATN_DB_INDEX(appTypeIdx,record::app_type)
HATN_DB_INDEX(dataIdx,db::nested(record::data,record_var::name),db::nested(record::data,record_var::value))

HATN_DB_OID_PARTITION_MODEL_PROTOTYPE(recordModel,record,
                            familyIdx(),
                            operationIdx(),
                            objectIdx(),
                            objectModelIdx(),
                            subjectIdx(),
                            subjectModelIdx(),
                            originIdx(),
                            accessTypeIdx(),
                            serviceIdx(),
                            appIdx(),
                            appTypeIdx()
                                      ,
                            dataIdx()
                        )

class JournalDbModels : public db::ModelsWrapper
{
    public:

        JournalDbModels(std::string prefix={}) : db::ModelsWrapper(std::move(prefix))
        {}

        const auto& recordModel() const
        {
            return db::makeModelFromProrotype(prefix(),HATN_JOURNAL_NAMESPACE::recordModel);
        }

        auto models()
        {
            return hana::make_tuple(
                [this](){return recordModel();}
            );
        }

    private:

        std::string m_prefix;
};

HATN_JOURNAL_NAMESPACE_END

#endif // HATNJOURNALDBMODELS_H
