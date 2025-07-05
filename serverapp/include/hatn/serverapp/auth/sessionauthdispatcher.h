/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file serverapp/auth/sessionauthdispatcher.h
  */

/****************************************************************************/

#ifndef HATNSERVERSESSIONAUTHDISPATCHER_H
#define HATNSERVERSESSIONAUTHDISPATCHER_H

#include <hatn/common/flatmap.h>

#include <hatn/api/authprotocol.h>
#include <hatn/api/server/authdispatcher.h>

#include <hatn/serverapp/sessioncontroller.h>

HATN_SERVERAPP_NAMESPACE_BEGIN

template <typename EnvT=serverapi::BasicEnv, typename RequestT=serverapi::Request<EnvT>>
class SessionAuthProtocol : public api::AuthProtocol
{
    public:

        using Env=EnvT;
        using Request=RequestT;

        SessionAuthProtocol();

        void invoke(
            common::SharedPtr<serverapi::RequestContext<Request>> reqCtx,
            serverapi::DispatchCb<Request> callback,
            common::ByteArrayShared authFieldContent
        );
};

template <typename EnvT=serverapi::BasicEnv, typename RequestT=serverapi::Request<EnvT>>
class SessionAuthDispatcher : public serverapi::AuthDispatcher<EnvT,RequestT,SessionAuthProtocol<EnvT,RequestT>>
{
    public:

        SessionAuthDispatcher();
};

HATN_SERVERAPP_NAMESPACE_END

#endif // HATNSERVERSESSIONAUTHDISPATCHER_H
