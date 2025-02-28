/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/client/session.h
  *
  */

/****************************************************************************/

#ifndef HATNAPICLIENTSESSION_H
#define HATNAPICLIENTSESSION_H

#include <functional>

#include <hatn/common/objecttraits.h>
#include <hatn/common/sharedptr.h>
#include <hatn/common/error.h>

#include <hatn/api/api.h>
#include <hatn/api/authunit.h>

HATN_API_NAMESPACE_BEGIN

namespace client {

template <typename ContextT>
using SessionCb=std::function<void (common::SharedPtr<ContextT> ctx, common::SharedPtr<AuthManaged> auth, const common::Error&)>;

template <typename Traits>
class Session : public common::WithTraits<Traits>
{
    public:

        using common::WithTraits<Traits>::WithTraits;

        template <typename ContextT>
        void getCurrentAuth(
                common::SharedPtr<ContextT> ctx,
                SessionCb<ContextT> cb
            )
        {
            this->traits().getCurrentAuth(std::move(ctx),std::move(cb));
        }
};

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPICLIENTSESSION_H
