/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/server/request.h
  *
  */

/****************************************************************************/

#ifndef HATNAPISERVERREQUEST_H
#define HATNAPISERVERREQUEST_H

#include <functional>

#include <hatn/common/objecttraits.h>
#include <hatn/common/error.h>
#include <hatn/common/sharedptr.h>

#include <hatn/api/api.h>
#include <hatn/api/requestunit.h>
#include <hatn/api/server/response.h>
#include <hatn/api/server/env.h>

HATN_API_NAMESPACE_BEGIN

namespace server {

template <typename EnvT=SimpleEnv>
class Request : public common::TaskSubcontext
{
    using Env=EnvT;

    common::StringOnStack subject;
    request::managed unit;
    common::ByteArrayShared buf;
    common::WeakPtr<common::TaskContext> connectionCtx;

    Response response;

    common::SharedPtr<Env> env;
};

template <typename EnvT=SimpleEnv>
using RequestContext=Request<EnvT>;

template <typename EnvT=SimpleEnv>
using RouteCb=std::function<void (common::SharedPtr<RequestContext<EnvT>> request)>;

template <typename EnvT=SimpleEnv>
using RouteFh=std::function<void (common::SharedPtr<RequestContext<EnvT>> request, RouteCb<EnvT> cb)>;

template <typename Traits, typename EnvT=SimpleEnv>
class RequestsRouter : public common::WithTraits<Traits>
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

#endif // HATNAPISERVERREQUEST_H
