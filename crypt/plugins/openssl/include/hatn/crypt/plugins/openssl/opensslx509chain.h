/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslx509certificatechain.h
 *
 * 	Wrapper of X.509 certificate chain with OpenSSL backend
 *
 */
/****************************************************************************/

#ifndef HATNOPENSSLX509CHAIN_H
#define HATNOPENSSLX509CHAIN_H

#include <hatn/common/nativehandler.h>
#include <hatn/common/meta/pointertraits.h>

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>

#include <openssl/x509.h>
#include <openssl/pem.h>

#include <hatn/crypt/x509certificatechain.h>

HATN_OPENSSL_NAMESPACE_BEGIN

using X509CrtStack=common::PointerTraits<STACK_OF(X509*)>::Type;

namespace detail
{
struct X509CrtStackTraits
{
    static void free(X509CrtStack* stack)
    {
        if (stack!=nullptr)
        {
            auto count=::sk_X509_num(stack);
            for (int i=0;i<count;i++)
            {
                ::X509_free(sk_X509_value(stack,i));
            }
            ::sk_X509_free(stack);
        }
    }
};
}

//! Wrapper/container of X.509 certificate chain with OpenSSL backend
class HATN_OPENSSL_EXPORT OpenSslX509Chain :
                            public common::NativeHandlerContainer<X509CrtStack,detail::X509CrtStackTraits,X509CertificateChain,OpenSslX509Chain>
{
    public:

        //! Ctor
        using common::NativeHandlerContainer<X509CrtStack,detail::X509CrtStackTraits,X509CertificateChain,OpenSslX509Chain>
              ::NativeHandlerContainer;

        /**
         * @brief Parse PEM data to native chain
         * @param chain Parsed native chain object
         * @param data Pointer to buffer
         * @param size Size of data
         * @return Operation status
         */
        static Error parse(
            OpenSslX509Chain* chain,
            const char* data,
            size_t size
        ) noexcept;

        /**
         * @brief Serialize chain to PEM content
         * @param chain Native chain object
         * @param content Target ByteArray content
         * @return Operation status
         */
        static Error serialize(
            const OpenSslX509Chain* chain,
            common::ByteArray& content
        );

        /**
         * @brief Add certificate to chain
         * @param crt Certificate Authority
         * @return Error status
         */
        virtual common::Error addCertificate(const X509Certificate& crt) override;

        /**
         * @brief Get list of certificates
         * @param ec Error code
         * @return List of certificates
         */
        virtual std::vector<common::SharedPtr<X509Certificate>> certificates(common::Error& ec) const override;

        /**
         * @brief Load chain from buffer in PEM format
         * @param buf Pointer to buffer with PEM data
         * @param size Size of buffer
         * @param keepContent Keep content in this object
         * @return Error status
         */
        virtual common::Error importFromPemBuf(
            const char* buf,
            size_t size,
            bool keepContent=false
        ) override;

        /**
         * @brief Export to buffer in PEM format
         * @param buf Target buffer
         * @return Error status
         */
        virtual common::Error exportToPemBuf(
            common::ByteArray& buf
        ) override;

        //! Reset chain
        virtual void reset() override;

    private:

        void createNativeHandlerIfNull();
};

HATN_OPENSSL_NAMESPACE_END

#endif // HATNOPENSSLX509CHAIN_H
