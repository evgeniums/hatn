/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/server/service.h
  *
  */

/****************************************************************************/

#ifndef HATNAPISERVERSERVICE_H
#define HATNAPISERVERSERVICE_H

#include <hatn/api/api.h>
#include <hatn/api/service.h>
#include <hatn/api/server/request.h>

HATN_API_NAMESPACE_BEGIN

namespace server
{

class Service : public HATN_API_NAMESPACE::Service
{
    public:

        using HATN_API_NAMESPACE::Service::Service;

        template <typename RequestT=api::server::Request<>>
        void handleRequest(
            common::SharedPtr<api::server::RequestContext<RequestT>> request,
            api::server::RouteCb<RequestT> callback
        );
};

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPISERVERSERVICE_H
