/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file utility/withmac.h
  */

/****************************************************************************/

#ifndef HATNWITHMAC_H
#define HATNWITHMAC_H

#include <hatn/db/object.h>

#include <hatn/utility/utility.h>

HATN_UTILITY_NAMESPACE_BEGIN

HDU_UNIT(with_mac_policy,
    HDU_FIELD(mac_policy,TYPE_UINT32,10000)
)

HATN_UTILITY_NAMESPACE_END

#endif // HATNWITHMAC_H
