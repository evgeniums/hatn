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

        template <typename ContextT, typename CallbackT>
        void send(
            common::SharedPtr<ContextT> ctx,
            common::SpanBuffers buffers,
            CallbackT callback
        )
        {
            auto cb=[ctx,callback](const Error& ec,size_t,common::SpanBuffers)
            {
                callback(ctx,ec);
            };
            m_stream.write(std::move(buffers),std::move(cb));
        }

    private:

        common::StreamChainGather<Streams...> m_stream;
};

HATN_API_NAMESPACE_END

#endif // HATNAPISCONNECTION_H
