/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/client/rawtransportclient.h
  *
  */

/****************************************************************************/

#ifndef HATNAPIRAWTRANSPORTCLIENT_H
#define HATNAPIRAWTRANSPORTCLIENT_H

#include <hatn/api/client/client.h>
#include <hatn/api/client/rawtransport.h>

HATN_API_NAMESPACE_BEGIN

namespace client {

template <typename RouterT, typename SessionWrapperT, typename Traits=DefaultClientTraits>
using RawTransportClient=Client<RouterT,RawTransport,SessionWrapperT,Traits>;

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPIRAWTRANSPORTCLIENT_H
