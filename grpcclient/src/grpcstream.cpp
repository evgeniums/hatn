/*
    Copyright (c) 2024 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    {{LICENSE}}
*/

/****************************************************************************/
/*

*/
/** @file grpcclient/grpcstream.cpp
  *
  */

#include <hatn/grpcclient/grpcstream.h>
#include <hatn/grpcclient/ipp/grpctransport.ipp>

#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>

HATN_GRPCCLIENT_NAMESPACE_BEGIN

/*****************************GrpcStream*********************************/

//--------------------------------------------------------------------------

GrpcStream::GrpcStream(
        std::shared_ptr<detail::GrpcTransport_p> transport,
        api::Priority channelPriority,
        std::shared_ptr<grpc::ClientContext> context
    ) : m_transport(std::move(transport)),
        m_channelPriority(channelPriority),
        m_context(std::move(context)),
        m_closed(false)
{
}

//--------------------------------------------------------------------------

GrpcStream::~GrpcStream()
{}

//--------------------------------------------------------------------------

void GrpcStream::OnWriteDone(bool ok)
{
    if (!ok)
    {
        // failed, wait for onDone
        return;
    }

    m_mutex.lock();
    auto wcb=m_writeCallback;
    m_mutex.unlock();

    if (wcb)
    {
        wcb({});
    }

    m_mutex.lock();
    m_writeCallback=WriteCb{};
    m_mutex.unlock();
}

//--------------------------------------------------------------------------

void GrpcStream::OnReadDone(bool ok)
{
    if (!ok)
    {
        // failed, wait for onDone
        return;
    }

    // hadnle response
    auto resp=m_transport->handleResponse(
        m_context,
        grpc::Status::OK,
        m_responseBuffer
    );

    m_mutex.lock();
    auto rcb=m_readCallback;
    m_mutex.unlock();

    if (resp)
    {
        // inform on error
        if (rcb)
        {
            rcb(resp.error(),{});
        }
        return;
    }

    // parse wrapping message

    // read next if it is heartbeat

    if (!rcb)
    {
        return;
    }

    // inform application if it is either error or normal message

    // reset callback
    m_mutex.lock();
    m_readCallback=ReadCb{};
    m_mutex.unlock();
}

//--------------------------------------------------------------------------

void GrpcStream::OnDone(const grpc::Status& status)
{
    m_mutex.lock();
    m_closed=true;
    auto rcb=m_readCallback;
    auto wcb=m_writeCallback;
    auto ccb=m_closeCallback;
    m_mutex.unlock();

    if (!status.ok())
    {
        auto resp=m_transport->handleResponse(
            m_context,
            status,
            m_responseBuffer
        );

        if (resp)
        {
            if (wcb)
            {
                wcb(resp.error());
            }
            if (rcb)
            {
                rcb(resp.error(),{});
            }
            if (ccb)
            {
                ccb(resp.error());
            }
        }
        else
        {
            auto ec=commonError(CommonError::ABORTED);
            if (wcb)
            {
                wcb(ec);
            }
            if (rcb)
            {
                rcb(ec,{});
            }
            if (ccb)
            {
                ccb({});
            }
        }
    }
    else if (ccb)
    {
        ccb({});
    }

    m_mutex.lock();
    m_writeCallback=WriteCb{};
    m_readCallback=ReadCb{};
    m_closeCallback=CloseCb{};
    m_mutex.unlock();

    auto self=shared_from_this();
    m_transport->removeStream(m_channelPriority,self);
}

//--------------------------------------------------------------------------

void GrpcStream::readNext(ReadCb callback) {

    if (m_closed)
    {
        callback(commonError(CommonError::ABORTED),{});
        return;
    }

    m_mutex.lock();
    m_readCallback=callback;
    m_mutex.unlock();

    m_responseBuffer.Clear();
    StartRead(&m_responseBuffer);
}

//--------------------------------------------------------------------------

void GrpcStream::writeNext(common::ByteArrayShared message, std::string messageType, WriteCb callback)
{
    if (m_closed)
    {
        callback(commonError(CommonError::ABORTED));
        return;
    }

    m_mutex.lock();
    m_writeCallback=callback;
    m_mutex.unlock();

    //! @todo write to stream
}

//--------------------------------------------------------------------------

void GrpcStream::close(clientapi::StreamChannel::CloseCb callback)
{
    m_mutex.lock();
    auto ccb=m_closeCallback;
    m_mutex.unlock();

    if (m_closed || ccb)
    {
        if (callback)
        {
            callback({});
        }
        return;
    }

    if (callback)
    {
        m_mutex.lock();
        m_writeCallback=callback;
        m_mutex.unlock();
    }

    m_context->TryCancel();
}

//--------------------------------------------------------------------------

HATN_GRPCCLIENT_NAMESPACE_END
