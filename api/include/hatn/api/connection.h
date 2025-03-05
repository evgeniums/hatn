/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/connection.h
  *
  */

/****************************************************************************/

#ifndef HATNAPISCONNECTION_H
#define HATNAPISCONNECTION_H

#include <hatn/common/sharedptr.h>
#include <hatn/common/spanbuffer.h>
#include <hatn/common/taskcontext.h>
#include <hatn/common/streamchaingather.h>

#include <hatn/api/api.h>

HATN_API_NAMESPACE_BEGIN

template <typename ... Streams>
class Connection : public common::TaskSubcontext
{
    public:

        template <typename ContextT>
        using CallbackT=std::function<void (common::SharedPtr<ContextT> ctx,const Error& ec)>;

        Connection()
        {}

        template <typename ...Ts>
        Connection(
                Ts... streams
            ) : m_stream(streams...)
        {
        }

        template <typename ...Ts>
        void setStreams(Ts... streams)
        {
            m_stream.setStreams(streams...);
        }

        void connect(
            std::function<void (const Error &)> callback
        )
        {
            m_stream.prepare(std::move(callback));
        }

        template <typename ContextT, typename CallbackT>
        void send(
            common::SharedPtr<ContextT> ctx,
            common::SpanBuffers buffers,
            CallbackT callback
        )
        {
            auto cb=[ctx{std::move(ctx)},callback{std::move(callback)}](const Error& ec, size_t sentBytes, common::SpanBuffers buffers)
            {
                callback(ctx,ec,sentBytes,std::move(buffers));
            };
            m_stream.write(std::move(buffers),std::move(cb));
        }

        template <typename ContextT, typename CallbackT>
        void send(
            common::SharedPtr<ContextT> ctx,
            const char* data,
            size_t size,
            CallbackT callback
            )
        {
            auto cb=[ctx{std::move(ctx)},callback{std::move(callback)}](const Error& ec, size_t sentBytes)
            {
                callback(ctx,ec,sentBytes);
            };
            m_stream.write(data,size,std::move(cb));
        }

        template <typename ContextT, typename CallbackT>
        void recv(
            common::SharedPtr<ContextT> ctx,
            char* data,
            size_t size,
            CallbackT callback
        )
        {
            auto cb=[ctx{std::move(ctx)},callback{std::move(callback)}](const Error& ec, size_t)
            {
                callback(ctx,ec);
            };
            m_stream.readAll(data,size,std::move(cb));
        }

        template <typename ContextT, typename CallbackT>
        void read(
            common::SharedPtr<ContextT> ctx,
            char* data,
            size_t maxSize,
            CallbackT callback
        )
        {
            auto cb=[ctx{std::move(ctx)},callback{std::move(callback)}](const Error& ec, size_t readBytes)
            {
                callback(ctx,ec,readBytes);
            };
            m_stream.read(data,maxSize,std::move(cb));
        }

        void close(std::function<void (const Error &)> callback={})
        {
            m_stream.close(std::move(callback));
        }

        template <typename ContextT, typename CallbackT>
        void waitForRead(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback
        )
        {
            auto cb=[ctx{std::move(ctx)},callback{std::move(callback)}](const Error& ec)
            {
                callback(ctx,ec);
            };
            m_stream.waitForRead(std::move(cb));
        }

        void cancel()
        {
            m_stream.cancel();
        }

    private:

        common::StreamChainGather<Streams...> m_stream;
};

HATN_API_NAMESPACE_END

#endif // HATNAPISCONNECTION_H
