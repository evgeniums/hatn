/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*
    
*/
/** @file adminserver/requesttraits.h
  */

/****************************************************************************/

#ifndef HATNADMINREQUESTTRAITS_H
#define HATNADMINREQUESTTRAITS_H

#include <hatn/api/server/serverrequest.h>

#include <hatn/adminserver/adminserver.h>

HATN_ADMIN_SERVER_NAMESPACE_BEGIN

template <typename RequestT=HATN_API_SERVER_NAMESPACE::Request<>>
class RequestTraits
{
    public:

        using Request=RequestT;
        using Context=HATN_API_SERVER_NAMESPACE::RequestContext<Request>;

        static auto& contextDb(common::SharedPtr<Context>& ctx)
        {
            auto& req=ctx->template get<Request>();
            return req.db();
        }
};

HATN_ADMIN_SERVER_NAMESPACE_END

#endif // HATNADMINREQUESTTRAITS_H
