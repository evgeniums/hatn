/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
    
  */

/****************************************************************************/

/** @file network/asio/tcpstream.h
  *
  *   Stream over ASIO TCP socket
  *
  */

/****************************************************************************/

#ifndef HATNASIOTCPSTREAM_H
#define HATNASIOTCPSTREAM_H

#include <hatn/common/taskcontext.h>

#include <hatn/logcontext/context.h>

#include <hatn/network/network.h>
#include <hatn/network/reliablestream.h>

#include <hatn/network/asio/ipendpoint.h>
#include <hatn/network/asio/socket.h>

HATN_NETWORK_NAMESPACE_BEGIN

namespace asio {

class TcpStream;

//! TcpStream handler traits
class HATN_NETWORK_EXPORT TcpStreamTraits : public WithSocket<TcpSocket>
{
    public:

        //! Ctor
        TcpStreamTraits(
            TcpStream* stream
        );

        //! Check if stream is open
        bool isOpen() const
        {
            return rawSocket().is_open();
        }

        /**
         * @brief Prepare stream: open, bind and connect ASIO socket
         * @param callback Status of stream preparation
         */
        void prepare(
            std::function<void (const common::Error &)> callback
        );

        //! Close stream
        void close(const std::function<void (const common::Error &)>& callback, bool destroying=false);

        //! Write to stream
        void write(
            const char* data,
            std::size_t size,
            std::function<void (const common::Error&,size_t)> callback
        );

        void write(
            common::SpanBuffers buffers,
            std::function<void (const common::Error&,size_t,common::SpanBuffers)> callback
        );

        //! Read from stream
        void read(
            char* data,
            std::size_t maxSize,
            std::function<void (const common::Error&,size_t)> callback
        );

        const char* idStr() const noexcept;

        void cancel();

        void reset()
        {}

    private:

        TcpStream* m_stream;

        inline common::WeakPtr<common::TaskContext> ctxWeakPtr() const;
        inline void leaveAsyncHandler();
};

//! Stream over ASIO TCP socket
class HATN_NETWORK_EXPORT TcpStream : public common::TaskSubcontext,
                                      public ReliableStreamWithEndpoints<TcpEndpoint,TcpStreamTraits>
{
    public:

        //! Constructor
        TcpStream(
            common::Thread* thread=common::Thread::currentThread() //!< Thread the socket lives in
        );

        ~TcpStream()=default;
        TcpStream(const TcpStream&)=delete;
        TcpStream(TcpStream&&) =delete;
        TcpStream& operator=(const TcpStream&)=delete;
        TcpStream& operator=(TcpStream&&) =delete;

        //! Update endpoints
        inline void updateEndpoints()
        {
            try
            {
                localEndpoint().fromBoostEndpoint(socket().socket().local_endpoint());
                remoteEndpoint().fromBoostEndpoint(socket().socket().remote_endpoint());
            }
            catch (const boost::system::system_error&)
            {
            }
        }

        //! Get socket
        inline TcpSocket& socket() noexcept
        {
            return traits().socket();
        }
};

struct makeTcpStreamCtxT
{
    template <typename ...BaseArgs>
    auto operator()(common::Thread* thread, BaseArgs&&... args) const
    {
        return common::makeTaskContext<TcpStream,HATN_LOGCONTEXT_NAMESPACE::LogCtxType>(
                common::subcontexts(
                    common::subcontext(thread)
                ),
                common::subcontexts(
                    common::subcontext()
                ),
                std::forward<BaseArgs>(args)...
            );
    }

    template <typename ...BaseArgs>
    auto operator()(BaseArgs&&... args) const
    {
        return common::makeTaskContext<TcpStream,HATN_LOGCONTEXT_NAMESPACE::LogCtxType>(
                common::subcontexts(
                    common::subcontext()
                    ),
                common::subcontexts(
                    common::subcontext()
                    ),
                std::forward<BaseArgs>(args)...
            );
    }

    auto operator()() const
    {
        return common::makeTaskContext<TcpStream,HATN_LOGCONTEXT_NAMESPACE::LogCtxType>();
    }
};
constexpr makeTcpStreamCtxT makeTcpStreamCtx{};
using TcpStreamSharedCtx=decltype(makeTcpStreamCtx(""));

} // namespace asio

HATN_NETWORK_NAMESPACE_END

HATN_TASK_CONTEXT_DECLARE(HATN_NETWORK_NAMESPACE::asio::TcpStream,HATN_NETWORK_EXPORT)

#endif // HATNASIOTCPSTREAM_H
