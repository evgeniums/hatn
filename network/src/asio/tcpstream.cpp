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

#include <hatn/common/thread.h>

#include <hatn/logcontext/contextlogger.h>

#include <hatn/network/networkerror.h>
#include <hatn/network/asio/tcpstream.h>

#include <hatn/network/detail/asynchandler.ipp>

HATN_TASK_CONTEXT_DEFINE(HATN_NETWORK_NAMESPACE::asio::TcpStream,TcpStream)

HATN_NETWORK_NAMESPACE_BEGIN
HATN_COMMON_USING

namespace asio {

/*********************** TcpStream **************************/

//---------------------------------------------------------------
TcpStream::TcpStream(
        Thread *thread
    ) : ReliableStreamWithEndpoints<TcpEndpoint,TcpStreamTraits>(
            thread,
            this
        ),
        m_connectTimeoutSecs(DefaultConnectTimeoutSecs)
{    
}

/*********************** TcpStreamTraits **************************/

//---------------------------------------------------------------
TcpStreamTraits::TcpStreamTraits(TcpStream *stream):
    WithSocket<TcpSocket>(stream->thread()->asioContextRef()),
    m_stream(stream),
    m_timeoutTimer(stream->thread())
{
    m_timeoutTimer.setAutoAsyncGuardEnabled(false);
    m_timeoutTimer.setSingleShot(true);
}

//---------------------------------------------------------------
void TcpStreamTraits::cancel()
{
    boost::system::error_code ec;
    rawSocket().cancel(ec);
    if (ec)
    {
        HATN_CTX_SCOPE("tcpstreamcancel")
        HATN_CTX_WARN_RECORDS_M("failed to cancel TCP socket",HatnAsioLog,{"err_code",ec.value()},{"err_msg",ec.message()})
    }
}

//---------------------------------------------------------------
void TcpStreamTraits::close(const std::function<void (const Error &)> &callback, bool destroying)
{
    if (destroying)
    {
        return;
    }

    HATN_CTX_SCOPE("tcpstreamclose")

    auto postThread=m_stream->thread()!=common::Thread::currentThread() || static_cast<bool>(callback);

    auto doClose=[callback{std::move(callback)},ctx{m_stream->sharedMainCtx()},this,postThread]()
    {
        std::ignore=ctx;
        if (postThread)
        {
            m_stream->mainCtx().onAsyncHandlerEnter();
        }

        {
            HATN_CTX_SCOPE("tcpstreamdoclose")

            common::Error ret;
            if (rawSocket().lowest_layer().is_open())
            {
                try
                {
                    boost::system::error_code ec;

                    rawSocket().shutdown(boost::asio::socket_base::shutdown_both,ec);
                    if (ec)
                    {
                        HATN_CTX_WARN_RECORDS_M("failed to shutdown",HatnAsioLog,{"err_code",ec.value()},{"err_msg",ec.message()})
                    }

                    rawSocket().lowest_layer().close(ec);
                    if (ec)
                    {
                        HATN_CTX_WARN_RECORDS_M("failed to close",HatnAsioLog,{"err_code",ec.value()},{"err_msg",ec.message()})
                        throw boost::system::system_error(ec);
                    }
                    HATN_CTX_DEBUG(DoneDebugVerbosity,"stream closed",HatnAsioLog)
                }
                catch (const boost::system::system_error& e)
                {
                    ret=makeBoostError(e.code());
                }
            }

            if (callback)
            {
                callback(ret);
            }
        }

        if (postThread)
        {
            m_stream->mainCtx().onAsyncHandlerExit();
        }
    };

    if (postThread)
    {
        m_stream->thread()->execAsync(doClose);
    }
    else
    {
        doClose();
    }
}

//---------------------------------------------------------------

void TcpStreamTraits::startTimeoutTimer()
{
    m_timeoutTimer.start(
        [this,wptr{ctxWeakPtr()}](TimerTypes::Status status)
        {
            auto ctx=wptr.lock();
            if (!ctx)
            {
                return;
            }

            if (status==TimerTypes::Status::Timeout)
            {
                rawSocket().cancel();
            }
        }
    );
}

//---------------------------------------------------------------

void TcpStreamTraits::prepare(
        std::function<void (const Error &)> callback
    )
{
    m_stream->mainCtx().setCurrentTaskCtxAsParent();

    HATN_CTX_SCOPE_WITH_BARRIER("tcpstreamprepare")

    if (isOpen())
    {
        close(std::function<void (const Error &)>{});
    }

    boost::system::error_code bec;

    bool bind=false;
    auto ep=m_stream->remoteEndpoint().toBoostEndpoint();
    HATN_CTX_SCOPE_PUSH("remote_ip",ep.address().to_string())
    HATN_CTX_SCOPE_PUSH("remote_port",ep.port())
    HATN_CTX_SCOPE_PUSH("local_port",m_stream->localEndpoint().port())

    // bind socket if local port is set
    if (m_stream->localEndpoint().port()!=0)
    {
        bind=true;
        auto localEp=m_stream->localEndpoint().toBoostEndpoint();
        auto protocol=m_stream->localEndpoint().address().is_v6()?boost::asio::ip::tcp::v6():boost::asio::ip::tcp::v4();

        HATN_CTX_SCOPE_PUSH("local_ip",m_stream->localEndpoint().address().to_string())        

        rawSocket().open(protocol,bec);
        if (!bec)
        {
            rawSocket().bind(localEp,bec);
            if (!bec)
            {
                HATN_CTX_DEBUG(DoneDebugVerbosity,"socket bound",HatnAsioLog);
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
    if (bec)
    {
        m_stream->thread()->execAsync(
            [callback{std::move(callback)},wptr{ctxWeakPtr()},bec{std::move(bec)},this]()
            {
                if (detail::enterAsyncHandler(wptr,callback))
                {
                    {
                        auto ec=common::chainErrors(makeBoostError(bec),networkError(NetworkError::CONNECT_FAILED));
                        HATN_CTX_LOG_DEBUG_ERROR(DoneDebugVerbosity,ec,"invalid local endpoint",HatnAsioLog);

                        HATN_CTX_STACK_BARRIER_OFF("tcpstreamprepare")
                        m_stream->mainCtx().resetParentCtx();
                        callback(ec);
                    }
                    leaveAsyncHandler();
                }
            }
        );
        return;
    }

    HATN_CTX_LOG_DEBUG(DoneDebugVerbosity,"trying to connect to server",HatnAsioLog);

    // start connection timer
    if (m_stream->connectTimeout()!=0)
    {
        m_timeoutTimer.setPeriodUs(m_stream->connectTimeout()*1000*1000);
        startTimeoutTimer();
    }

    // connect to remote endpoint
    rawSocket().async_connect(
                    ep,
                    [bind,callback{std::move(callback)},ep,wptr{ctxWeakPtr()},this](const boost::system::error_code &bec)
                    {
                        if (!detail::enterAsyncHandler(wptr,callback))
                        {
                            return;
                        }

                        {
                            if (bec)
                            {
                                Error ec;

                                if (bec.value()==boost::system::errc::operation_canceled)
                                {
                                    if (!m_timeoutTimer.isRunning())
                                    {
                                        ec=common::chainErrors(commonError(CommonError::TIMEOUT),networkError(NetworkError::CONNECT_FAILED));
                                    }
                                    else
                                    {
                                        m_timeoutTimer.cancel();
                                    }
                                }
                                else
                                {
                                    ec=common::chainErrors(makeBoostError(bec),networkError(NetworkError::CONNECT_FAILED));
                                    m_timeoutTimer.cancel();
                                }

                                HATN_CTX_LOG_DEBUG_ERROR(DoneDebugVerbosity,ec,"failed to connect to server",HatnAsioLog);

                                if (rawSocket().lowest_layer().is_open())
                                {
                                    boost::system::error_code ec1;
                                    rawSocket().lowest_layer().close(ec1);
                                    if (ec1)
                                    {
                                        HATN_CTX_WARN_RECORDS_M("failed to close",HatnAsioLog,{"err_code",ec1.value()},{"err_msg",ec1.message()})
                                    }
                                }

                                HATN_CTX_STACK_BARRIER_OFF("tcpstreamprepare")
                                m_stream->mainCtx().resetParentCtx();
                                callback(ec);
                                return;
                            }

                            m_timeoutTimer.cancel();

                            m_stream->updateEndpoints();
                            if (!bind)
                            {
                                HATN_CTX_SCOPE_PUSH("local_ip",m_stream->localEndpoint().address().to_string())
                                HATN_CTX_SCOPE_PUSH("local_port",m_stream->localEndpoint().port())
                            }
                            HATN_CTX_DEBUG(DoneDebugVerbosity,"client connected",HatnAsioLog);

                            HATN_CTX_STACK_BARRIER_OFF("tcpstreamprepare")
                            m_stream->mainCtx().resetParentCtx();
                            callback(Error{});
                        }

                        leaveAsyncHandler();
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
        m_stream->thread()->execAsync(
            [callback{std::move(callback)},wptr{ctxWeakPtr()},this]()
            {
                if (detail::enterAsyncHandler(wptr,callback,0))
                {
                    {
                        HATN_CTX_SCOPE("tcpsocketread")
                        callback(makeBoostError(boost::asio::error::eof),0);
                    }
                    leaveAsyncHandler();
                }
            }
        );
        return;
    }

    if (buf==nullptr || maxSize==0)
    {
        m_stream->thread()->execAsync(
            [callback{std::move(callback)},wptr{ctxWeakPtr()},this]()
            {
                if (detail::enterAsyncHandler(wptr,callback,0))
                {
                    {
                        callback(common::Error(),0);
                    }
                    leaveAsyncHandler();
                }
            }
        );
        return;
    }

    rawSocket().async_receive(
                boost::asio::buffer(buf,maxSize),
                [callback{std::move(callback)},wptr{ctxWeakPtr()},this](const boost::system::error_code &ec, size_t size)
                    {
                        if (detail::enterAsyncHandler(wptr,callback,0))
                        {
                            {
                                callback(makeBoostError(ec),size);
                            }

                            leaveAsyncHandler();
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
        m_stream->thread()->execAsync(
            [callback{std::move(callback)},wptr{ctxWeakPtr()},this]()
            {
                if (detail::enterAsyncHandler(wptr,callback,0))
                {
                    {
                        callback(makeBoostError(boost::asio::error::eof),0);
                    }

                    leaveAsyncHandler();
                }
            }
        );
        return;
    }

    if (buf==nullptr || size==0)
    {
        m_stream->thread()->execAsync(
            [callback{std::move(callback)},wptr{ctxWeakPtr()},this]()
            {
                if (detail::enterAsyncHandler(wptr,callback,0))
                {
                    callback(common::Error(),0);
                    leaveAsyncHandler();
                }
            }
        );
        return;
    }

    rawSocket().async_send(
                    boost::asio::buffer(buf,size),
                    [callback{std::move(callback)},wptr{ctxWeakPtr()},this](const boost::system::error_code &ec, size_t size)
                    {
                        if (detail::enterAsyncHandler(wptr,callback,0))
                        {
                            callback(makeBoostError(ec),size);
                            leaveAsyncHandler();
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
        m_stream->thread()->execAsync(
            [callback{std::move(callback)},wptr{ctxWeakPtr()},buffers{std::move(buffers)},this]()
            {
                if (detail::enterAsyncHandler(wptr,callback,std::move(buffers)))
                {
                    callback(makeBoostError(boost::asio::error::eof),0,std::move(buffers));
                    leaveAsyncHandler();
                }
            }
        );
        return;
    }

    if (buffers.empty())
    {
        m_stream->thread()->execAsync(
            [callback{std::move(callback)},wptr{ctxWeakPtr()},buffers{std::move(buffers)},this]()
            {
                if (detail::enterAsyncHandler(wptr,callback,std::move(buffers)))
                {
                    callback(common::Error(),0,std::move(buffers));
                    leaveAsyncHandler();
                }
            }
        );
        return;
    }

    pmr::vector<boost::asio::const_buffer> asioBuffers;
    if (!fillAsioBuffers(buffers,asioBuffers))
    {
        m_stream->thread()->execAsync(
            [callback{std::move(callback)},wptr{ctxWeakPtr()},buffers{std::move(buffers)},this]()
            {
                if (detail::enterAsyncHandler(wptr,callback,std::move(buffers)))
                {
                    callback(common::Error(CommonError::INVALID_SIZE),0,std::move(buffers));
                    leaveAsyncHandler();
                }
            }
        );
        return;
    }

    rawSocket().async_send(
                    asioBuffers,
                    [callback{std::move(callback)},wptr{ctxWeakPtr()},buffers{std::move(buffers)},this](const boost::system::error_code &ec, size_t size)
                    {                        
                        if (detail::enterAsyncHandler(wptr,callback,std::move(buffers)))
                        {
                            callback(makeBoostError(ec),size,std::move(buffers));
                            leaveAsyncHandler();
                        }
                    }
                );
}

//---------------------------------------------------------------

inline common::WeakPtr<common::TaskContext> TcpStreamTraits::ctxWeakPtr() const
{
    return toWeakPtr(m_stream->sharedMainCtx());
}

//---------------------------------------------------------------

inline void TcpStreamTraits::leaveAsyncHandler()
{
    m_stream->mainCtx().onAsyncHandlerExit();
}

//---------------------------------------------------------------

void TcpStreamTraits::waitForRead(std::function<void (const Error&)> callback)
{
    if (!m_stream->isActive())
    {
        m_stream->thread()->execAsync(
            [callback{std::move(callback)},wptr{ctxWeakPtr()},this]()
            {
                if (detail::enterAsyncHandler(wptr,callback))
                {
                    callback(makeBoostError(boost::asio::error::eof));
                    leaveAsyncHandler();
                }
            }
        );
        return;
    }

    rawSocket().async_wait(
        boost::asio::ip::tcp::socket::wait_read,
        [callback{std::move(callback)},wptr{ctxWeakPtr()},this](const boost::system::error_code &ec)
        {
            if (detail::enterAsyncHandler(wptr,callback))
            {
                if (ec.value()!=boost::system::errc::operation_canceled)
                {
                    callback(makeBoostError(ec));
                }
                leaveAsyncHandler();
            }
        }
    );
}

//---------------------------------------------------------------

TcpStream::~TcpStream()
{
}

//---------------------------------------------------------------
} // namespace asio

HATN_NETWORK_NAMESPACE_END
