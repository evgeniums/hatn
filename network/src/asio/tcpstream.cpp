/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file network/asiotcpstream.cpp
  *
  *   Stream over ASIO TCP socket
  *
  */

/****************************************************************************/

#include <memory>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"

#endif

#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

#ifdef __GNUC__
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop
#endif

#include <hatn/common/logger.h>
#include <hatn/common/thread.h>

#include <hatn/logcontext/contextlogger.h>

#include <hatn/network/asio/tcpstream.h>

#include <hatn/network/asio/detail/asiohandler.ipp>

HATN_TASK_CONTEXT_DEFINE(HATN_NETWORK_NAMESPACE::asio::TcpStream,TcpStream)

HATN_NETWORK_NAMESPACE_BEGIN
HATN_COMMON_USING

namespace {

constexpr const char* LogModule="asiotcpstream";

}

namespace asio {

/*********************** TcpStream **************************/

//---------------------------------------------------------------
TcpStream::TcpStream(
        Thread *thread
    ) : ReliableStreamWithEndpoints<TcpEndpoint,TcpStreamTraits>(
            thread,
            this
        )
{
}

/*********************** TcpStreamTraits **************************/

//---------------------------------------------------------------
TcpStreamTraits::TcpStreamTraits(TcpStream *stream):
    WithSocket<TcpSocket>(stream->thread()->asioContextRef()),
    m_stream(stream)
{
}

//---------------------------------------------------------------
void TcpStreamTraits::cancel()
{
    boost::system::error_code ec;
    rawSocket().cancel(ec);
    if (ec)
    {
        HATN_CTX_WARN_RECORDS_M("failed to cancel TCP socket",LogModule,{"err_code",ec.value()},{"err_msg",ec.message()})
    }
}

//---------------------------------------------------------------
void TcpStreamTraits::close(const std::function<void (const Error &)> &callback, bool destroying)
{
    HATN_CTX_SCOPE("tcpstreamclose")

    common::Error ret;
    if (rawSocket().lowest_layer().is_open())
    {
        try
        {
            boost::system::error_code ec;

            if (!destroying)
            {
                rawSocket().shutdown(boost::asio::socket_base::shutdown_both,ec);
                if (ec)
                {
                    HATN_CTX_WARN_RECORDS_M("failed to shutdown",LogModule,{"err_code",ec.value()},{"err_msg",ec.message()})
                }
            }

            rawSocket().lowest_layer().close(ec);
            if (ec)
            {
                HATN_CTX_WARN_RECORDS_M("failed to close",LogModule,{"err_code",ec.value()},{"err_msg",ec.message()})
                throw boost::system::system_error(ec);
            }
            HATN_CTX_DEBUG("closed",LogModule)
        }
        catch (const boost::system::system_error& e)
        {
            ret=makeBoostError(e.code());
        }
    }

    m_stream->mainCtx().leaveAsyncHandler();
    m_stream->thread()->execAsync(
        [callback{std::move(callback)},wptr{ctxWeakPtr()},ret{std::move(ret)}]()
        {
            if (detail::enterHandler(wptr,callback))
            {
                callback(ret);
            }
        }
    );
}

//---------------------------------------------------------------
void TcpStreamTraits::prepare(
        std::function<void (const Error &)> callback
    )
{
    HATN_CTX_SCOPE("tcpstreamprepare")

    boost::system::error_code ec;

    bool bind=false;
    auto ep=m_stream->remoteEndpoint().toBoostEndpoint();
    HATN_CTX_SCOPE_PUSH("remote_ip",ep.address().to_string())
    HATN_CTX_SCOPE_PUSH("remote_port",ep.port())

    // bind socket if local port is set
    if (m_stream->localEndpoint().port()!=0)
    {
        bind=true;
        auto localEp=m_stream->localEndpoint().toBoostEndpoint();
        auto protocol=m_stream->localEndpoint().address().is_v6()?boost::asio::ip::tcp::v6():boost::asio::ip::tcp::v4();

        HATN_CTX_SCOPE_PUSH("local_ip",m_stream->localEndpoint().address().to_string())
        HATN_CTX_SCOPE_PUSH("local_port",m_stream->localEndpoint().port())

        rawSocket().open(protocol,ec);
        if (!ec)
        {
            rawSocket().bind(localEp,ec);
            if (!ec)
            {
                HATN_CTX_DEBUG("local socket bound",LogModule);
            }
            else
            {
                HATN_CTX_SCOPE_ERROR("socket-bind")
            }
        }
        else
        {
            HATN_CTX_SCOPE_ERROR("socket-open")
        }
    }

    // return if error
    if (ec)
    {
        m_stream->mainCtx().leaveAsyncHandler();
        m_stream->thread()->execAsync(
            [callback{std::move(callback)},wptr{ctxWeakPtr()},ec{std::move(ec)}]()
            {
                if (detail::enterHandler(wptr,callback))
                {
                    callback(makeBoostError(ec));
                }
            }
        );
        return;
    }

    // connect to remote endpoint
    m_stream->mainCtx().leaveAsyncHandler();
    rawSocket().async_connect(
                    ep,
                    [bind,callback{std::move(callback)},ep,wptr{ctxWeakPtr()},this](const boost::system::error_code &ec)
                    {
                        if (!detail::enterHandler(wptr,callback))
                        {
                            return;
                        }

                        HATN_CTX_SCOPE("connect-cb")

                        if (!ec)
                        {
                            m_stream->updateEndpoints();
                            if (!bind)
                            {
                                HATN_CTX_SCOPE_PUSH("local_ip",m_stream->localEndpoint().address().to_string())
                                HATN_CTX_SCOPE_PUSH("local_port",m_stream->localEndpoint().port())
                            }
                            HATN_CTX_DEBUG("connected",LogModule);
                        }
                        else
                        {
                            HATN_CTX_SCOPE_ERROR("socket-connect")
                            if (rawSocket().lowest_layer().is_open())
                            {
                                boost::system::error_code ec1;
                                rawSocket().lowest_layer().close(ec1);
                                if (ec1)
                                {
                                    HATN_CTX_WARN_RECORDS_M("failed to close",LogModule,{"err_code",ec1.value()},{"err_msg",ec1.message()})
                                }
                            }
                        }

                        callback(makeBoostError(ec));
                    }
                );
}

//---------------------------------------------------------------
void TcpStreamTraits::read(
        char* buf,
        std::size_t maxSize,
        std::function<void (const common::Error &, size_t)> callback
    )
{
    if (!m_stream->isActive())
    {
        m_stream->mainCtx().leaveAsyncHandler();
        m_stream->thread()->execAsync(
            [callback{std::move(callback)},wptr{ctxWeakPtr()}]()
            {
                if (detail::enterHandler(wptr,callback,0))
                {
                    callback(makeBoostError(boost::asio::error::eof),0);
                }
            }
        );
        return;
    }

    if (buf==nullptr || maxSize==0)
    {
        m_stream->mainCtx().leaveAsyncHandler();
        m_stream->thread()->execAsync(
            [callback{std::move(callback)},wptr{ctxWeakPtr()}]()
            {
                if (detail::enterHandler(wptr,callback,0))
                {
                    callback(common::Error(),0);
                }
            }
        );
        return;
    }

    m_stream->mainCtx().leaveAsyncHandler();
    rawSocket().async_receive(
                boost::asio::buffer(buf,maxSize),
                [callback{std::move(callback)},wptr{ctxWeakPtr()},this](const boost::system::error_code &ec, size_t size)
                    {
                        if (detail::enterHandler(wptr,callback,0))
                        {
                            callback(makeBoostError(ec),size);
                        }
                    }
                );
}

//---------------------------------------------------------------
void TcpStreamTraits::write(
        const char* buf,
        std::size_t size,
        std::function<void (const common::Error &, size_t)> callback
    )
{
    if (!m_stream->isActive())
    {
        m_stream->mainCtx().leaveAsyncHandler();
        m_stream->thread()->execAsync(
            [callback{std::move(callback)},wptr{ctxWeakPtr()}]()
            {
                if (detail::enterHandler(wptr,callback,0))
                {
                    callback(makeBoostError(boost::asio::error::eof),0);
                }
            }
        );
        return;
    }

    if (buf==nullptr || size==0)
    {
        m_stream->mainCtx().leaveAsyncHandler();
        m_stream->thread()->execAsync(
            [callback{std::move(callback)},wptr{ctxWeakPtr()}]()
            {
                if (detail::enterHandler(wptr,callback,0))
                {
                    callback(common::Error(),0);
                }
            }
        );
        return;
    }

    m_stream->mainCtx().leaveAsyncHandler();
    rawSocket().async_send(
                    boost::asio::buffer(buf,size),
                    [callback{std::move(callback)},wptr{ctxWeakPtr()},this](const boost::system::error_code &ec, size_t size)
                    {
                        if (detail::enterHandler(wptr,callback,0))
                        {
                            callback(makeBoostError(ec),size);
                        }
                    }
                );
}

//---------------------------------------------------------------
void TcpStreamTraits::write(
        SpanBuffers buffers,
        std::function<void (const Error &, size_t, SpanBuffers)> callback
    )
{
    if (!m_stream->isActive())
    {
        m_stream->mainCtx().leaveAsyncHandler();
        m_stream->thread()->execAsync(
            [callback{std::move(callback)},wptr{ctxWeakPtr()},buffers{std::move(buffers)}]()
            {
                if (detail::enterHandler(wptr,callback,std::move(buffers)))
                {
                    callback(makeBoostError(boost::asio::error::eof),0,std::move(buffers));
                }
            }
        );
        return;
    }

    if (buffers.empty())
    {
        m_stream->mainCtx().leaveAsyncHandler();
        m_stream->thread()->execAsync(
            [callback{std::move(callback)},wptr{ctxWeakPtr()},buffers{std::move(buffers)}]()
            {
                if (detail::enterHandler(wptr,callback,std::move(buffers)))
                {
                    callback(common::Error(),0,std::move(buffers));
                }
            }
        );
        return;
    }

    pmr::vector<boost::asio::const_buffer> asioBuffers;
    if (!fillAsioBuffers(buffers,asioBuffers))
    {
        m_stream->mainCtx().leaveAsyncHandler();
        m_stream->thread()->execAsync(
            [callback{std::move(callback)},wptr{ctxWeakPtr()},buffers{std::move(buffers)}]()
            {
                if (detail::enterHandler(wptr,callback,std::move(buffers)))
                {
                    callback(common::Error(CommonError::INVALID_SIZE),0,std::move(buffers));
                }
            }
        );
        return;
    }

    m_stream->mainCtx().leaveAsyncHandler();
    rawSocket().async_send(
                    asioBuffers,
                    [callback{std::move(callback)},wptr{ctxWeakPtr()},buffers{std::move(buffers)},this](const boost::system::error_code &ec, size_t size)
                    {
                        if (detail::enterHandler(wptr,callback,std::move(buffers)))
                        {
                            callback(makeBoostError(ec),size,std::move(buffers));
                        }
                    }
                );
}

//---------------------------------------------------------------

common::WeakPtr<common::TaskContext> TcpStreamTraits::ctxWeakPtr() const
{
    return toWeakPtr(m_stream->mainCtx().sharedFromThis());
}

//---------------------------------------------------------------
} // namespace asio

HATN_NETWORK_NAMESPACE_END
