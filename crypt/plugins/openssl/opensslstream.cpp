/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/openssl.cpp
  *
  *   TLS streams with OpenSSL as backend
  *
  */

/****************************************************************************/

#include <openssl/x509v3.h>

#include <hatn/common/utils.h>

#include <hatn/crypt/plugins/openssl/opensslcontext.h>
#include <hatn/crypt/plugins/openssl/opensslx509.h>
#include <hatn/crypt/plugins/openssl/opensslstream.h>
#include <hatn/crypt/plugins/openssl/opensslplugin.h>

//! \note Do not move this header upper, otherwise there are some conflicts of types on Windows platform
#include <hatn/common/makeshared.h>

#include <hatn/common/loggermoduleimp.h>

INIT_LOG_MODULE(opensslstream,HATN_OPENSSL_EXPORT)

HATN_OPENSSL_NAMESPACE_BEGIN

/*********************** OpenSslStreamTraits **************************/

//---------------------------------------------------------------
OpenSslStreamTraitsImpl::OpenSslStreamTraitsImpl(OpenSslStream *stream, OpenSslContext *context, common::Thread* thread)
    : m_stream(stream),
      m_ctx(context),
      m_ssl(::SSL_new(context->nativeContext())),
      m_bio(nullptr),
      m_writingToNext(false),
      m_readingFromNext(false),
      m_writeAppBuf(nullptr),
      m_writeAppSize(0),
      m_writtenAppSize(0),
      m_readAppBuf(nullptr),
      m_readAppMaxSize(0),
      m_readNextBufOffset(0),
      m_readNextBufSize(0),
      m_closed(false),
      m_shutdowning(false),
      m_shutdownNotifying(false),
      m_shutdownTimer(common::makeShared<common::AsioDeadlineTimer>(thread)),
      m_stopped(false)
{
    if (!m_ssl)
    {
        throw common::ErrorException(makeLastSslError(CryptError::GENERAL_FAIL));
    }

    ::SSL_set_mode(m_ssl, SSL_MODE_ENABLE_PARTIAL_WRITE);
    ::SSL_set_mode(m_ssl, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
    ::SSL_set_mode(m_ssl, SSL_MODE_RELEASE_BUFFERS);

    BIO* internalBio = nullptr;
    ::BIO_new_bio_pair(&internalBio,0,&m_bio,0);
    if (!m_bio)
    {
        ::SSL_free(m_ssl);
        throw common::ErrorException(makeLastSslError(CryptError::GENERAL_FAIL));
    }
    ::SSL_set_bio(m_ssl,internalBio,internalBio);

    m_readNextBuf.resize(BUF_SIZE);
    m_writeNextBuf.resize(BUF_SIZE);

    m_shutdownTimer->setPeriodUs(3000000);
}

//---------------------------------------------------------------
OpenSslStreamTraitsImpl::~OpenSslStreamTraitsImpl()
{
    HATN_DEBUG_ID_LVL(opensslstream,3,HATN_FORMAT("~OpenSslStreamTraitsImpl destroying {}",idStr()));

    if (m_ssl)
    {
        ::SSL_free(m_ssl);
    }
    if (m_bio)
    {
        ::BIO_vfree(m_bio);
    }
    m_ssl=nullptr;
    m_bio=nullptr;
}

//---------------------------------------------------------------
void OpenSslStreamTraitsImpl::checkNeedWriteNext()
{
    if (m_writingToNext)
    {
        HATN_DEBUG_ID_LVL(opensslstream,5,"checkNeedWriteNext(): already writinig to next");

        return;
    }

    size_t readyBytes=0;
    ::ERR_clear_error();
    int ret=::BIO_read_ex(m_bio,m_writeNextBuf.data(),m_writeNextBuf.size(),&readyBytes);

    HATN_DEBUG_ID_LVL(opensslstream,5,HATN_FORMAT("checkNeedWriteNext(): ret={}, readyBytes={}",ret,readyBytes));

    if (!processBioResult(ret))
    {
        return;
    }
    if (readyBytes>0)
    {
        doWriteNext(readyBytes);
    }
    else if (m_handshakingError)
    {
        fail(m_handshakingError);
    }
}

//---------------------------------------------------------------
void OpenSslStreamTraitsImpl::doWriteNext(size_t size, size_t offset)
{
    HATN_DEBUG_ID_LVL(opensslstream,5,HATN_FORMAT("doWriteNext(): size={}, offset={}",size,offset));

    auto&& cb=[this,size,offset](const Error& ec,size_t doneSize)
    {
        HATN_DEBUG_ID_LVL(opensslstream,5,HATN_FORMAT("doWriteNext() cb: doneSize={}, ec={}/{}",doneSize,ec.value(),ec.message()));

        if (m_handshakingError)
        {
            fail(m_handshakingError);
            return;
        }

        if (m_stopped)
        {
            HATN_DEBUG_ID_LVL(opensslstream,5,"doReadNext() cb: stopped");

            return;
        }

        m_writingToNext=false;
        if (!ec)
        {
            if (doneSize<size)
            {
                HATN_DEBUG_ID_LVL(opensslstream,5,"doWriteNext() cb: write next part");

                doWriteNext(size-doneSize,offset+doneSize);
            }
            else
            {
                if (m_shutdownNotifying)
                {
                    HATN_DEBUG_ID_LVL(opensslstream,5,"doWriteNext() cb: shutdown complete");

                    m_stopped=true;
                    doneOp();
                }
                else if (m_shutdowning)
                {
                    HATN_DEBUG_ID_LVL(opensslstream,5,"doWriteNext() cb: shutdowning");

                    doShutdown();
                }
                else
                {
                    if (m_writeCb)
                    {
                        if (m_writtenAppSize<m_writeAppSize)
                        {
                            HATN_DEBUG_ID_LVL(opensslstream,5,"doWriteNext() cb: go to writeSslPipe");

                            writeSslPipe(m_writeAppBuf+m_writtenAppSize,m_writeAppSize-m_writtenAppSize);
                            return;
                        }
                        else
                        {
                            HATN_DEBUG_ID_LVL(opensslstream,5,"doWriteNext() cb: call write callback");

                            auto writeCbTmp=std::exchange(m_writeCb,common::StreamChain::ResultCb());
                            m_writtenAppSize=0;
                            m_writeAppBuf=nullptr;
                            writeCbTmp(Error(),std::exchange(m_writeAppSize,0));
                        }
                    }
                    else
                    {
                        HATN_DEBUG_ID_LVL(opensslstream,5,"doWriteNext() cb: no write callback");
                    }

                    checkNeedWriteNext();
                }
            }
        }
        else
        {
            HATN_DEBUG_ID_LVL(opensslstream,5,"doWriteNext(): fail");

            fail(ec);
        }
    };
    m_writingToNext=true;
    if (m_shutdowning)
    {
        m_shutdownNotifying=true;
    }
    m_stream->writeNext(m_writeNextBuf.data()+offset,size,cb);
}

//---------------------------------------------------------------
void OpenSslStreamTraitsImpl::writeSslPipe(const char *data, size_t size)
{
    size_t writtenSize=0;
    ::ERR_clear_error();
    int ret=::SSL_write_ex(m_ssl,data,size,&writtenSize);

    HATN_DEBUG_ID_LVL(opensslstream,5,HATN_FORMAT("writeSslPipe(): ret={}, size={}, writtenSize={}",ret,size,writtenSize));

    if (writtenSize>0)
    {
        m_writtenAppSize+=writtenSize;
    }

    processSslResult(ret);
}

//---------------------------------------------------------------
void OpenSslStreamTraitsImpl::doReadNext()
{
    if (m_handshakingError)
    {
        HATN_DEBUG_ID_LVL(opensslstream,5,"doReadNext(): handshaking error");
        return;
    }
    if (m_readingFromNext)
    {
        HATN_DEBUG_ID_LVL(opensslstream,5,"doReadNext(): already reading");
        return;
    }
    if (m_shutdowning && !m_shutdownNotifying)
    {
        HATN_DEBUG_ID_LVL(opensslstream,5,"doReadNext(): shutdown waiting");
        return;
    }

    auto&& cb=[this](const common::Error& ec,size_t receivedBytes)
    {
        HATN_DEBUG_ID_LVL(opensslstream,5,HATN_FORMAT("doReadNext() cb: receivedBytes={}, ec={}/{}",receivedBytes,ec.value(),ec.message()));

        if (m_stopped)
        {
            HATN_DEBUG_ID_LVL(opensslstream,5,"doReadNext() cb: stopped");

            return;
        }

        m_readingFromNext=false;
        if (!ec)
        {
            if (m_shutdowning && !m_shutdownNotifying)
            {
                HATN_DEBUG_ID_LVL(opensslstream,5,"doReadNext() cb: shutdown waiting");
                if (!m_writingToNext)
                {
                    doShutdown();
                }
                return;
            }

            size_t writtenBytes=0;
            ::ERR_clear_error();
            int ret=::BIO_write_ex(m_bio,m_readNextBuf.data()+m_readNextBufOffset,receivedBytes,&writtenBytes);
            if (!processBioResult(ret))
            {
                HATN_DEBUG_ID_LVL(opensslstream,5,"doReadNext() cb: failed processBioResult");
                return;
            }

            m_readNextBufSize=receivedBytes-writtenBytes;
            m_readNextBufOffset+=writtenBytes;

            if (m_shutdowning)
            {
                HATN_DEBUG_ID_LVL(opensslstream,5,"doReadNext() cb: doShutdown");

                doShutdown();
            }
            else if (!m_stream->isOpen())
            {
                HATN_DEBUG_ID_LVL(opensslstream,5,"doReadNext() cb: doHandshake");

                doHandshake();
            }
            else
            {
                HATN_DEBUG_ID_LVL(opensslstream,5,"doReadNext() cb: readSslPipe");

                readSslPipe();
            }
        }
        else
        {
            HATN_DEBUG_ID_LVL(opensslstream,5,"doReadNext() cb: fail");

            fail(ec);
        }
    };

    HATN_DEBUG_ID_LVL(opensslstream,5,HATN_FORMAT("doReadNext(): m_readNextBufSize={}",m_readNextBufSize));

    m_readingFromNext=true;
    if (m_readNextBufSize>0)
    {
        cb(Error(),m_readNextBufSize);
    }
    else
    {
        m_readNextBufOffset=0;
        m_stream->readNext(m_readNextBuf.data(),m_readNextBuf.size(),cb);
    }
}

//---------------------------------------------------------------
void OpenSslStreamTraitsImpl::readSslPipe()
{
    if (m_readCb)
    {
        size_t readyBytes=0;
        ::ERR_clear_error();
        int ret=::SSL_read_ex(m_ssl,m_readAppBuf,m_readAppMaxSize,&readyBytes);

        HATN_DEBUG_ID_LVL(opensslstream,5,HATN_FORMAT("readSslPipe(): ret={}, readyBytes={}",ret,readyBytes));

        if (!processSslResult(ret))
        {
            return;
        }

        if (readyBytes>0)
        {
            auto readCbTmp=std::move(m_readCb);
            m_readCb=common::StreamChain::ResultCb();
            readCbTmp(Error(),readyBytes);
        }
        else
        {
            doReadNext();
        }
    }
}

//---------------------------------------------------------------
void OpenSslStreamTraitsImpl::doHandshake()
{
    if (m_writingToNext)
    {
        HATN_DEBUG_ID_LVL(opensslstream,5,"doHandshake(): already writinig to next");

        return;
    }

    //! @todo Refactor logging
    // HATN_DEBUG_ID_LVL(opensslstream,3,HATN_FORMAT("doHandshake(): before SSL_do_handshake: state={}",::SSL_get_state(m_ssl)));

    ::ERR_clear_error();
    int ret=::SSL_do_handshake(m_ssl);

    // HATN_DEBUG_ID_LVL(opensslstream,3,HATN_FORMAT("doHandshake(): after SSL_do_handshake: ret={}, state={}",ret,::SSL_get_state(m_ssl)));

    if (!processSslResult(ret,true))
    {
        // HATN_DEBUG_ID_LVL(opensslstream,3,"doHandshake(): fail processSslResult");
        return;
    }
    if (ret==1)
    {
        // HATN_DEBUG_ID_LVL(opensslstream,3,"doHandshake(): done");

        if (m_stream->errors().empty())
        {
            doneOp();
        }
        else
        {
            doneOp(m_stream->errors().front());
        }
    }
    else
    {
        // HATN_DEBUG_ID_LVL(opensslstream,3,"doHandshake(): not done");
    }
}

//---------------------------------------------------------------
void OpenSslStreamTraitsImpl::doShutdown()
{
    ::ERR_clear_error();
    int ret=::SSL_shutdown(m_ssl);

    HATN_DEBUG_ID_LVL(opensslstream,3,HATN_FORMAT("doShutdown(): ret={}",ret));
    if (ret==0)
    {
        ret=::SSL_shutdown(m_ssl);
        HATN_DEBUG_ID_LVL(opensslstream,3,HATN_FORMAT("doShutdown(): again: ret={}",ret));
    }

    if (ret<0)
    {
        processSslResult(ret);
    }
    else
    {
        HATN_DEBUG_ID_LVL(opensslstream,3,"doShutdown(): done");
        m_stopped=true;

        doneOp();
    }
}

//---------------------------------------------------------------
void OpenSslStreamTraitsImpl::doneOp(const Error &ec)
{
    HATN_DEBUG_ID_LVL(opensslstream,1,HATN_FORMAT("doneOp(): {} - {}",ec.value(),ec.message()));

    if (m_closed || m_shutdowning)
    {
        HATN_DEBUG_ID_LVL(opensslstream,3,"doneOp() closed");

        stream()->reset();

        if (m_readCb)
        {
            HATN_DEBUG_ID_LVL(opensslstream,3,"doneOp() read cb");

            auto readCbTmp=std::exchange(m_readCb,common::StreamChain::ResultCb());
            readCbTmp(ec,0);
        }
        if (m_writeCb)
        {
            HATN_DEBUG_ID_LVL(opensslstream,3,"doneOp() write cb");

            auto writeCbTmp=std::exchange(m_writeCb,common::StreamChain::ResultCb());
            writeCbTmp(ec,0);
        }
    }
    m_shutdownTimer->stop();
    if (m_opCb)
    {
        auto opCbTmp=std::exchange(m_opCb,common::StreamChain::OpCb());
        opCbTmp(ec);
    }
}

//---------------------------------------------------------------
void OpenSslStreamTraitsImpl::fail(const Error &ec)
{
    m_closed=true;
    m_stopped=true;

    HATN_DEBUG_ID_LVL(opensslstream,1,HATN_FORMAT("fail(): {} - {}",ec.value(),ec.message()));

    if (m_readCb)
    {
        HATN_DEBUG_ID_LVL(opensslstream,3,"Read fail");

        auto readCbTmp=std::exchange(m_readCb,common::StreamChain::ResultCb());
        readCbTmp(ec,0);
    }
    if (m_writeCb)
    {
        HATN_DEBUG_ID_LVL(opensslstream,3,"Write fail");

        auto writeCbTmp=std::exchange(m_writeCb,common::StreamChain::ResultCb());
        writeCbTmp(ec,0);
    }
    doneOp(ec);
}

//---------------------------------------------------------------
void OpenSslStreamTraitsImpl::cancel()
{
    auto ec=makeSystemError(std::errc::operation_canceled);

    HATN_DEBUG_ID_LVL(opensslstream,1,"cancel()");

    if (m_readCb)
    {
        HATN_DEBUG_ID_LVL(opensslstream,3,"Read cancelled");

        auto readCbTmp=std::exchange(m_readCb,common::StreamChain::ResultCb());
        readCbTmp(ec,0);
    }
    if (m_writeCb)
    {
        HATN_DEBUG_ID_LVL(opensslstream,3,"Write cancelled");

        auto writeCbTmp=std::exchange(m_writeCb,common::StreamChain::ResultCb());
        writeCbTmp(ec,0);
    }
    if (m_opCb)
    {
        HATN_DEBUG_ID_LVL(opensslstream,3,"Operation cancelled");

        auto opCbTmp=std::exchange(m_opCb,common::StreamChain::OpCb());
        opCbTmp(ec);
    }
}

//---------------------------------------------------------------
bool OpenSslStreamTraitsImpl::processSslResult(int ret, bool handshakingRead)
{
    if (ret<=0)
    {
        int sslRet=::SSL_get_error(m_ssl,ret);
        switch (sslRet)
        {
            case(SSL_ERROR_WANT_READ):
            {
                HATN_DEBUG_ID_LVL(opensslstream,5,"processSslResult() want read");

                checkNeedWriteNext();
                doReadNext();
            }
            break;

            case(SSL_ERROR_WANT_WRITE):
            {
                HATN_DEBUG_ID_LVL(opensslstream,5,"processSslResult() want write");
                checkNeedWriteNext();
            }
            break;

            case(SSL_ERROR_ZERO_RETURN):
            {
                fail(makeLastSslError(CryptError::TLS_CLOSED));
                return false;
            }
            break;

            case(SSL_ERROR_SSL): HATN_FALLTHROUGH
            case(SSL_ERROR_SYSCALL):
            {
                HATN_DEBUG_ID_LVL(opensslstream,5,"processSslResult() fail");

                if (handshakingRead)
                {
                    // try to send error report to peer
                    m_handshakingError=makeLastSslError(CryptError::TLS_ERROR);
                    checkNeedWriteNext();
                }
                else
                {
                    fail(makeLastSslError(CryptError::TLS_ERROR));
                }

                return false;
            }
            break;

            default:
            {
                HATN_DEBUG_ID_LVL(opensslstream,5,HATN_FORMAT("processSslResult(): default ret={}, sslRet={}",ret,sslRet));
                checkNeedWriteNext();
            }
            break;
        }
    }
    else
    {
        HATN_DEBUG_ID_LVL(opensslstream,5,HATN_FORMAT("processSslResult(): ret={}",ret));
        checkNeedWriteNext();
    }
    return true;
}

//---------------------------------------------------------------
bool OpenSslStreamTraitsImpl::processBioResult(int ret)
{
    if (ret<=0 && BIO_should_retry(m_bio)==0)
    {
        HATN_DEBUG_ID_LVL(opensslstream,5,HATN_FORMAT("processBioResult(): fail, ret={}",ret));

        fail(makeLastSslError());
        return false;
    }

    HATN_DEBUG_ID_LVL(opensslstream,5,HATN_FORMAT("processBioResult(): ok, ret={}",ret));
    return true;
}

//---------------------------------------------------------------
Error OpenSslStreamTraitsImpl::addPeerVerifyName(const X509Certificate::NameType &name)
{
    HATN_DEBUG_ID_LVL(opensslstream,1,HATN_FORMAT("addPeerVerifyName(): {};",name.c_str()));
    ::SSL_set_hostflags(m_ssl,
                        X509_CHECK_FLAG_ALWAYS_CHECK_SUBJECT
                        |
                        X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS
                        |
                        X509_CHECK_FLAG_MULTI_LABEL_WILDCARDS);

    ::ERR_clear_error();

    if (::SSL_add1_host(m_ssl,name.c_str()) != 1)
    {
        return makeLastSslError();
    }

    return Error();
}

//---------------------------------------------------------------
Error OpenSslStreamTraitsImpl::setPeerVerifyName(const X509Certificate::NameType &name)
{
    HATN_DEBUG_ID_LVL(opensslstream,1,HATN_FORMAT("setPeerVerifyName(): {};",name.c_str()));
    ::SSL_set_hostflags(m_ssl,
                        X509_CHECK_FLAG_ALWAYS_CHECK_SUBJECT
                        |
                        X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS
                        |
                        X509_CHECK_FLAG_MULTI_LABEL_WILDCARDS);

    ::ERR_clear_error();

    if (::SSL_set1_host(m_ssl,name.c_str()) != 1)
    {
        return makeLastSslError();
    }

    return Error();
}

//---------------------------------------------------------------
static int SslVerifyCb(int preverifyOk, X509_STORE_CTX *x509Ctx)
{
    if (preverifyOk==1)
    {
        return preverifyOk;
    }
    if (x509Ctx)
    {
        auto idx=SSL_get_ex_data_X509_STORE_CTX_idx();
        if (idx<0)
        {
            return 0;
        }
        SSL* ssl=static_cast<SSL*>(::X509_STORE_CTX_get_ex_data(x509Ctx,idx));
        if (ssl)
        {
            OpenSslStreamTraitsImpl* streamTraits=static_cast<OpenSslStreamTraitsImpl*>(::SSL_get_ex_data(ssl,OpenSslPlugin::sslCtxIdx()));
            if (streamTraits)
            {
                int code=::X509_STORE_CTX_get_error(x509Ctx);
                X509* nativeCrt=::X509_STORE_CTX_get_current_cert(x509Ctx);
                if (!nativeCrt)
                {
                    return 0;
                }
                ::X509_up_ref(nativeCrt);
                auto cert=common::makeShared<OpenSslX509>();
                cert->setNativeHandler(nativeCrt);

                HATN_DEBUG_CONTEXT(opensslstream,streamTraits->idStr(),3,HATN_FORMAT("SslVerifyCb() error: ({}) {}, certificate={}",
                                                                                   code,OpenSslError::codeToString(code),
                                                                                   cert->toString(true,false)));

                auto&& ec=makeSslError(code,std::move(cert));
                if (streamTraits->stream()->context()->checkAllErrorsIgnored()
                        ||
                    streamTraits->stream()->context()->isErrorIgnored(ec)
                   )
                {
                    HATN_DEBUG_CONTEXT(opensslstream,streamTraits->idStr(),3,"SslVerifyCb(): error ignored");

                    return 1;
                }
                else if (streamTraits->stream()->context()->checkCollectAllErrors())
                {
                    HATN_DEBUG_CONTEXT(opensslstream,streamTraits->idStr(),3,"SslVerifyCb(): error collected and skipped for later fail report");

                    streamTraits->stream()->addError(std::move(ec));
                    return 1;
                }
                else
                {
                    HATN_DEBUG_CONTEXT(opensslstream,streamTraits->idStr(),3,"SslVerifyCb(): error activated");

                    streamTraits->stream()->addError(std::move(ec));
                    return 0;
                }
            }
        }
    }
    return 0;
}

//---------------------------------------------------------------
void OpenSslStreamTraitsImpl::prepare(std::function<void (const Error &)> callback)
{
    if (::SSL_set_ex_data(m_ssl,OpenSslPlugin::sslCtxIdx(),this)!=1)
    {
        fail(makeLastSslError(CryptError::GENERAL_FAIL));
        return;
    }

    m_opCb=std::move(callback);
    auto server=m_stream->endpointType()==SecureStreamTypes::Endpoint::Server;
    if (server)
    {
        ::SSL_set_accept_state(m_ssl);
        HATN_DEBUG_ID_LVL(opensslstream,1,"Prepare server mode");
    }
    else
    {
        ::SSL_set_connect_state(m_ssl);
        HATN_DEBUG_ID_LVL(opensslstream,1,"Prepare client mode");
    }

    if (
           m_stream->context()->checkCollectAllErrors()
        || m_stream->context()->checkAllErrorsIgnored()
        || !m_stream->context()->ignoredErrors().empty()
       )
    {
        HATN_DEBUG_ID_LVL(opensslstream,3,"Set verify callback");

        // set verify callback only if need to collect all errors at once or some errors can be ignored
        ::SSL_set_verify(m_ssl,::SSL_get_verify_mode(m_ssl),SslVerifyCb);
    }

    doHandshake();
}

//---------------------------------------------------------------
const char* OpenSslStreamTraitsImpl::getVerifiedPeerName() const
{
    return ::SSL_get0_peername(const_cast<OpenSslStreamTraitsImpl*>(this)->m_ssl);
}

//---------------------------------------------------------------
common::SharedPtr<X509Certificate> OpenSslStreamTraitsImpl::getPeerCertificate() const
{
    auto nativeCrt=::SSL_get_peer_certificate(m_ssl);
    if (nativeCrt)
    {
        auto cert=common::makeShared<OpenSslX509>();
        cert->setNativeHandler(nativeCrt);
        return cert;
    }
    return common::SharedPtr<X509Certificate>();
}

//---------------------------------------------------------------
OpenSslStream::~OpenSslStream() = default;

//---------------------------------------------------------------

HATN_OPENSSL_NAMESPACE_END
