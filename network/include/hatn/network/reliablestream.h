/*
   Copyright (c) 2019 - current, Evgeny Sidorov (esid1976@gmail.com), All rights reserved
    
    
  */

/****************************************************************************/
/*
    
*/
/** \file network/reliablestream.h
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

namespace hatn {
namespace network {

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

} // namespace network
} // namespace hatn

#endif // HATNRELIABLESTREAM_H
