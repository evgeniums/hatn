/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file clientserver/models/usernamevalidator.h
  */

/****************************************************************************/

#ifndef HATNUSERNAMEVALIDATOR_H
#define HATNUSERNAMEVALIDATOR_H

#include <cstddef>

#include <hatn/clientserver/clientserver.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

constexpr const size_t MinDigitalCodeLinkLength=6;
constexpr const char* UserRegExp="^[a-z_][a-z0-9_\\-]*$";
constexpr const char* PlainDomainRegExp="^[a-z0-9\\-]{3,}$";
constexpr const char* DomainRegExp="^(?:[\\p{L}\\p{N}][\\p{L}\\p{N}-]*\\.)+[\\p{L}\\p{N}]{2,}$";

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNUSERNAMEVALIDATOR_H
