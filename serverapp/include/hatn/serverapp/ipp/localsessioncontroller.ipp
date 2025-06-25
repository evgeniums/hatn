/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file serverapp/ipp/localsessioncontroller.ipp
  */

/****************************************************************************/

#ifndef HATNLOCALSESSIONCONTROLLER_IPP
#define HATNLOCALSESSIONCONTROLLER_IPP

#include <hatn/common/meta/chain.h>

#include <hatn/serverapp/sessiondbmodels.h>
#include <hatn/serverapp/localsessioncontroller.h>

HATN_SERVERAPP_NAMESPACE_BEGIN

//--------------------------------------------------------------------------

template <typename ContextTraits>
template <typename CallbackT>
void LocalSessionController<ContextTraits>::createSession(
        common::SharedPtr<Context> ctx,
        CallbackT callback,
        const du::ObjectId& login,
        db::Topic topic
    ) const
{
    const auto* factory=ContextTraits::factory(ctx);

    auto session=factory->template createObject<session::managed>();
}

//--------------------------------------------------------------------------

HATN_SERVERAPP_NAMESPACE_END

#endif // HATNLOCALSESSIONCONTROLLER_IPP
