/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/server/requestrouter.h
  *
  */

/****************************************************************************/

#ifndef HATNAPISERVERREQUESTROUTER_H
#define HATNAPISERVERREQUESTROUTER_H

#include <hatn/api/api.h>
#include <hatn/api/server/serverrequest.h>

HATN_API_NAMESPACE_BEGIN

namespace server {

template <typename Traits, typename EnvT=BasicEnv, typename RequestT=Request<EnvT>>
class RequestRouter : public common::WithTraits<Traits>
{
    public:

        using Env=EnvT;
        using Request=RequestT;

        using common::WithTraits<Traits>::WithTraits;

        void route(
            common::SharedPtr<RequestContext<Request>> request,
            RouteCb<Request> cb
        )
        {
            this->traits().route(std::move(request),std::move(cb));
        }
};

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPISERVERREQUESTROUTER_H
