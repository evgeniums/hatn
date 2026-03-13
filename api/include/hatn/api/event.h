/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file api/auth/authprotocol.h
  */

/****************************************************************************/

#ifndef HATNAPIAUTHPROTOCOL_H
#define HATNAPIAUTHPROTOCOL_H

#include <hatn/api/api.h>
#include <hatn/api/withnameandversion.h>
#include <hatn/api/protocol.h>

HATN_API_NAMESPACE_BEGIN

using AuthProtocol=WithNameAndVersion<protocol::AuthProtocolNameLengthMax>;

HATN_API_NAMESPACE_END

#endif // HATNAPIAUTHPROTOCOL_H
