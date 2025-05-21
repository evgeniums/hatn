/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file utility/accessstatus.h
  */

/****************************************************************************/

#ifndef HATNACCESSSTATUS_H
#define HATNACCESSSTATUS_H

#include <cstdint>

#include <hatn/utility/utility.h>

HATN_UTILITY_NAMESPACE_BEGIN

enum class AccessStatus : uint8_t
{
    Unknown=0,
    Grant=1,
    Deny=2
};

HATN_UTILITY_NAMESPACE_END

#endif // HATNACCESSSTATUS_H
