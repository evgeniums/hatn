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

#include <hatn/api/apiliberror.h>

#include <hatn/grpcclient/grpcstream.h>
#include <hatn/grpcclient/ipp/grpctransport.ipp>

#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>

HATN_GRPCCLIENT_NAMESPACE_BEGIN

HDU_UNIT(stream_response,
    HDU_FIELD(message_type,TYPE_STRING,1)
    HDU_FIELD(message,TYPE_BYTES,2)
)

/*****************************GrpcStream*********************************/

//--------------------------------------------------------------------------

GrpcStream::GrpcStream(
        std::shared_ptr<detail::GrpcTransport_p> transport,
        api::Priority channelPriority,
        std::shared_ptr<grpc::ClientContext> context
    ) : m_transport(std::move(transport)),
        m_channelPriority(channelPriority),
        m_context(std::move(context)),
        m_closed(false),
        m_initialResponse(true)
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

    //! @todo Fix for outgoing/bidirectional streams
    StartWritesDone();

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

    auto resetCallback=[&]()
    {
        m_mutex.lock();
        m_readCallback=ReadCb{};
        m_mutex.unlock();
    };
    HATN_SCOPE_GUARD(resetCallback)

    m_mutex.lock();
    auto rcb=m_readCallback;
    m_mutex.unlock();

    // hadnle response
    auto resp=m_transport->handleResponse(
        m_context,
        grpc::Status::OK,
        m_responseBuffer,
        !m_initialResponse
    );
    if (resp)
    {
        // inform on error
        if (rcb)
        {
            rcb(resp.error(),{});
        }
        return;
    }

    // handle initial response
    if (m_initialResponse)
    {
        // initial response done
        m_initialResponse=false;

        // invoke callback on initial message
        if (rcb)
        {
            resp->setStreamChannel(shared_from_this());
            rcb({},resp.takeValue());
        }

        return;
    }

    // handle stream mesage
    stream_response::type respWrapper;
    respWrapper.setParseToSharedArrays(true);
    resp->setMessageType(stream_response::conf().name);
    auto ec=resp->parse(respWrapper);
    if (ec)
    {
        if (rcb)
        {
            rcb(ec,{});
        }
        return;
    }

    // process response depending on message type
    if (respWrapper.fieldValue(stream_response::message_type)
        ==
        m_transport->transport->config().fieldValue(grpc_config::heartbeat_response_type))
    {
        // skip heartbeat
        m_responseBuffer.Clear();
        StartRead(&m_responseBuffer);
    }
    else if (respWrapper.fieldValue(stream_response::message_type)
             ==
             m_transport->transport->config().fieldValue(grpc_config::error_response_type))
    {
        //! @todo Implement error handling
        if (rcb)
        {
            ec=commonError(CommonError::SERVER_API_ERROR);
            rcb(ec,{});
        }
    }
    else
    {
        auto msgResp=m_transport->handleResponse(
            m_context,
            grpc::Status::OK,
            grpc::ByteBuffer{},
            true,
            std::string{respWrapper.fieldValue(stream_response::message_type)},
            respWrapper.field(stream_response::message).byteArrayShared()
        );

        if (msgResp)
        {
            // inform on error
            if (rcb)
            {
                rcb(resp.error(),{});
            }
            return;
        }

        // normal message
        if (rcb)
        {
            rcb({},msgResp.takeValue());
        }
    }
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

void GrpcStream::writeNext(common::ByteArrayShared message, std::string /*messageType*/, WriteCb callback)
{
    if (m_closed)
    {
        callback(commonError(CommonError::ABORTED));
        return;
    }

    m_mutex.lock();
    m_writeCallback=callback;
    m_mutex.unlock();

    m_writeBuffer.Clear();
    grpc::Slice slice(message->data(), message->size());
    m_writeBuffer=grpc::ByteBuffer(&slice, 1);

    StartWrite(&m_writeBuffer);
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
        m_closeCallback=callback;
        m_mutex.unlock();
    }

    m_context->TryCancel();
}

//--------------------------------------------------------------------------

void GrpcStream::startStream(const grpc::ByteBuffer* initMsg, ReadCb callback)
{
    m_readCallback=callback;

    StartWrite(initMsg);
    StartRead(&m_responseBuffer);
    StartCall();
}

//--------------------------------------------------------------------------

HATN_GRPCCLIENT_NAMESPACE_END
