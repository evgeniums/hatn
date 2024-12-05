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

#include <hatn/network/asio/tcpstream.h>

#include <hatn/common/loggermoduleimp.h>
INIT_LOG_MODULE(asiotcpstream,HATN_NETWORK_EXPORT)

HATN_NETWORK_NAMESPACE_BEGIN
HATN_COMMON_USING

namespace asio {

/*********************** TcpStream **************************/

//---------------------------------------------------------------
TcpStream::TcpStream(
        Thread *thread,
        STR_ID_TYPE id
    ) : ReliableStreamWithEndpoints<TcpEndpoint,TcpStreamTraits>(            
            thread,
            std::move(id),
            thread,
            this
        )
{
}

/*********************** TcpStreamTraits **************************/

//---------------------------------------------------------------
TcpStreamTraits::TcpStreamTraits(Thread *thread, TcpStream *stream):
    WithSocket<TcpSocket>(thread->asioContextRef()),
    m_stream(stream)
{
}

//---------------------------------------------------------------
inline const char* TcpStreamTraits::idStr() const noexcept
{
    return m_stream->id().c_str();
}

//---------------------------------------------------------------
void TcpStreamTraits::cancel()
{
    boost::system::error_code ec;
    rawSocket().cancel(ec);
    if (ec)
    {
        // DCS_DEBUG_ID(asiotcpstream,HATN_FORMAT("TCP socket failed to cancel: ({}) {}",ec.value(),ec.message()));
    }
}

//---------------------------------------------------------------
void TcpStreamTraits::close(const std::function<void (const Error &)> &callback, bool destroying)
{
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
                    // DCS_DEBUG_ID(asiotcpstream,HATN_FORMAT("TCP socket failed to shutdown: ({}) {}",ec.value(),ec.message()));
                }
            }

            rawSocket().lowest_layer().close(ec);
            if (ec)
            {
                // DCS_DEBUG_ID(asiotcpstream,HATN_FORMAT("TCP socket failed to close: ({}) {}",ec.value(),ec.message()));
                throw boost::system::system_error(ec);
            }
            // DCS_DEBUG_ID(asiotcpstream,"TCP socket closed");
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

//---------------------------------------------------------------
void TcpStreamTraits::prepare(
        std::function<void (const Error &)> callback
    )
{
    boost::system::error_code ec;

    // bind socket if local port is set
    if (m_stream->localEndpoint().port()!=0)
    {
        auto localEp=m_stream->localEndpoint().toBoostEndpoint();
        auto protocol=m_stream->localEndpoint().address().is_v6()?boost::asio::ip::tcp::v6():boost::asio::ip::tcp::v4();

        rawSocket().open(protocol,ec);
        if (!ec)
        {
            rawSocket().bind(localEp,ec);
            if (ec)
            {
                // DCS_ERROR_ID(asiotcpstream,HATN_FORMAT("TCP socket failed to bind to {}:{}, error: ({}) {}",localEp.address().to_string(),localEp.port(),ec.value(),ec.message()));
            }
            else
            {
                // DCS_DEBUG_ID(asiotcpstream,HATN_FORMAT("TCP socket local bind to {}:{}",m_stream->localEndpoint().address().to_string(),m_stream->localEndpoint().port()));
            }
        }
        else
        {
            // DCS_ERROR_ID(asiotcpstream,HATN_FORMAT("TCP socket failed to open {}:{}, error: ({}) {}",localEp.address().to_string(),localEp.port(),ec.value(),ec.message()));
        }
    }

    // connect to remote endpoint
    if (!ec)
    {
        auto ep=m_stream->remoteEndpoint().toBoostEndpoint();
        auto epCopy=ep;
        rawSocket().async_connect(
                        epCopy,
                        guardedAsyncHandler(
                            std::function<void (const boost::system::error_code &)>(
                            [callback{std::move(callback)},ep{std::move(ep)},this](const boost::system::error_code &ec)
                            {
                                if (!ec)
                                {
                                    m_stream->updateEndpoints();
#if 0
                                    DCS_DEBUG_ID(asiotcpstream,HATN_FORMAT("TCP socket connected to {}:{} from {}:{}",m_stream->remoteEndpoint().address().to_string(),m_stream->remoteEndpoint().port(),
                                                                          m_stream->localEndpoint().address().to_string(),m_stream->localEndpoint().port()));
#endif
                                }
                                else
                                {
                                    // DCS_DEBUG_ID(asiotcpstream,HATN_FORMAT("TCP socket failed to connect to {}:{}, error: ({}) {}",ep.address().to_string(),ep.port(),ec.value(),ec.message()));
                                    if (rawSocket().lowest_layer().is_open())
                                    {
                                        boost::system::error_code ec1;
                                        rawSocket().lowest_layer().close(ec1);
                                    }
                                }

                                callback(makeBoostError(ec));
                            })
                        )
                    );
    }
    else
    {
        m_stream->thread()->execAsync(
                        [callback{std::move(callback)},ec{std::move(ec)}]()
                        {
                            callback(makeBoostError(ec));
                        }
                    );
    }
}

