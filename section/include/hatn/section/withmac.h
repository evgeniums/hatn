/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file section/withmac.h
  */

/****************************************************************************/

#ifndef HATNWITHMAC_H
#define HATNWITHMAC_H

#include <hatn/db/object.h>

//! @todo Implement Manadatory Access Control

#ifdef HATN_MANDATORY_CONTROL
#include <hatn/mac/withmac.h>

#else

#include <hatn/section/mac.h>

HATN_MAC_NAMESPACE_BEGIN

HDU_UNIT(with_mac_policy,
    HDU_FIELD(mac_policy,TYPE_OBJECT_ID,10000)
)

HATN_MAC_NAMESPACE_END

#endif

#endif // HATNWITHMAC_H
