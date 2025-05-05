/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file adminserver/userservice/adduser.h
  */

/****************************************************************************/

#ifndef HATNUSERERVICEADDUSER_H
#define HATNUSERERVICEADDUSER_H

#include <hatn/api/server/serverservice.h>

#include <hatn/clientserver/models/user.h>

#include <hatn/adminclient/userserviceconstants.h>

#include <hatn/adminserver/adminserver.h>
#include <hatn/adminserver/userservice/userservicemethod.h>

HATN_ADMIN_SERVER_NAMESPACE_BEGIN

namespace user_service {

template <typename RequestT>
class AddUserTraits : public BaseMethod<RequestT>
{
    public:

        using BaseMethod<RequestT>::BaseMethod;

        template <typename CallbackT>
        void exec(
                common::SharedPtr<HATN_API_SERVER_NAMESPACE::RequestContext<RequestT>> requestCtx,
                CallbackT callback,
                common::SharedPtr<User> msg
            ) const
        {
            auto cb=[requestCtx,callback{std::move(callback)}](auto, common::SharedPtr<User> user, const Error& ec)
            {                
                auto& req=requestCtx->template get<RequestT>();
                if (ec)
                {
                    //! @todo Log error
                    // handler error
                    req.response.setStatus(HATN_API_NAMESPACE::protocol::ResponseStatus::ExecFailed,ec);
                }
                else
                {
                    // fill response
                    req.response.unit.field(HATN_API_NAMESPACE::protocol::response::message).set(std::move(user));
                }
                callback(std::move(requestCtx));
            };

            //! @todo Use topic from request
            this->controller()->addUninitializedUser(
                requestCtx,
                std::move(cb),
                msg
            );
        }

        HATN_VALIDATOR_NAMESPACE::error_report validate(
                const common::SharedPtr<HATN_API_SERVER_NAMESPACE::RequestContext<RequestT>>& /*request*/,
                const User& /*msg*/
            ) const
        {
            //! @todo Implement message validation
            return HATN_VALIDATOR_NAMESPACE::error_report{};
        }
};

template <typename RequestT>
class AddUser : public HATN_API_SERVER_NAMESPACE::ServiceMethodV<HATN_API_SERVER_NAMESPACE::ServiceMethodT<AddUserTraits<RequestT>,User>>
{
    public:

        using Base=HATN_API_SERVER_NAMESPACE::ServiceMethodV<HATN_API_SERVER_NAMESPACE::ServiceMethodT<AddUserTraits<RequestT>,User>>;

        AddUser(std::shared_ptr<Controller<RequestT>> controller) : Base(UserServiceConstants::AddUser,std::move(controller))
        {}
};

}

HATN_ADMIN_SERVER_NAMESPACE_END

#endif // HATNUSERERVICEADDUSER_H