//---------------------------------------------------------------
void TcpStreamTraits::read(
        char* buf,
        std::size_t maxSize,
        std::function<void (const common::Error &, size_t)> callback
    )
{
    if (m_stream->isActive())
    {
        if (buf==nullptr || maxSize==0)
        {
            m_stream->thread()->execAsync(
                [callback{std::move(callback)}]()
                {
                    callback(common::Error(),0);
                }
            );
        }
        else
        {
            rawSocket().async_receive(
                        boost::asio::buffer(buf,maxSize),
                        guardedAsyncHandler(
                            std::function<void (const boost::system::error_code &, size_t)>(
                            [callback{std::move(callback)},this](const boost::system::error_code &ec, size_t size)
                                {
                                    if (m_stream->isActive())
                                    {
                                        callback(makeBoostError(ec),size);
                                    }
                                })
                            )
                        );
        }
    }
    else
    {
        m_stream->thread()->execAsync(
            [callback{std::move(callback)}]()
            {
                auto ec=boost::asio::error::eof;
                callback(makeBoostError(ec),0);
            }
        );
    }
}

//---------------------------------------------------------------
void TcpStreamTraits::write(
        const char* buf,
        std::size_t size,
        std::function<void (const common::Error &, size_t)> callback
    )
{
    if (m_stream->isActive())
    {
        if (buf==nullptr || size==0)
        {
            m_stream->thread()->execAsync(
                [callback{std::move(callback)}]()
                {
                    callback(common::Error(),0);
                }
            );
        }
        else
        {
            rawSocket().async_send(
                        boost::asio::buffer(buf,size),
                        guardedAsyncHandler(
                            std::function<void (const boost::system::error_code &, size_t)>(
                                [callback{std::move(callback)},this](const boost::system::error_code &ec, size_t size)
                                {
                                    if (m_stream->isActive())
                                    {
                                        callback(makeBoostError(ec),size);
                                    }
                                })
                            )
                        );
        }
    }
    else
    {
        m_stream->thread()->execAsync(
            [callback{std::move(callback)}]()
            {
                callback(makeBoostError(boost::asio::error::eof),0);
            }
        );
    }
}

//---------------------------------------------------------------
void TcpStreamTraits::write(
        SpanBuffers buffers,
        std::function<void (const Error &, size_t, SpanBuffers)> callback
    )
{
    if (m_stream->isActive())
    {
        if (buffers.empty())
        {
            m_stream->thread()->execAsync(
                [callback{std::move(callback)},buffers{std::move(buffers)}]() mutable
                {
                    callback(common::Error(),0,std::move(buffers));
                }
            );
        }
        else
        {
            pmr::vector<boost::asio::const_buffer> asioBuffers;
            if (!fillAsioBuffers(buffers,asioBuffers))
            {
                m_stream->thread()->execAsync(
                    [callback{std::move(callback)},buffers{std::move(buffers)}]() mutable
                    {
                        callback(common::Error(CommonError::INVALID_SIZE),0,std::move(buffers));
                    }
                );
                return;
            }
            rawSocket().async_send(
                        asioBuffers,
                        guardedAsyncHandler(
                            std::function<void (const boost::system::error_code &, size_t)>(
                                [callback{std::move(callback)},buffers{std::move(buffers)},this](const boost::system::error_code &ec, size_t size) mutable
                                {
                                    if (m_stream->isActive())
                                    {
                                        callback(makeBoostError(ec),size,std::move(buffers));
                                    }
                                })
                            )
                        );
        }
    }
    else
    {
        m_stream->thread()->execAsync(
            [callback{std::move(callback)},buffers{std::move(buffers)}]() mutable
            {
                callback(makeBoostError(boost::asio::error::eof),0,std::move(buffers));
            }
        );
    }
}

//---------------------------------------------------------------
} // namespace asio

HATN_NETWORK_NAMESPACE_END
