/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file network/reliablestream.h
  *
  *   Base class for reliable streams
  *
  */

/****************************************************************************/

#ifndef HATNRELIABLESTREAM_H
#define HATNRELIABLESTREAM_H

#include <functional>

#include <hatn/common/error.h>
#include <hatn/common/objectid.h>
#include <hatn/common/stream.h>

#include <hatn/network/network.h>
#include <hatn/network/endpoint.h>

HATN_NETWORK_NAMESPACE_BEGIN

//! Reliable stream
template <typename Traits> using ReliableStream=common::StreamWithIDThread<Traits>;

//! Reliable stream with endpoints
template <typename EndpointT,typename Traits> class ReliableStreamWithEndpoints :
        public ReliableStream<Traits>,
        public WithLocalEndpoint<EndpointT>,
        public WithRemoteEndpoint<EndpointT>
{
    public:

        using ReliableStream<Traits>::ReliableStream;
};


HATN_NETWORK_NAMESPACE_END

#endif // HATNRELIABLESTREAM_H
