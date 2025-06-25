/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file serverapp/sessioncontroller.h
  */

/****************************************************************************/

#ifndef HATNSESSIONCONTROLLER_H
#define HATNSESSIONCONTROLLER_H

#include <hatn/common/objecttraits.h>

#include <hatn/db/topic.h>

#include <hatn/clientserver/auth/authprotocol.h>

#include <hatn/serverapp/serverappdefs.h>

HATN_SERVERAPP_NAMESPACE_BEGIN

template <typename Traits>
class SessionController : public common::WithTraits<Traits>
{
    public:

        template <typename ContextT, typename CallbackT>
        void createSession(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback,
            const du::ObjectId& login,
            db::Topic topic
        )
        {
            this->traits().createSession(std::move(ctx),std::move(callback),login,topic);
        }

        template <typename ContextT, typename CallbackT>
        void checkSession(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback,
            common::SharedPtr<HATN_CLIENT_SERVER_NAMESPACE::auth_with_token::managed> sessionContent
        )
        {
            this->traits().checkSession(std::move(ctx),std::move(callback),std::move(sessionContent));
        }

        template <typename ContextT, typename CallbackT>
        void refreshSession(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback,
            common::SharedPtr<HATN_CLIENT_SERVER_NAMESPACE::auth_refresh::managed> message
        )
        {
            this->traits().refreshSession(std::move(ctx),std::move(callback),std::move(message));
        }

#if 0
        template <typename ContextT, typename CallbackT>
        void dropSession(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback,
            const du::ObjectId& sessionId,
            db::Topic topic
        );

        template <typename ContextT, typename CallbackT>
        void dropLoginSessions(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback,
            const du::ObjectId& loginId,
            db::Topic topic
        );

        template <typename ContextT, typename CallbackT>
        void dropSessions(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback,
            db::Topic topic
        );
#endif
};

HATN_SERVERAPP_NAMESPACE_END

#endif // HATNSESSIONCONTROLLER_H
