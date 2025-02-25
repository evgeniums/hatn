/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslstream.h
  *
  *   TLS stream with OpenSSL as backend
  *
  */

/****************************************************************************/

#ifndef HATNOPENSSLSOCKET_H
#define HATNOPENSSLSOCKET_H

#include <hatn/common/logger.h>
#include <hatn/common/bytearray.h>
#include <hatn/common/asiotimer.h>

#include <hatn/logcontext/contextlogger.h>

#include <hatn/crypt/securestream.h>

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>
#include <hatn/crypt/plugins/openssl/opensslerror.h>
#include <hatn/crypt/plugins/openssl/opensslcontext.h>

DECLARE_LOG_MODULE_EXPORT(opensslstream,HATN_OPENSSL_EXPORT)

HATN_OPENSSL_NAMESPACE_BEGIN

class OpenSslStream;

constexpr const uint8_t StreamDebugVerbosity=5;
constexpr const uint8_t HandshakeDebugVerbosity=3;
constexpr const uint8_t ShutdownDebugVerbosity=3;
constexpr const uint8_t DoneDebugVerbosity=1;

//! TLS stream with OpenSSL as backend
class HATN_OPENSSL_EXPORT OpenSslStreamTraitsImpl
{
    public:

        constexpr static const size_t BUF_SIZE=0x4000u;

        //! Constructor
        OpenSslStreamTraitsImpl(
            OpenSslStream* stream,
            OpenSslContext* context,
            common::Thread* thread
        );

        //! Destructor
        ~OpenSslStreamTraitsImpl();

        OpenSslStreamTraitsImpl(OpenSslStreamTraitsImpl&& other)=delete;
        OpenSslStreamTraitsImpl& operator=(OpenSslStreamTraitsImpl&&)=delete;
        OpenSslStreamTraitsImpl& operator=(OpenSslStreamTraitsImpl&)=delete;
        OpenSslStreamTraitsImpl(OpenSslStreamTraitsImpl&)=delete;

        //! Get openssl stream context
        inline OpenSslContext* ctx() const noexcept
        {
            return m_ctx;
        }

        //! Write to stream in derived class
        inline void write(
            const char* data,
            std::size_t size,
            std::function<void (const Error&,size_t)> callback
        )
        {
            HATN_CTX_SCOPE("osslwrite")

            if (m_closed)
            {
                HATN_CTX_DEBUG(StreamDebugVerbosity,"stream closed",HLOG_MODULE(opensslstream))
                callback(makeSystemError(std::errc::connection_aborted),0);
                return;
            }

            HATN_CTX_DEBUG(StreamDebugVerbosity,"do write",HLOG_MODULE(opensslstream))

            m_writtenAppSize=0;
            m_writeAppBuf=data;
            m_writeAppSize=size;
            m_writeCb=std::move(callback);

            writeSslPipe(m_writeAppBuf,m_writeAppSize);
        }

        //! Read from stream in derived class
        inline void read(
            char* data,
            std::size_t maxSize,
            std::function<void (const Error&,size_t)> callback
        )
        {
            HATN_CTX_SCOPE("osslread")

            if (m_closed)
            {
                HATN_CTX_DEBUG(StreamDebugVerbosity,"stream closed",HLOG_MODULE(opensslstream))
                callback(makeSystemError(std::errc::connection_aborted),0);
                return;
            }

            HATN_CTX_DEBUG(StreamDebugVerbosity,"do read",HLOG_MODULE(opensslstream))

            m_readAppBuf=data;
            m_readAppMaxSize=maxSize;
            m_readCb=std::move(callback);

            readSslPipe();
        }

        inline bool isOpen() const noexcept
        {
            return ::SSL_is_init_finished(m_ssl)==1;
        }

        /**
         * @brief Do handshake
         * @param callback Status handshaking
         */
        void prepare(
            std::function<void (const Error &)> callback
        );

