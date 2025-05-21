/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file clientserver/usercontroller.h
  */

/****************************************************************************/

#ifndef HATNUSERCONTROLLER_H
#define HATNUSERCONTROLLER_H

#include <hatn/common/objecttraits.h>

#include <hatn/db/update.h>
#include <hatn/db/indexquery.h>
#include <hatn/db/asyncmodelcontroller.h>

#include <hatn/clientserver/clientserver.h>
#include <hatn/clientserver/models/user.h>

HATN_CLIENT_SERVER_NAMESPACE_BEGIN

template <typename ContextT, typename Traits>
class UserController : public common::WithTraits<Traits>
{
    public:

        using common::WithTraits<Traits>::WithTraits;

        using Context=ContextT;
        using CallbackEc=db::AsyncCallbackEc<Context>;
        using CallbackList=db::AsyncCallbackList<Context>;
        using CallbackOid=db::AsyncCallbackOid<Context>;
        template <typename ModelT>
        using CallbackObj=db::AsyncCallbackObj<Context,ModelT>;

        void addUser(
            common::SharedPtr<Context> ctx,
            CallbackOid callback,
            common::SharedPtr<user::managed> usr,
            db::Topic topic
        )
        {
            this->traits().addUser(std::move(ctx),std::move(callback),std::move(usr),topic);
        }

        //! @todo Implement the rest user operations
};

//! @todo Remove placeholder
class HATN_CLIENT_SERVER_EXPORT LibPlaceHolder
{
    public:

        LibPlaceHolder();
};

HATN_CLIENT_SERVER_NAMESPACE_END

#endif // HATNUSERCONTROLLER_H
