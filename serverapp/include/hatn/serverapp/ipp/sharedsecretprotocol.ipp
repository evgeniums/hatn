/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file serverapp/ipp/sharedsecretprotocol.ipp
  */

/****************************************************************************/

#ifndef HATNSERVERSHAREDSECRETPROTOCOL_IPP
#define HATNSERVERSHAREDSECRETPROTOCOL_IPP

#include <hatn/common/meta/chain.h>

#include <hatn/serverapp/auth/sharedsecretprotocol.h>

HATN_SERVERAPP_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

template <typename ContextTraits>
template <typename CallbackT>
void AuthProtocolSharedSecret<ContextTraits>::prepare(
        common::SharedPtr<Context> ctx,
        CallbackT callback,
        const common::SharedPtr<auth_negotiate_request::managed>& authRequest
    )
{

}

//--------------------------------------------------------------------------

HATN_SERVERAPP_NAMESPACE_END

#endif // HATNSERVERSHAREDSECRETPROTOCOL_IPP
