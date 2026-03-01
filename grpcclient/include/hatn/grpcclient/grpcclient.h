/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file grpcclient/grpcclient.h
  *
  */

/****************************************************************************/

#ifndef HATNGRPCCLIENT_H
#define HATNGRPCCLIENT_H

#include <hatn/api/client/client.h>
#include <hatn/grpcclient/grpctransport.h>

HATN_GRPCCLIENT_NAMESPACE_BEGIN

struct DefaultGrpcClientTraits : public HATN_API_NAMESPACE::client::DefaultClientTraits
{
    using MessageType=HATN_API_NAMESPACE::Message<hana::false_,HATN_DATAUNIT_NAMESPACE::WireData>;
};

template <typename Router, typename Traits>
using GrpcTransportWrapper=GrpcTransport;

template <typename RouterT, typename SessionWrapperT, typename Traits=DefaultGrpcClientTraits>
using GrpcClient=HATN_API_NAMESPACE::client::Client<RouterT,GrpcTransportWrapper,SessionWrapperT,Traits>;

HATN_GRPCCLIENT_NAMESPACE_END

#endif // HATNGRPCCLIENT_H
