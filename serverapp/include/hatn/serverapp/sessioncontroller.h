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
#include <hatn/serverapp/sessiondbmodels.h>
#include <hatn/serverapp/auth/authtokens.h>

HATN_SERVERAPP_NAMESPACE_BEGIN

struct SessionResponse
{
    common::SharedPtr<auth_complete::managed> response;
    common::SharedPtr<session::managed> session;
};

struct SessionCheckResult
{
    common::SharedPtr<auth_token::managed> token;
    common::SharedPtr<session::managed> session;
};

template <typename Traits>
class SessionController : public common::WithTraits<Traits>
{
    public:

        // Callback= void (auto ctx,common::Error ec,SessionResponse response}
        template <typename ContextT, typename CallbackT>
        void createSession(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback,            
            const du::ObjectId& login,
            const du::ObjectId& user,
            db::Topic topic
        ) const
        {
            this->traits().createSession(std::move(ctx),std::move(callback),login,user,topic);
        }

        // Callback= void (auto ctx,common::Error ec,SessionCheckResult result}
        template <typename ContextT, typename CallbackT>
        void checkSession(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback,
            common::SharedPtr<auth_with_token::managed> sessionContent,
            bool update=true
        ) const
        {
            this->traits().checkSession(std::move(ctx),std::move(callback),std::move(sessionContent),update);
        }

        template <typename ContextT, typename CallbackT>
        void refreshSession(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback,
            common::SharedPtr<auth_refresh::managed> message
        ) const
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