        //! Close stream
        inline void close(const std::function<void (const Error &)>& callback, bool destroying)
        {
            HATN_CTX_SCOPE("osslclose")

            if (m_closed)
            {
                HATN_CTX_DEBUG(StreamDebugVerbosity,"stream closed",HLOG_MODULE(opensslstream))

                if (callback)
                {
                    callback(Error());
                }
                return;
            }

            if (!destroying)
            {
                HATN_CTX_DEBUG(StreamDebugVerbosity,"go to doShutdown",HLOG_MODULE(opensslstream))

                cancel();
                m_opCb=callback;

                m_shutdownTimer->start(
                    [callback,this](common::TimerTypes::Status status)
                    {
                        if (status==common::TimerTypes::Status::Timeout && callback)
                        {
                            fail(common::Error(common::CommonError::TIMEOUT));
                        }
                    }
                );

                m_shutdowning=true;
                doShutdown();
            }
            else
            {
                HATN_CTX_DEBUG(StreamDebugVerbosity,"do close",HLOG_MODULE(opensslstream))

                m_writeCb=decltype(m_writeCb)();
                m_readCb=decltype(m_writeCb)();
                m_opCb=decltype(m_opCb)();
            }
        }

        void cancel();

        void reset()
        {}

        //! Add peer name to use in TLS verification
        common::Error addPeerVerifyName(const X509Certificate::NameType& name);

        //! Set peer name to use in TLS verification overrideing previuosly added names
        common::Error setPeerVerifyName(const X509Certificate::NameType& name);

        inline OpenSslStream* stream() const noexcept
        {
            return m_stream;
        }

        //! Get peer name that was verified in peer certificate
        const char* getVerifiedPeerName() const;

        //! Get peer certificate
        common::SharedPtr<X509Certificate> getPeerCertificate() const;

        //! Get native stream handler
        inline void* nativeHandler() noexcept
        {
            return m_ssl;
        }

    private:

        void writeSslPipe(const char* data,size_t size);
        void checkNeedWriteNext();
        void doWriteNext(size_t size,size_t offset=0);

        void readSslPipe();
        void doReadNext();

        void fail(const Error &ec);

        void doShutdown();
        void doHandshake();

        bool processSslResult(int ret, bool handshakingRead=false);
        bool processBioResult(int ret);

        void doneOp(const Error& ec=Error());

    private:

        OpenSslStream* m_stream;

        OpenSslContext* m_ctx;
        SSL* m_ssl;
        BIO* m_bio;

        bool m_writingToNext;
        bool m_readingFromNext;

        common::ByteArray m_writeNextBuf;
        const char* m_writeAppBuf;
        size_t m_writeAppSize;
        size_t m_writtenAppSize;

        common::ByteArray m_readNextBuf;
        char* m_readAppBuf;
        size_t m_readAppMaxSize;
        size_t m_readNextBufOffset;
        size_t m_readNextBufSize;

        common::StreamChain::ResultCb m_writeCb;
        common::StreamChain::ResultCb m_readCb;
        common::StreamChain::OpCb m_opCb;

        Error m_handshakingError;
        bool m_closed;
        bool m_shutdowning;
        bool m_shutdownNotifying;

        //! @todo Use timer on stack as part of context
        common::SharedPtr<common::AsioDeadlineTimer> m_shutdownTimer;
        bool m_stopped;
};

using OpenSslStreamTraits=common::StreamGatherTraits<OpenSslStreamTraitsImpl>;

class HATN_OPENSSL_EXPORT OpenSslStream : public SecureStream<OpenSslStreamTraits>
{
    public:

        //! Constructor
        OpenSslStream(
            OpenSslContext* context,
            common::Thread* thread
        ) : SecureStream<OpenSslStreamTraits>(
                    context,
                    thread,
                    this,
                    context,
                    thread
                )
        {}

        //! Constructor
        OpenSslStream(
            OpenSslContext* context
        ) : OpenSslStream(context,common::Thread::currentThread())
        {}

        ~OpenSslStream();
        OpenSslStream(const OpenSslStream&) = delete;
        OpenSslStream(OpenSslStream&&) = delete;
        OpenSslStream& operator=(const OpenSslStream&) = delete;
        OpenSslStream& operator=(OpenSslStream&&) = delete;
};

HATN_OPENSSL_NAMESPACE_END

#endif // HATNOPENSSLSOCKET_H
