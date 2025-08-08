/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/client/resolver.h
  *
  */

/****************************************************************************/

#ifndef HATNAPIRESOLVER_H
#define HATNAPIRESOLVER_H

#include <hatn/common/error.h>
#include <hatn/common/sharedptr.h>
#include <hatn/common/objecttraits.h>

#include <hatn/network/asio/caresolver.h>

#include <hatn/api/api.h>

HATN_API_NAMESPACE_BEGIN

namespace client {

template <typename ResolvedEndpoints>
using ResolverCallbackFn=std::function<void (const Error&, ResolvedEndpoints)>;

template <typename Traits>
class Resolver : public common::WithTraits<Traits>
{
    public:

        using common::WithTraits<Traits>::WithTraits;

        using ResolvedEndpoints=typename Traits::ResolvedEndpoints;
        using ServerName=typename Traits::ServerName;

        void resolve(
                const common::TaskContextShared& ctx,
                ResolverCallbackFn<ResolvedEndpoints> callback,
                const ServerName& serverName
            )
        {
            this->traits().resolve(ctx,std::move(callback),serverName);
        }
};

}

HATN_API_NAMESPACE_END

#endif // HATNAPIRESOLVER_H
