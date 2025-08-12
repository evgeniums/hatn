/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file utility/journalmodels.h
  */

/****************************************************************************/

#ifndef HATNUTILITYJOURNALMODELS_H
#define HATNUTILITYJOURNALMODELS_H

#include <hatn/db/object.h>

#include <hatn/utility/utility.h>
#include <hatn/utility/accesstype.h>

HATN_UTILITY_NAMESPACE_BEGIN

HDU_MAP(parameter,TYPE_STRING,TYPE_STRING)

HDU_UNIT_WITH(event,(HDU_BASE(db::object)),
    HDU_FIELD(ctx,TYPE_OBJECT_ID,1)
    HDU_FIELD(status,TYPE_STRING,2)
    HDU_FIELD(op_family,TYPE_STRING,3)
    HDU_FIELD(op,TYPE_STRING,4)
    HDU_FIELD(subject,TYPE_OBJECT_ID,5)
    HDU_FIELD(subject_topic,TYPE_STRING,6)
    HDU_FIELD(subject_model,TYPE_STRING,7)
    HDU_FIELD(subject_name,TYPE_STRING,8)
    HDU_FIELD(object,TYPE_OBJECT_ID,9)
    HDU_FIELD(object_topic,TYPE_STRING,10)
    HDU_FIELD(object_model,TYPE_STRING,11)
    HDU_FIELD(object_name,TYPE_STRING,12)
    HDU_FIELD(access_type,HDU_TYPE_ENUM(AccessType),13)
    HDU_FIELD(service,TYPE_STRING,14)
    HDU_FIELD(service_method,TYPE_STRING,15)
    HDU_MAP_FIELD(origin,parameter::TYPE,16)
    HDU_MAP_FIELD(parameters,parameter::TYPE,17)
)

/**
 * possible origin fields, not limited to:
 *
 * [
 *  {
 *      "type" : "ip_addr"
 *      "value" : "127.0.0.1"
 *  },
 *  {
 *      "type" : "ip_port"
 *      "value" : "11223"
 *  },
 *  {
 *      "type" : "session"
 *      "value" : "11aabbccddeeff"
 *  }
 * ]
 * */

HATN_UTILITY_NAMESPACE_END

#endif // HATNUTILITYJOURNALMODELS_H
