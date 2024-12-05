/*
   Copyright (c) 2019 - current, Evgeny Sidorov (esid1976@gmail.com), All rights reserved
    
    
  */

/****************************************************************************/
/*
    
*/
/** \file network/asio/asioudpchannel.cpp
  *
  *     UDP channels
  *
  */

/****************************************************************************/

#include <hatn/common/thread.h>

#include <hatn/network/error.h>
#include <hatn/network/asio/udpchannel.h>

#include <hatn/common/loggermoduleimp.h>
INIT_LOG_MODULE(asioudpchannel,HATN_NETWORK_EXPORT)

namespace hatn {

using namespace common;

namespace network {
namespace asio {

/********************** UdpChannelTraits **************************/

//---------------------------------------------------------------
template <typename UdpChannelT>
UdpChannelTraits<UdpChannelT>::UdpChannelTraits(
        UdpChannelT* channel,
        Thread *thread
    ) : WithSocket<UdpSocket>(thread->asioContextRef()),
        m_channel(channel)
{
}

//---------------------------------------------------------------
template <typename UdpChannelT>
const char* UdpChannelTraits<UdpChannelT>::idStr() const noexcept
{
    return channel()->id().c_str();
}

//---------------------------------------------------------------
template <typename UdpChannelT>
void UdpChannelTraits<UdpChannelT>::cancel()
{
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
    std::ignore=destroying;
    common::Error ret;
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
        callback(ret);
    }
}

//---------------------------------------------------------------
template <typename UdpChannelT>
void UdpChannelTraits<UdpChannelT>::bind(
    UdpEndpoint endpoint,
    std::function<void (const common::Error &)> callback
)
{
    channel()->setLocalEndpoint(std::move(endpoint));
    bind(std::move(callback));
}

//---------------------------------------------------------------
template <typename UdpChannelT>
void UdpChannelTraits<UdpChannelT>::bind(std::function<void (const Error &)> callback)
{
    common::Error err;
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
                    [callback{std::move(callback)},err{std::move(err)}]()
                    {
                        callback(err);
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
    if (channel()->isActive())
    {
        std::function<void (const boost::system::error_code &, size_t)>&& cb=[callback{std::move(callback)},this](const boost::system::error_code &ec, size_t bytesTransferred)
        {
            if (channel()->isActive())
            {
                callback(makeBoostError(ec),bytesTransferred,UdpEndpoint(m_rxBoostEndpoint));
            }
        };
        rawSocket().async_receive_from(
                    boost::asio::buffer(buf,maxSize),
                    m_rxBoostEndpoint,
                    guardedAsyncHandler(std::move(cb))
        );
    }
    else
    {
        channel()->thread()->execAsync(
            [callback{std::move(callback)}]()
            {
                callback(makeBoostError(boost::asio::error::eof),0,UdpEndpoint());
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
    if (channel()->isActive())
    {
        auto ep=endpoint.toBoostEndpoint();
        std::function<void (const boost::system::error_code &, size_t)>&& cb=[callback{std::move(callback)},this](const boost::system::error_code &ec, size_t bytesTransferred)
        {
            if (channel()->isActive())
            {
                callback(makeBoostError(ec),bytesTransferred);
            }
        };
        rawSocket().async_send_to(
                    boost::asio::buffer(buf,size),
                    ep,
                    guardedAsyncHandler(std::move(cb))
        );
    }
    else
    {
        channel()->thread()->execAsync(
            [callback{std::move(callback)}]()
            {
                callback(makeBoostError(boost::asio::error::eof),0);
            }
        );
    }
}

//---------------------------------------------------------------
void UdpChannelMultipleTraits::sendTo(
        common::SpanBuffers buffers,
        const UdpEndpoint& endpoint,
        std::function<void (const common::Error&,size_t,common::SpanBuffers)> callback
    )
{
    if (channel()->isActive())
    {
        auto&& ep=endpoint.toBoostEndpoint();
        pmr::vector<boost::asio::const_buffer> asioBuffers;
        if (!fillAsioBuffers(buffers,asioBuffers))
        {
            channel()->thread()->execAsync(
                [callback{std::move(callback)},buffers{std::move(buffers)}]()
                {
                    callback(common::Error(CommonError::INVALID_SIZE),0,std::move(buffers));
                }
            );
            return;
        }
        rawSocket().async_send_to(
                    asioBuffers,
                    ep,
                    guardedAsyncHandler(
                        [callback{std::move(callback)},buffers{std::move(buffers)},this]
                        (const boost::system::error_code &ec, size_t bytesTransferred)
                        {
                            if (channel()->isActive())
                            {
                                callback(makeBoostError(ec),bytesTransferred,std::move(buffers));
                            }
                        }
                    )
        );
    }
    else
    {
        channel()->thread()->execAsync(
            [callback{std::move(callback)},buffers{std::move(buffers)}]() mutable
            {
                callback(makeBoostError(boost::asio::error::eof),0,std::move(buffers));
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
    auto&& cb=[callback{std::move(callback)},this](const Error &ec) mutable
    {
        if (ec)
        {
            callback(ec);
            return;
        }
        std::function<void (const boost::system::error_code &)>&& cb1=[callback{std::move(callback)},this](const boost::system::error_code &ec1) mutable
        {
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
        };
        rawSocket().async_connect(
            channel()->remoteEndpoint().toBoostEndpoint(),
            guardedAsyncHandler(cb1)
        );
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
        cb(Error());
    }
}

//---------------------------------------------------------------
void UdpChannelSingleTraits::send(const char *buf, size_t size, std::function<void (const Error &, size_t)> callback)
{
    if (channel()->isActive())
    {
        if (buf==nullptr || size==0)
        {
            channel()->thread()->execAsync(
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
                                [callback{std::move(callback)},this](const boost::system::error_code &ec, size_t size)
                                {
                                    if (channel()->isActive())
                                    {
                                        if (ec)
                                        {
                                            // DCS_WARN_ID(asioudpchannel,HATN_FORMAT("UDP socket failed to send: ({}) {}",ec.value(),ec.message()));
                                        }
                                        callback(makeBoostError(ec),size);
                                    }
                                }
                            )
                        );
        }
    }
    else
    {
        channel()->thread()->execAsync(
            [callback{std::move(callback)}]()
            {
                callback(makeBoostError(boost::asio::error::eof),0);
            }
        );
    }
}

//---------------------------------------------------------------
void UdpChannelSingleTraits::send(SpanBuffers buffers,
                                  std::function<void (const Error &, size_t, SpanBuffers)> callback
                                  )
{
    if (channel()->isActive())
    {
        pmr::vector<boost::asio::const_buffer> asioBuffers;
        if (!fillAsioBuffers(buffers,asioBuffers))
        {
            channel()->thread()->execAsync(
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
                            [callback{std::move(callback)},buffers{std::move(buffers)},this](const boost::system::error_code &ec, size_t size)
                            {
                                if (channel()->isActive())
                                {
                                    if (ec)
                                    {
                                        // DCS_WARN_ID(asioudpchannel,HATN_FORMAT("UDP socket failed to send: ({}) {}",ec.value(),ec.message()));
                                    }
                                    callback(makeBoostError(ec),size,std::move(buffers));
                                }
                            }
                        )
                    );
    }
    else
    {
        channel()->thread()->execAsync(
            [callback{std::move(callback)},buffers{std::move(buffers)}]() mutable
            {
                callback(makeBoostError(boost::asio::error::eof),0,std::move(buffers));
            }
        );
    }
}

//---------------------------------------------------------------
void UdpChannelSingleTraits::receive(char *buf, size_t maxSize, std::function<void (const Error &, size_t)> callback)
{
    if (channel()->isActive())
    {
        if (buf==nullptr || maxSize==0)
        {
            channel()->thread()->execAsync(
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
                            [callback{std::move(callback)},this](const boost::system::error_code &ec, size_t size)
                            {
                                if (channel()->isActive())
                                {
                                    callback(makeBoostError(ec),size);
                                }
                            }
                        )
                    );
        }
    }
    else
    {
        channel()->thread()->execAsync(
            [callback{std::move(callback)}]()
            {
                auto ec=boost::asio::error::eof;
                callback(makeBoostError(ec),0);
            }
        );
    }
}

/********************** UdpChannelMultiple **************************/

//---------------------------------------------------------------
UdpChannelMultiple::UdpChannelMultiple(Thread *thread, STR_ID_TYPE id)
    : UnreliableChannelMultiple<UdpEndpoint,UdpChannelMultipleTraits>(
                thread,
                std::move(id),
                this,
                thread
          )
{
}

/********************** UdpChannelSingle **************************/

//---------------------------------------------------------------
UdpChannelSingle::UdpChannelSingle(Thread *thread, STR_ID_TYPE id)
    : UnreliableChannelSingle<UdpEndpoint,UdpChannelSingleTraits>(                
                thread,
                std::move(id),
                this,
                thread
          )
{
}

#ifndef _WIN32
template class UdpChannelTraits<UdpChannelMultiple>;
template class UdpChannelTraits<UdpChannelSingle>;
#endif

//---------------------------------------------------------------
} // namespace asio
} // namespace network
} // namespace hatn
