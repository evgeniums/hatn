/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/securestream.h
  *
  *   Base class for secure stream
  *
  */

/****************************************************************************/

#ifndef HATNSECURESTREAM_H
#define HATNSECURESTREAM_H

#include <functional>

#include <hatn/common/stream.h>

#include <hatn/crypt/crypt.h>
#include <hatn/crypt/securestreamtypes.h>
#include <hatn/crypt/securestreamcontext.h>

HATN_COMMON_NAMESPACE_BEGIN

class Thread;

HATN_COMMON_NAMESPACE_END

HATN_CRYPT_NAMESPACE_BEGIN

//! Base template class for secure streams
template <typename Traits>
class SecureStream : public common::StreamWithIDThread<Traits>,
                     public common::StreamChain
{
    public:

        //! Constructor
        template <typename ...Args>
        SecureStream(
            SecureStreamContext* context,
            common::Thread* thread,
            common::STR_ID_TYPE id,
            Args&& ...traitsArgs
        ) : common::StreamWithIDThread<Traits>::StreamWithIDThread(thread,std::move(id),std::forward<Args>(traitsArgs)...),
            m_context(context),
            m_endpointType(context->endpointType())
        {}

        //! Constructor
        template <typename ...Args>
        SecureStream(
            SecureStreamContext* context,
            common::Thread* thread,
            Args&& ...traitsArgs
        ) : common::StreamWithIDThread<Traits>::StreamWithIDThread(thread,std::forward<Args>(traitsArgs)...),
            m_context(context),
            m_endpointType(context->endpointType())
        {}

        //! Constructor
        template <typename ...Args>
        SecureStream(
            SecureStreamContext* context,
            common::STR_ID_TYPE id,
            Args&& ...traitsArgs
        ) : SecureStream(context,common::Thread::currentThread(),std::move(id),std::forward<Args>(traitsArgs)...)
        {}

        //! Constructor
        template <typename ...Args>
        SecureStream(
            SecureStreamContext* context,
            Args&& ...traitsArgs
        ) : SecureStream(context,common::Thread::currentThread(),std::forward<Args>(traitsArgs)...)
        {}

        //! Get context
        inline SecureStreamContext* context() const noexcept
        {
            return m_context;
        }

        //! Get TLS errors
        inline SecureStreamErrors errors() const noexcept
        {
            return m_errors;
        }

        //! Clear list of stream errors
        inline void clearErrors() noexcept
        {
            m_errors.clear();
        }

        //! Append error to list of stream errors
        inline void addError(SecureStreamError error) noexcept
        {
            m_errors.push_back(std::move(error));
        }

        //! Get endpoint type
        inline SecureStreamTypes::Endpoint endpointType() const noexcept
        {
            return m_endpointType;
        }
        //! Set endpoint type
        inline void setEndpointType(SecureStreamTypes::Endpoint type) noexcept
        {
            m_endpointType=type;
        }

        //! Add peer name to use in TLS verification
        inline common::Error addPeerVerifyName(const X509Certificate::NameType& name)
        {
            return this->traits().addPeerVerifyName(name);
        }

        //! Set peer name to use in TLS verification overrideing previuosly added names
        inline common::Error setPeerVerifyName(const X509Certificate::NameType& name)
        {
            return this->traits().setPeerVerifyName(name);
        }

        //! Get peer name that was verified in peer certificate
        inline const char* getVerifiedPeerName() const
        {
            return this->traits().getVerifiedPeerName();
        }

        //! Get peer certificate
        inline common::SharedPtr<X509Certificate> getPeerCertificate() const
        {
            return this->traits().getPeerCertificate();
        }

        //! Get native stream handler
        inline void* nativeHandler() noexcept
        {
            return this->traits().nativeHandler();
        }

    protected:

        //! Add peer name to use in TLS verification
        common::Error doAddPeerVerifyName(const X509Certificate::NameType& name)
        {
            std::ignore=name;
            return common::Error();
        }

        //! Set peer name to use in TLS verification overrideing previuosly added names
        common::Error doSetPeerVerifyName(const X509Certificate::NameType& name)
        {
            std::ignore=name;
            return common::Error();
        }

    private:

        SecureStreamContext* m_context;
        SecureStreamErrors m_errors;
        SecureStreamTypes::Endpoint m_endpointType;
};

//! Base template class for secure streams
template <typename ImplT>
class SecureStreamTmplV : public common::StreamTmplV<ImplT,SecureStreamV>
{
    public:

        using common::StreamTmplV<ImplT,SecureStreamV>::StreamTmplV;

        //! Get context
        virtual SecureStreamContext* context() const noexcept override
        {
            return this->impl().context();
        }

        //! Get stream errors
        virtual SecureStreamErrors errors() const noexcept override
        {
            return this->impl().errors();
        }

        //! Get endpoint type
        virtual SecureStreamTypes::Endpoint endpointType() const noexcept override
        {
            return this->impl().endpointType();
        }
        //! Set endpoint type
        virtual void setEndpointType(SecureStreamTypes::Endpoint type) override
        {
            this->impl().setEndpointType(type);
        }

        //! Add peer name to use in TLS verification
        virtual common::Error addPeerVerifyName(const X509Certificate::NameType& name) override
        {
            return this->impl().addPeerVerifyName(name);
        }

        //! Set peer name to use in TLS verification overrideing previuosly added names
        virtual common::Error setPeerVerifyName(const X509Certificate::NameType& name) override
        {
            return this->impl().setPeerVerifyName(name);
        }

        //! Get peer name that was verified in peer certificate
        virtual const char* getVerifiedPeerName() const override
        {
            return this->impl().getVerifiedPeerName();
        }

        //! Get peer certificate
        virtual common::SharedPtr<X509Certificate> getPeerCertificate() const override
        {
            return this->impl().getPeerCertificate();
        }

        //! Get native stream handler
        virtual void* nativeHandler() noexcept override
        {
            return this->impl().nativeHandler();
        }
};

HATN_CRYPT_NAMESPACE_END

#endif // HATNSECURESTREAM_H
