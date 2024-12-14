/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslhmac.h
 *
 * 	HMAC (Hash Message Authentication Code) implementation with OpenSSL backend
 *
 */
/****************************************************************************/

#ifndef HATNOPENSSLHMAC_H
#define HATNOPENSSLHMAC_H

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>

#include <boost/algorithm/string.hpp>

#include <openssl/hmac.h>

#include <hatn/common/nativehandler.h>
#include <hatn/common/utils.h>

#include <hatn/crypt/hmac.h>

#include <hatn/crypt/plugins/openssl/opensslerror.h>
#include <hatn/crypt/plugins/openssl/openssldigest.h>
#include <hatn/crypt/plugins/openssl/opensslcipher.h>
#include <hatn/crypt/plugins/openssl/opensslmackey.h>

#if OPENSSL_API_LEVEL < 30100

HATN_OPENSSL_NAMESPACE_BEGIN

//! HMAC algorithm
class HATN_OPENSSL_EXPORT HMACAlg : public CryptAlgorithm
{
    public:

        HMACAlg(const CryptEngine* engine, const char* name);

        virtual size_t keySize() const override
        {
            // can be of any size, let it be the size of hash result
            return EVP_MD_size(nativeHandler<EVP_MD>());
        }

        virtual size_t hashSize(bool) const override
        {
            return static_cast<size_t>(EVP_MD_size(nativeHandler<EVP_MD>()));
        }

        virtual size_t blockSize() const override
        {
            return static_cast<size_t>(EVP_MD_block_size(nativeHandler<EVP_MD>()));
        }

        virtual common::SharedPtr<MACKey> createMACKey() const override;
};

namespace detail
{
struct HMACTraits
{
    static void free(HMAC_CTX* ctx);
};
}

//! HMAC (Hash Message Authentication Code) implementation
class HATN_OPENSSL_EXPORT OpenSslHMAC : public common::NativeHandlerContainer<HMAC_CTX,detail::HMACTraits,HMAC,OpenSslHMAC>
{
    public:

        using common::NativeHandlerContainer<HMAC_CTX,detail::HMACTraits,HMAC,OpenSslHMAC>::NativeHandlerContainer;

        static common::Error findNativeAlgorithm(
            std::shared_ptr<CryptAlgorithm> &alg,
            const char *name,
            CryptEngine* engine
        ) noexcept;

    protected:

        /**
         * @brief Actually process data
         * @param buf Input buffer
         * @param size Size of input data
         * @return Operation status
         */
        virtual common::Error doProcess(
            const char* buf,
            size_t size
        ) noexcept override;

        /**
         * @brief Actually finalize processing and put result to buffer
         * @param buf Output buffer
         * @return Operation status
         */
        virtual common::Error doFinalize(
            char* buf
        ) noexcept override;

        /**
         * @brief Init HMAC
         * @param nativeAlg Digest algoritnm, if null then use previuosly set algorithm
         * @return Operation status
         */
        virtual common::Error doInit() noexcept override;

        /**
         * @brief Reset digest so that it can be used again with new data
         *
         * @return Operation status
         */
        virtual void doReset() noexcept override;
};

HATN_OPENSSL_NAMESPACE_END

#endif

#endif // HATNOPENSSLHMAC_H
