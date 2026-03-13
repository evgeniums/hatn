/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/client/streamchannel.h
  *
  */

/****************************************************************************/

#ifndef HATNAPICLIENTSTREAMCHANNEL_H
#define HATNAPICLIENTSTREAMCHANNEL_H

#include <hatn/common/error.h>
#include <hatn/common/bytearray.h>

#include <hatn/api/api.h>

HATN_API_NAMESPACE_BEGIN

namespace client
{

class Response;

class HATN_API_EXPORT StreamChannel
{
    public:

        using ReadCb=std::function<void (const Error&, Response response)>;
        using WriteCb=std::function<void (const Error&)>;
        using CloseCb=std::function<void (const Error&)>;

        StreamChannel()=default;

        virtual ~StreamChannel();

        StreamChannel(const StreamChannel&)=default;
        StreamChannel(StreamChannel&&)=default;
        StreamChannel& operator=(const StreamChannel&)=default;
        StreamChannel& operator=(StreamChannel&&)=default;

        virtual void readNext(ReadCb callback) =0;
        virtual void writeNext(common::ByteArrayShared message, std::string messageType, WriteCb callback) =0;

        virtual void close(CloseCb callback) =0;
};

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPICLIENTSTREAMCHANNEL_H
