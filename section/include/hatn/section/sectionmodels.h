/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file section/sectionmodels.h
  */

/****************************************************************************/

#ifndef HATNSECTIONMODELS_H
#define HATNSECTIONMODELS_H

#include <hatn/db/object.h>

#include <hatn/section/section.h>
#include <hatn/section/withmac.h>

HATN_SECTION_NAMESPACE_BEGIN

HDU_UNIT_WITH(topic_descriptor,(HDU_BASE(db::object),HDU_BASE(mac::with_mac_policy)),
    HDU_FIELD(name,TYPE_STRING,1)
    HDU_FIELD(description,TYPE_STRING,2)
    HDU_FIELD(parent,TYPE_OBJECT_ID,3)
    HDU_FIELD(section,TYPE_BOOL,4)
)

HATN_SECTION_NAMESPACE_END

#endif // HATNSECTIONMODELS_H
