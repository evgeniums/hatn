/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file adminserver/adminservice.h
  */

/****************************************************************************/

#ifndef HATNADMINSERVICEMETHODADDADMIN_H
#define HATNADMINSERVICEMETHODADDADMIN_H

#include <hatn/api/server/serverservice.h>

#include <hatn/adminclient/admin.h>
#include <hatn/adminclient/adminserviceconfig.h>

#include <hatn/adminserver/adminserver.h>
#include <hatn/adminserver/methods/basemethod.h>

HATN_ADMIN_SERVER_NAMESPACE_BEGIN

namespace methods {

template <typename RequestT>
class AddAdminTraits : public BaseTraits
{
    public:

        AddAdminTraits(std::shared_ptr<ApiAdminController> controller)
            : BaseTraits(std::move(controller))
        {}

        template <typename CallbackT>
        void exec(
                common::SharedPtr<HATN_API_SERVER_NAMESPACE::RequestContext<RequestT>> requestCtx,
                CallbackT callback,
                common::SharedPtr<Admin> msg
            ) const
        {
            auto cb=[requestCtx,callback{std::move(callback)}](auto, common::SharedPtr<Admin> admin, const Error& ec)
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
                    req.response.unit.field(HATN_API_NAMESPACE::protocol::response::message).set(std::move(admin));
                }
                callback(std::move(requestCtx));
            };
            controller()->addAdmin(
                requestCtx,
                std::move(cb),
                msg
            );
        }

        HATN_VALIDATOR_NAMESPACE::error_report validate(
                const common::SharedPtr<HATN_API_SERVER_NAMESPACE::RequestContext<RequestT>>& /*request*/,
                const Admin& /*msg*/
            ) const
        {
            //! @todo Implement message validation
            return HATN_VALIDATOR_NAMESPACE::error_report{};
        }
};

template <typename RequestT>
class AddAdmin : public HATN_API_SERVER_NAMESPACE::ServiceMethodV<HATN_API_SERVER_NAMESPACE::ServiceMethodT<AddAdminTraits<RequestT>,Admin>>
{
    public:

        using Base=HATN_API_SERVER_NAMESPACE::ServiceMethodV<HATN_API_SERVER_NAMESPACE::ServiceMethodT<AddAdminTraits<RequestT>,Admin>>;

        AddAdmin(std::shared_ptr<ApiAdminController> controller) : Base(AdminServiceConfig::AddAdmin,std::move(controller))
        {}
};

}

HATN_ADMIN_SERVER_NAMESPACE_END

#endif // HATNADMINSERVICEMETHODADDADMIN_H
