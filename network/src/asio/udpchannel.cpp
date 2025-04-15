/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file network/asio/asioudpchannel.cpp
  *
  *   UDP channels using boost.asio backend.
*/

/****************************************************************************/

#include <hatn/common/thread.h>

#include <hatn/network/networkerror.h>
#include <hatn/network/asio/udpchannel.h>

HATN_TASK_CONTEXT_DEFINE(HATN_NETWORK_NAMESPACE::asio::UdpServer,UdpServer)
HATN_TASK_CONTEXT_DEFINE(HATN_NETWORK_NAMESPACE::asio::UdpClient,UdpClient)

HATN_NETWORK_NAMESPACE_BEGIN
HATN_COMMON_USING

namespace asio {

/********************** UdpChannelTraits **************************/

//---------------------------------------------------------------
template <typename UdpChannelT>
UdpChannelTraits<UdpChannelT>::UdpChannelTraits(
        UdpChannelT* channel
    ) : WithSocket<UdpSocket>(channel->thread()->asioContextRef()),
        m_channel(channel)
{
}

//---------------------------------------------------------------
template <typename UdpChannelT>
void UdpChannelTraits<UdpChannelT>::cancel()
{
    HATN_CTX_SCOPE("udpchannelcancel")

    boost::system::error_code ec;
    rawSocket().cancel(ec);
    if (ec)
    {
        // DCS_DEBUG_ID(asioudpchannel,HATN_FORMAT("UDP socket failed to cancel: ({}) {}",ec.value(),ec.message()));
    }
}

//---------------------------------------------------------------
template <typename UdpChannelT>
void UdpChannelTraits<UdpChannelT>::close(const std::function<void (const Error &)> &callback, bool destroying)
{
    if (destroying)
    {
        return;
    }

    HATN_CTX_SCOPE("udpchannelclose")

    Error ret;
    if (rawSocket().lowest_layer().is_open())
    {
        try
        {
            boost::system::error_code ec;
            rawSocket().lowest_layer().close(ec);
            if (ec)
            {
                // DCS_DEBUG_ID(asioudpchannel,HATN_FORMAT("UDP socket failed to close: ({}) {}",ec.value(),ec.message()));
                throw boost::system::system_error(ec);
            }

            // DCS_DEBUG_ID(asioudpchannel,"UDP socket closed");
        }
        catch (const boost::system::system_error& e)
        {
            ret=makeBoostError(e.code());
        }
    }
    if (callback)
    {
        channel()->thread()->execAsync(
            [callback{std::move(callback)},wptr{channel()->ctxWeakPtr()},ret{std::move(ret)},this]()
            {
                if (detail::enterAsyncHandler(wptr,callback))
                {
                    HATN_CTX_SCOPE("udpchannelclose")
                    callback(ret);
                    channel()->leaveAsyncHandler();
                }
            }
        );
    }
}

//---------------------------------------------------------------
template <typename UdpChannelT>
void UdpChannelTraits<UdpChannelT>::bind(
    UdpEndpoint endpoint,
    std::function<void (const Error &)> callback
)
{
    channel()->setLocalEndpoint(std::move(endpoint));
    bind(std::move(callback));
}

//---------------------------------------------------------------
template <typename UdpChannelT>
void UdpChannelTraits<UdpChannelT>::bind(std::function<void (const Error &)> callback)
{
    HATN_CTX_SCOPE("udpchannelbind")

    Error err;
    auto endpoint=channel()->localEndpoint().toBoostEndpoint();
    if (!this->rawSocket().is_open())
    {
        boost::system::error_code ec;
        this->rawSocket().open(endpoint.protocol(),ec);
        if (!ec)
        {
            this->rawSocket().bind(endpoint,ec);
            if (ec)
            {
                // DCS_WARN_ID(asioudpchannel,HATN_FORMAT("UDP socket failed to bind {}:{} : ({}) {}",endpoint.address().to_string(),endpoint.port(),ec.value(),ec.message()));
                err=makeBoostError(ec);
                this->rawSocket().close(ec);
            }
            else
            {
                channel()->localEndpoint()=rawSocket().local_endpoint();
                // DCS_DEBUG_ID(asioudpchannel,HATN_FORMAT("UDP socket bind local {}:{} success",channel()->localEndpoint().address().to_string(),channel()->localEndpoint().port()));
            }
        }
        else
        {
            // DCS_WARN_ID(asioudpchannel,HATN_FORMAT("UDP socket failed to open {}:{} : ({}) {}",endpoint.address().to_string(),endpoint.port(),ec.value(),ec.message()));
            err=makeBoostError(ec);
        }
    }
    else
    {
        // DCS_WARN_ID(asioudpchannel,HATN_FORMAT("UDP socket {}:{} is open already",endpoint.address().to_string(),endpoint.port()));
    }
    channel()->thread()->execAsync(
                    [callback{std::move(callback)},wptr{channel()->ctxWeakPtr()},err{std::move(err)},this]()
                    {
                        if (detail::enterAsyncHandler(wptr,callback))
                        {
                            HATN_CTX_SCOPE("udpchannelbind")
                            callback(err);
                            channel()->leaveAsyncHandler();
                        }
                    }
                );
}

/********************** UdpChannelMultipleTraits **************************/

//---------------------------------------------------------------
void UdpChannelMultipleTraits::receiveFrom(
        char *buf,
        size_t maxSize,
        std::function<void (const Error &, size_t, const UdpEndpoint &)> callback
    )
{
    HATN_CTX_SCOPE("udpmultirecvfrom")
    if (channel()->isActive())
    {
        rawSocket().async_receive_from(
                    boost::asio::buffer(buf,maxSize),
                    m_rxBoostEndpoint,
                    [callback{std::move(callback)},wptr{channel()->ctxWeakPtr()},this](const boost::system::error_code &ec, size_t bytesTransferred)
                    {
                        if (detail::enterAsyncHandler(wptr,callback,UdpEndpoint{}))
                        {
                            HATN_CTX_SCOPE("udpmultirecvfrom")
                            callback(makeBoostError(ec),bytesTransferred,UdpEndpoint(m_rxBoostEndpoint));
                            channel()->leaveAsyncHandler();
                        }
                    }
        );
    }
    else
    {
        channel()->thread()->execAsync(
            [callback{std::move(callback)},wptr{channel()->ctxWeakPtr()},this]()
            {                
                if (detail::enterAsyncHandler(wptr,callback,UdpEndpoint{}))
                {
                    HATN_CTX_SCOPE("udpmultirecvfrom")
                    callback(makeBoostError(boost::asio::error::eof),0,UdpEndpoint{});
                    channel()->leaveAsyncHandler();
                }
            }
        );
    }
}

//---------------------------------------------------------------
void UdpChannelMultipleTraits::sendTo(
        const char *buf,
        size_t size,
        const UdpEndpoint &endpoint,
        std::function<void (const Error &, size_t)> callback
    )
{
    HATN_CTX_SCOPE("udpmultisendto")
    if (channel()->isActive())
    {
        auto ep=endpoint.toBoostEndpoint();
        rawSocket().async_send_to(
                    boost::asio::buffer(buf,size),
                    ep,
                    [callback{std::move(callback)},wptr{channel()->ctxWeakPtr()},this](const boost::system::error_code &ec, size_t bytesTransferred)
                    {
                        if (detail::enterAsyncHandler(wptr,callback,0))
                        {
                            HATN_CTX_SCOPE("udpmultisendto")
                            callback(makeBoostError(ec),bytesTransferred);
                            channel()->leaveAsyncHandler();
                        }
                    }
        );
    }
    else
    {
        channel()->thread()->execAsync(
            [callback{std::move(callback)},wptr{channel()->ctxWeakPtr()},this]()
            {
                if (detail::enterAsyncHandler(wptr,callback,0))
                {
                    HATN_CTX_SCOPE("udpmultisendto")
                    callback(makeBoostError(boost::asio::error::eof),0);
                    channel()->leaveAsyncHandler();
                }
            }
        );
    }
}

//---------------------------------------------------------------
void UdpChannelMultipleTraits::sendTo(
        common::SpanBuffers buffers,
        const UdpEndpoint& endpoint,
        std::function<void (const Error&,size_t,common::SpanBuffers)> callback
    )
{
    HATN_CTX_SCOPE("udpmultispansendto")

    if (channel()->isActive())
    {
        auto&& ep=endpoint.toBoostEndpoint();
        pmr::vector<boost::asio::const_buffer> asioBuffers;
        if (!fillAsioBuffers(buffers,asioBuffers))
        {
            channel()->thread()->execAsync(
                [callback{std::move(callback)},wptr{channel()->ctxWeakPtr()},buffers{std::move(buffers)},this]()
                {
                    if (detail::enterAsyncHandler(wptr,callback,std::move(buffers)))
                    {
                        HATN_CTX_SCOPE("udpmultispansendto")
                        callback(Error(CommonError::INVALID_SIZE),0,std::move(buffers));
                        channel()->leaveAsyncHandler();
                    }
                }
            );
            return;
        }
        rawSocket().async_send_to(
                    asioBuffers,
                    ep,            
                    [callback{std::move(callback)},wptr{channel()->ctxWeakPtr()},buffers{std::move(buffers)},this](const boost::system::error_code &ec, size_t bytesTransferred)
                    {
                        if (detail::enterAsyncHandler(wptr,callback,std::move(buffers)))
                        {
                            HATN_CTX_SCOPE("udpmultispansendto")
                            callback(makeBoostError(ec),bytesTransferred,std::move(buffers));
                            channel()->leaveAsyncHandler();
                        }
                    }
        );
    }
    else
    {
        channel()->thread()->execAsync(
            [callback{std::move(callback)},wptr{channel()->ctxWeakPtr()},buffers{std::move(buffers)},this]()
            {
                if (detail::enterAsyncHandler(wptr,callback,std::move(buffers)))
                {
                    HATN_CTX_SCOPE("udpmultispansendto")
                    callback(makeBoostError(boost::asio::error::eof),0,std::move(buffers));
                    channel()->leaveAsyncHandler();
                }
            }
        );
    }
}

/********************** UdpChannelSingleTraits **************************/

//---------------------------------------------------------------
void UdpChannelSingleTraits::prepare(
        std::function<void (const Error &)> callback
    )
{
    HATN_CTX_SCOPE("udpsingleprepare")

    auto cb=[callback,wptr{channel()->ctxWeakPtr()},this](const Error &ec)
    {
        if (!detail::enterAsyncHandler(wptr,callback))
        {
            return;
        }

        HATN_CTX_SCOPE("udpsingleconnect")

        if (ec)
        {
            callback(ec);
            channel()->leaveAsyncHandler();
            return;
        }

        auto cb1=[callback{std::move(callback)},wptr{channel()->ctxWeakPtr()},this](const boost::system::error_code &ec1)
        {
            if (!detail::enterAsyncHandler(wptr,callback))
            {
                return;
            }

            HATN_CTX_SCOPE("udpsingleconnect")

            if (!ec1)
            {
                channel()->localEndpoint()=rawSocket().local_endpoint();
#if 0
                DCS_DEBUG_ID(asioudpchannel,HATN_FORMAT("UDP socket connected to remote {}:{} from local {}:{}",channel()->remoteEndpoint().address().to_string(),channel()->remoteEndpoint().port(),
                                                      channel()->localEndpoint().address().to_string(),channel()->localEndpoint().port()));
#endif
            }
            else
            {
                // DCS_WARN_ID(asioudpchannel,HATN_FORMAT("UDP socket failed to connect to {}:{} : ({}) {}",channel()->remoteEndpoint().address().to_string(),channel()->remoteEndpoint().port(),ec1.value(),ec1.message()));
            }

            callback(makeBoostError(ec1));
            channel()->leaveAsyncHandler();
        };
        rawSocket().async_connect(
            channel()->remoteEndpoint().toBoostEndpoint(),
            std::move(cb1)
        );
        channel()->leaveAsyncHandler();
    };
    if (channel()->localEndpoint().port()!=0 ||
            (channel()->localEndpoint().address()!=boost::asio::ip::address_v4::any()
             &&
             channel()->localEndpoint().address()!=boost::asio::ip::address_v6::any()
             )
        )
    {
        bind(cb);
    }
    else
    {
        channel()->thread()->execAsync(
            [cb{std::move(cb)},callback,wptr{channel()->ctxWeakPtr()}]()
            {
                if (detail::enterAsyncHandler(wptr,callback))
                {
                    HATN_CTX_SCOPE("udpsingleprepare")
                    cb(Error());
                }
            }
        );
    }
}

//---------------------------------------------------------------
void UdpChannelSingleTraits::send(
        const char *buf,
        size_t size,
        std::function<void (const Error &, size_t)> callback
    )
{
    HATN_CTX_SCOPE("udpsinglesend")

    if (channel()->isActive())
    {
        if (buf==nullptr || size==0)
        {
            channel()->thread()->execAsync(
                [callback{std::move(callback)},wptr{channel()->ctxWeakPtr()},this]()
                {
                    if (detail::enterAsyncHandler(wptr,callback,0))
                    {
                        HATN_CTX_SCOPE("udpsinglesend")
                        callback(Error(),0);
                        channel()->leaveAsyncHandler();
                    }
                }
            );
        }
        else
        {
            rawSocket().async_send(
                            boost::asio::buffer(buf,size),
                            [callback{std::move(callback)},wptr{channel()->ctxWeakPtr()},this](const boost::system::error_code &ec, size_t size)
                            {
                                if (detail::enterAsyncHandler(wptr,callback,0))
                                {
                                    HATN_CTX_SCOPE("udpsinglesend")
                                    if (ec)
                                    {
                                        // DCS_WARN_ID(asioudpchannel,HATN_FORMAT("UDP socket failed to send: ({}) {}",ec.value(),ec.message()));
                                    }
                                    callback(makeBoostError(ec),size);
                                    channel()->leaveAsyncHandler();
                                }
                            }
                        );
        }
    }
    else
    {
        channel()->thread()->execAsync(
            [callback{std::move(callback)},wptr{channel()->ctxWeakPtr()},this]()
            {
                if (detail::enterAsyncHandler(wptr,callback,0))
                {
                    HATN_CTX_SCOPE("udpsinglesend")
                    callback(makeBoostError(boost::asio::error::eof),0);
                    channel()->leaveAsyncHandler();
                }
            }
        );
    }
}

//---------------------------------------------------------------
void UdpChannelSingleTraits::send(SpanBuffers buffers,
                                  std::function<void (const Error &, size_t, SpanBuffers)> callback
                                  )
{
    HATN_CTX_SCOPE("udpsinglespansend")

    if (channel()->isActive())
    {
        pmr::vector<boost::asio::const_buffer> asioBuffers;
        if (!fillAsioBuffers(buffers,asioBuffers))
        {
            channel()->thread()->execAsync(
                [callback{std::move(callback)},wptr{channel()->ctxWeakPtr()},buffers{std::move(buffers)},this]()
                {
                   if (detail::enterAsyncHandler(wptr,callback,std::move(buffers)))
                   {
                       HATN_CTX_SCOPE("udpsinglespansend")
                       callback(Error(CommonError::INVALID_SIZE),0,std::move(buffers));
                       channel()->leaveAsyncHandler();
                   }
                }
            );
            return;
        }
        rawSocket().async_send(
                        asioBuffers,
                        [callback{std::move(callback)},buffers{std::move(buffers)},wptr{channel()->ctxWeakPtr()},this](const boost::system::error_code &ec, size_t size)
                        {
                           if (detail::enterAsyncHandler(wptr,callback,std::move(buffers)))
                           {
                                HATN_CTX_SCOPE("udpsinglespansend")
                                if (ec)
                                {
                                    // DCS_WARN_ID(asioudpchannel,HATN_FORMAT("UDP socket failed to send: ({}) {}",ec.value(),ec.message()));
                                }
                                callback(makeBoostError(ec),size,std::move(buffers));
                                channel()->leaveAsyncHandler();
                           }
                        }
                    );
    }
    else
    {
        channel()->thread()->execAsync(
            [callback{std::move(callback)},buffers{std::move(buffers)},wptr{channel()->ctxWeakPtr()},this]()
            {
                if (detail::enterAsyncHandler(wptr,callback,std::move(buffers)))
                {
                    HATN_CTX_SCOPE("udpsinglespansend")
                    callback(makeBoostError(boost::asio::error::eof),0,std::move(buffers));
                    channel()->leaveAsyncHandler();
                }
            }
        );
    }
}

//---------------------------------------------------------------
void UdpChannelSingleTraits::receive(char *buf, size_t maxSize, std::function<void (const Error &, size_t)> callback)
{
    HATN_CTX_SCOPE("udpsinglerecv")

    if (channel()->isActive())
    {
        if (buf==nullptr || maxSize==0)
        {
            channel()->thread()->execAsync(
                [callback{std::move(callback)},wptr{channel()->ctxWeakPtr()},this]()
                {
                    if (detail::enterAsyncHandler(wptr,callback,0))
                    {
                        HATN_CTX_SCOPE("udpsinglerecv")
                        callback(Error(),0);
                        channel()->leaveAsyncHandler();
                    }
                }
            );
        }
        else
        {
            rawSocket().async_receive(
                        boost::asio::buffer(buf,maxSize),
                        [callback{std::move(callback)},wptr{channel()->ctxWeakPtr()},this](const boost::system::error_code &ec, size_t size)
                        {
                            if (detail::enterAsyncHandler(wptr,callback,0))
                            {
                                HATN_CTX_SCOPE("udpsinglerecv")
                                callback(makeBoostError(ec),size);
                                channel()->leaveAsyncHandler();
                            }
                        }
                    );
        }
    }
    else
    {
        channel()->thread()->execAsync(
            [callback{std::move(callback)},wptr{channel()->ctxWeakPtr()},this]()
            {
                if (detail::enterAsyncHandler(wptr,callback,0))
                {
                    HATN_CTX_SCOPE("udpsinglerecv")
                    callback(makeBoostError(boost::asio::error::eof),0);
                    channel()->leaveAsyncHandler();
                }
            }
        );
    }
}

/********************** UdpChannelMultiple **************************/

//---------------------------------------------------------------
UdpChannelMultiple::UdpChannelMultiple(Thread *thread)
    : UnreliableChannelMultiple<UdpEndpoint,UdpChannelMultipleTraits>(
                thread,
                this
          )
{
}

/********************** UdpChannelSingle **************************/

//---------------------------------------------------------------
UdpChannelSingle::UdpChannelSingle(Thread *thread)
    : UnreliableChannelSingle<UdpEndpoint,UdpChannelSingleTraits>(                
                thread,
                this
          )
{
}

#ifndef _WIN32
template class HATN_NETWORK_EXPORT UdpChannelTraits<UdpChannelMultiple>;
template class HATN_NETWORK_EXPORT UdpChannelTraits<UdpChannelSingle>;
#endif

//---------------------------------------------------------------
} // namespace asio

HATN_NETWORK_NAMESPACE_END
