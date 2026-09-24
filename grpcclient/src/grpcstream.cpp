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

#include "grpcstream.h"
#include "grpctransport_p.h"

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
        m_initialResponse(true),
        m_readPending(false)
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

    // Consume the write callback exactly once, BEFORE invoking it. The callback may issue the
    // next writeNext() (synchronously or from another thread), which installs the next
    // callback into m_writeCallback; resetting the slot after the invocation (as this used to
    // do) could wipe that freshly installed callback. Same invariant as OnReadDone().
    WriteCb wcb;
    {
        common::MutexScopedLock l{m_mutex};
        wcb=std::move(m_writeCallback);
        m_writeCallback=WriteCb{};
    }

    if (wcb)
    {
        wcb({});
    }
}

//--------------------------------------------------------------------------

// Invariant: the read callback is consumed exactly once, before it is invoked. The callback
// re-arms the read by calling readNext() - either synchronously or, as ServerEventListener
// does, from a handler posted to the app thread - and readNext() installs the NEXT callback
// into m_readCallback. Nothing in this function may touch m_readCallback after the
// invocation. The previous implementation reset the slot in a scope guard that ran after
// rcb(...) returned; when the app-thread readNext() raced in before that guard ran, its
// freshly installed callback was wiped, the next OnReadDone() found no callback, nobody
// re-armed StartRead(), and the stream silently went deaf while the channel (and unary
// calls on it) stayed perfectly healthy.
void GrpcStream::OnReadDone(bool ok)
{
    if (!ok)
    {
        // failed, wait for onDone, which reports the error to the still-pending read callback
        m_readPending=false;
#if 0
        std::cerr << "GrpcStream::OnReadDone failed" << std::endl;
#endif
        return;
    }
#if 0
    std::cout << "GrpcStream::OnReadDone begin" << std::endl;
#endif
    ReadCb rcb;
    {
        common::MutexScopedLock l{m_mutex};
        rcb=std::move(m_readCallback);
        m_readCallback=ReadCb{};
    }
    // release only after the slot has been taken, so a readNext() triggered by rcb below
    // always installs into an empty slot that nobody will reset afterwards
    m_readPending=false;

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
#if 0
    std::cout << "GrpcStream::OnReadDone respWrapper: " << respWrapper.toString(true) << std::endl;
#endif
    // process response depending on message type
    if (respWrapper.fieldValue(stream_response::message_type)
             ==
             m_transport->transport->config().fieldValue(grpc_config::error_response_type))
    {
        // No evgo producer ever sends a message_type=="grpc_api_server.Error" in-band frame
        // (that proto message exists but has zero writers - see the taxonomy design notes),
        // so this branch is unreachable against any server in this tree; kept only in case a
        // future server or transport starts using it. The path this contract actually relies
        // on for a late in-stream failure is GrpcStream::OnDone(), reached via the trailer
        // metadata evgo's fillResponse(...,trailing=true) now sets - see
        // whitemdesktop/docs/error-contract.md.
        //! @todo Implement error handling if a producer for this frame type appears
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
            // inform on error (was resp.error() - the outer, successful result - so the
            // listener got "success with an empty response" instead of the actual error)
            if (rcb)
            {
                rcb(msgResp.error(),{});
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
#if 0
    std::cerr << "GrpcStream::OnDone status=" << status.error_message() << std::endl;
#endif

    // m_closed is set before the callbacks are taken, and readNext()/writeNext() check it
    // first, so no new callback can be installed once the slots are moved out here.
    ReadCb rcb;
    WriteCb wcb;
    CloseCb ccb;
    {
        common::MutexScopedLock l{m_mutex};
        m_closed=true;
        rcb=std::move(m_readCallback);
        wcb=std::move(m_writeCallback);
        ccb=std::move(m_closeCallback);
        m_readCallback=ReadCb{};
        m_writeCallback=WriteCb{};
        m_closeCallback=CloseCb{};
    }

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
        else if (resp->status() == HATN_API_NAMESPACE::protocol::ResponseStatus::AuthError)
        {
            // auth error not API error, it can be processed by client session
            if (wcb)
            {
                wcb(commonError(CommonError::ABORTED));
            }
            if (rcb)
            {
                //! @todo handle API errors more gracefully
                rcb({},resp.takeValue());
            }
            if (ccb)
            {
                ccb({});
            }
        }
        else if (resp->error())
        {
            // The stream ended with a real error - possibly carrying the family/disposition
            // this contract adds, via evgo's fillResponse(...,trailing=true) on a late
            // in-stream failure (see whitemdesktop/docs/error-contract.md). Previously this
            // branch discarded resp->error() entirely and reported a bare
            // CommonError::ABORTED regardless of what the server had actually stated.
            auto ec=resp->error();
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
                ccb(ec);
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
    else
    {
        // Clean end of the stream (server returned OK). A pending read/write can never
        // complete now, so report it as aborted: for the event listener a stream that ended
        // without close() being requested is still "the stream is gone" and must drive the
        // same onDisconnected()/reconnect path as a transport failure. Previously only ccb
        // was invoked here, so a pending read callback was silently dropped and the listener
        // never learned that the stream had ended.
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

    std::shared_ptr<GrpcStream> self;
    {
        common::MutexScopedLock l{m_mutex};
        self=std::move(m_self);
    }
    m_transport->removeStream(m_channelPriority,self);
}

//--------------------------------------------------------------------------

void GrpcStream::readNext(ReadCb callback)
{
#if 0
    std::cerr << "GrpcStream::readNext closed " << m_closed << std::endl;
#endif
    if (m_closed)
    {
        callback(commonError(CommonError::ABORTED),{});
        return;
    }

    // Guard against issuing two concurrent StartRead ops on the same stream.
    // This can happen when a stale-stream callback fires on the app thread after
    // a reconnect has already armed a new StartRead on this stream. gRPC reacts
    // with GRPC_CALL_ERROR_TOO_MANY_OPERATIONS if StartRead is called twice
    // before OnReadDone fires. The dropped callback is intentional: the read that is
    // already pending owns the slot and its own callback will be delivered; the caller
    // that lost the race was a duplicate arm, not a distinct read request.
    if (m_readPending.exchange(true))
    {
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
#if 0
    std::cerr << "GrpcStream::close trying cancel" << std::endl;
#endif
    m_context->TryCancel();
}

//--------------------------------------------------------------------------

void GrpcStream::startStream(const grpc::ByteBuffer* initMsg, ReadCb callback)
{
#if 0
    std::cout << "GrpcStream::startStream" << std::endl;
#endif
    m_mutex.lock();
    m_readCallback=callback;
    m_self=shared_from_this();
    m_mutex.unlock();

    m_readPending=true;
    StartWrite(initMsg);
    StartWritesDone();
    StartRead(&m_responseBuffer);
    StartCall();
}

//--------------------------------------------------------------------------

HATN_GRPCCLIENT_NAMESPACE_END
