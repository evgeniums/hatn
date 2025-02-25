/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/securestreamtypes.h
  *
  *   Different types for secure streams
  *
  */

/****************************************************************************/

#ifndef HATNSECURESTREAMTYPES_H
#define HATNSECURESTREAMTYPES_H

#include <hatn/common/error.h>
#include <hatn/common/pmr/pmrtypes.h>
#include <hatn/common/stream.h>
#include <hatn/common/memorylockeddata.h>

#include <hatn/crypt/crypt.h>
#include <hatn/crypt/x509certificate.h>

HATN_CRYPT_NAMESPACE_BEGIN

using SecureStreamError=common::Error;
using SecureStreamErrors=common::pmr::vector<SecureStreamError>;

//! Container of different types for secure streams
struct SecureStreamTypes
{
    //! Endpoint type
    enum class Endpoint : int
    {
        Generic=0,
        Server=1,
        Client=2
    };

    //! Verification mode
    enum class Verification : int
    {
        None=0,
        Peer=1
    };

    //! Protocol version
    enum class ProtocolVersion : int
    {
        TLS1,
        TLS1_1,
        TLS1_2,
        TLS1_3,

        TLS_MAX=TLS1_3
    };
};

class SecureStreamContext;

//! Base class for secure streams with dynamic polimorphism
class HATN_CRYPT_EXPORT SecureStreamV : public common::StreamV
{
    public:

        //! Get context
        virtual SecureStreamContext* context() const noexcept
        {
            return nullptr;
        }

        //! Get TLS errors
        virtual SecureStreamErrors errors() const noexcept
        {
            return SecureStreamErrors();
        }

        //! Set errors
        virtual void setErrors(SecureStreamErrors errors) noexcept
        {
            std::ignore=errors;
        }

        //! Get endpoint type
        virtual SecureStreamTypes::Endpoint endpointType() const noexcept
        {
            return SecureStreamTypes::Endpoint::Generic;
        }
        //! Set endpoint type
        virtual void setEndpointType(SecureStreamTypes::Endpoint type)
        {
            std::ignore=type;
        }

        //! Add peer name to use in TLS verification
        virtual common::Error addPeerVerifyName(const X509Certificate::NameType& name)
        {
            std::ignore=name;
            return common::Error();
        }

        //! Set peer name to use in TLS verification overrideing previuosly added names
        virtual common::Error setPeerVerifyName(const X509Certificate::NameType& name)
        {
            std::ignore=name;
            return common::Error();
        }

        //! Get peer name that was verified in peer certificate
        virtual const char* getVerifiedPeerName() const
        {
            return nullptr;
        }

        //! Get peer certificate
        virtual common::SharedPtr<X509Certificate> getPeerCertificate() const
        {
            return common::SharedPtr<X509Certificate>();
        }

        //! Get native stream handler
        virtual void* nativeHandler() noexcept
        {
            return nullptr;
        }

        virtual void setMainCtx(common::TaskContext* mainContext) noexcept
        {
            std::ignore=mainContext;
        }
};

HATN_CRYPT_NAMESPACE_END

#endif // HATNSECURESTREAMTYPES_H
