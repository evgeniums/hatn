/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file serverapp/session/sessioncontroller.h
  */

/****************************************************************************/

#ifndef HATNSESSIONCONTROLLER_H
#define HATNSESSIONCONTROLLER_H

#include <hatn/dataunit/objectid.h>

#include <hatn/db/topic.h>

#include <hatn/clientserver/auth/authprotocol.h>

#include <hatn/serverapp/serverappdefs.h>

HATN_SERVERAPP_NAMESPACE_BEGIN

template <typename ContextTraits>
class SessionController
{
    public:

        using Context=typename ContextTraits::Context;

        template <typename CallbackT>
        void createSession(
            common::SharedPtr<Context> ctx,
            CallbackT callback,
            const du::ObjectId& loginId,
            db::Topic topic
        );

        template <typename CallbackT>
        void checkSession(
            common::SharedPtr<Context> ctx,
            CallbackT callback,
            const HATN_CLIENT_SERVER_NAMESPACE::auth_token::managed* sessionContent
        );

        template <typename CallbackT>
        void refreshSession(
            common::SharedPtr<Context> ctx,
            CallbackT callback,
            const HATN_CLIENT_SERVER_NAMESPACE::auth_refresh::managed* message
        );

        template <typename CallbackT>
        void dropSession(
            common::SharedPtr<Context> ctx,
            CallbackT callback,
            const du::ObjectId& sessionId,
            db::Topic topic
        );

        template <typename CallbackT>
        void dropLoginSessions(
            common::SharedPtr<Context> ctx,
            CallbackT callback,
            const du::ObjectId& loginId,
            db::Topic topic
        );

        template <typename CallbackT>
        void dropSessions(
            common::SharedPtr<Context> ctx,
            CallbackT callback,
            db::Topic topic
        );
};

HATN_SERVERAPP_NAMESPACE_END

#endif // HATNSESSIONCONTROLLER_H
