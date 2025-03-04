/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/router.h
  *
  */

/****************************************************************************/

#ifndef HATNAPIROUTER_H
#define HATNAPIROUTER_H

#include <hatn/common/error.h>
#include <hatn/common/sharedptr.h>
#include <hatn/common/objecttraits.h>

#include <hatn/api/api.h>

HATN_API_NAMESPACE_BEGIN

template <typename ConnectionContext>
using RouterCallbackFn=std::function<void (const Error&, common::SharedPtr<ConnectionContext>)>;

template <typename Traits>
class Router : public common::WithTraits<Traits>
{
    public:

        using common::WithTraits<Traits>::WithTraits;

        using ConnectionContext=typename Traits::ConnectionContext;
        using Connection=typename Traits::Connection;

        template <typename ContextT>
        void makeConnection(
                common::SharedPtr<ContextT> ctx,
                RouterCallbackFn<ConnectionContext> callback,
                lib::string_view name={}
            )
        {
            this->traits().makeConnection(std::move(ctx),std::move(callback),name);
        }
};

HATN_API_NAMESPACE_END

#endif // HATNAPIROUTER_H
