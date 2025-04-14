/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/server/authdispatcher.h
  *
  */

/****************************************************************************/

#ifndef HATNAPIAUTHDISPATCHER_H
#define HATNAPIAUTHDISPATCHER_H

#include <hatn/api/api.h>
#include <hatn/api/server/servicedispatcher.h>

HATN_API_NAMESPACE_BEGIN

namespace server {

//! @todo Implement auth dispatcher

template <typename EnvT=BasicEnv, typename RequestT=Request<EnvT>>
using AuthDispatcherTraits=ServiceDispatcherTraits<EnvT,RequestT>;

template <typename EnvT=BasicEnv, typename RequestT=Request<EnvT>>
using AuthDispatcher=Dispatcher<ServiceDispatcherTraits<EnvT,RequestT>,EnvT,RequestT>;

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPIAUTHDISPATCHER_H
