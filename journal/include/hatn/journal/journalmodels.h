/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file journal/journalmodels.h
  */

/****************************************************************************/

#ifndef HATNJOURNALMODELS_H
#define HATNJOURNALMODELS_H

#include <hatn/db/object.h>

#include <hatn/journal/journal.h>

HATN_JOURNAL_NAMESPACE_BEGIN

enum class AccessType : uint8_t
{
    Create=0,
    Read=1,
    Update=2,
    Delete=3,
    ReadUpdate=4
};

HDU_MAP(record_var,TYPE_STRING,TYPE_STRING)

HDU_UNIT_WITH(record,(HDU_BASE(db::object)),
    HDU_FIELD(family,TYPE_STRING,1)
    HDU_FIELD(operation,TYPE_STRING,2)
    HDU_FIELD(subject,TYPE_OBJECT_ID,3)
    HDU_FIELD(subject_topic,TYPE_STRING,4)
    HDU_FIELD(subject_model,TYPE_STRING,5)
    HDU_FIELD(subject_name,TYPE_STRING,6)
    HDU_FIELD(object,TYPE_OBJECT_ID,7)
    HDU_FIELD(object_topic,TYPE_STRING,8)
    HDU_FIELD(object_model,TYPE_STRING,9)
    HDU_FIELD(object_name,TYPE_STRING,10)
    HDU_FIELD(origin_type,TYPE_STRING,11)
    HDU_FIELD(origin,TYPE_STRING,12)
    HDU_FIELD(access_type,HDU_TYPE_ENUM(AccessType),13)
    HDU_FIELD(service,TYPE_STRING,14)
    HDU_FIELD(service_method,TYPE_STRING,15)
    HDU_FIELD(app_type,TYPE_STRING,16)
    HDU_FIELD(app_id,TYPE_STRING,17)
    HDU_MAP_FIELD(data,record_var,19)
)

HATN_JOURNAL_NAMESPACE_END

#endif // HATNJOURNALMODELS_H
