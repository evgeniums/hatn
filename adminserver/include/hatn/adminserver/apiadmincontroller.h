/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file adminserver/apiadmincontroller.h
  */

/****************************************************************************/

#ifndef HATNAPIADMINCONTROLLER_H
#define HATNAPIADMINCONTROLLER_H

#include <hatn/api/server/serverrequest.h>

#include <hatn/adminserver/adminserver.h>
#include <hatn/adminserver/localadmincontroller.h>

HATN_ADMIN_SERVER_NAMESPACE_BEGIN

template <typename RequestT=HATN_API_NAMESPACE::server::Request<>>
class ApiRequestTraits
{
    public:

        using Request=RequestT;
        using RequestContext=HATN_API_NAMESPACE::server::RequestContext<Request>;

        ApiRequestTraits(std::string adminTopic="admin"):m_adminTopic(std::move(adminTopic))
        {}

        const std::shared_ptr<db::AsyncClient>& adminDb(const common::SharedPtr<RequestContext>& request) const
        {
            const auto& req=request->template get<Request>();
            const auto& db=req.env->template get<api::server::Db>();
            const auto& client=db.dbClient(m_adminTopic);
            return client;
        }

        db::Topic adminTopic(const common::SharedPtr<RequestContext>& ={}) const
        {
            return m_adminTopic;
        }

    private:

        std::string m_adminTopic;
};

using ApiAdminController=LocalAdminControllerT<ApiRequestTraits<>>;

HATN_ADMIN_SERVER_NAMESPACE_END

#endif // HATNAPIADMINCONTROLLER_H
