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
#include <hatn/api/server/request.h>

HATN_API_NAMESPACE_BEGIN

namespace server {

template <typename Traits, typename EnvT=SimpleEnv>
class RequestRouter : public common::WithTraits<Traits>
{
    public:

        using Env=EnvT;
        using common::WithTraits<Traits>::WithTraits;

        void route(
            common::SharedPtr<RequestContext<Env>> request,
            RouteCb<Env> cb
        )
        {
            this->traits().route(std::move(request),std::move(cb));
        }
};

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPISERVERREQUESTROUTER_H
