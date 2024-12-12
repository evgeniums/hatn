/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/openssldigestsign.h
 *
 * 	Base class for EVP_DigestSign* utils of OpenSSL backend
 *
 */
/****************************************************************************/

#ifndef HATNOPENSSLDIGESTSIGN_H
#define HATNOPENSSLDIGESTSIGN_H

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>

#include <openssl/evp.h>

#include <hatn/common/nativehandler.h>
#include <hatn/common/utils.h>

#include <hatn/crypt/plugins/openssl/opensslerror.h>
#include <hatn/crypt/plugins/openssl/openssldigest.h>
#include <hatn/crypt/plugins/openssl/opensslmackey.h>
#include <hatn/crypt/plugins/openssl/opensslprivatekey.h>

HATN_OPENSSL_NAMESPACE_BEGIN

namespace detail
{

struct DigestSignTraits
{
    static void free(EVP_MD_CTX* ctx)
    {
        ::EVP_MD_CTX_destroy(ctx);
    }
};

template <CryptAlgorithm::Type Type>
struct DigestSignPkeyTraits
{
};

template <>
struct DigestSignPkeyTraits<CryptAlgorithm::Type::MAC>
{
    static EVP_PKEY* nativeKey(const SecureKey* key) noexcept
    {
        if (!key->hasRole(SecureKey::Role::MAC) || !key->isBackendKey())
        {
            return nullptr;
        }
        const OpenSslMACKey* pkey=dynamic_cast<const OpenSslMACKey*>(key);
        if (pkey==nullptr)
        {
            return nullptr;
        }
        return pkey->nativeHandler().handler;
    }
    constexpr static const CryptError FailedCode=CryptError::MAC_FAILED;
    template <typename DigestT>
    static Error additionalInit(DigestT* digest,EVP_PKEY_CTX * pctx) noexcept;
};

template <>
struct DigestSignPkeyTraits<CryptAlgorithm::Type::SIGNATURE>
{
    static EVP_PKEY* nativeKey(const SecureKey* key) noexcept
    {
        if (!key->hasRole(SecureKey::Role::SIGN))
        {
            return nullptr;
        }
        const OpenSslPrivateKey* pkey=dynamic_cast<const OpenSslPrivateKey*>(key);
        if (pkey==nullptr)
        {
            return nullptr;
        }
        return pkey->nativeHandler().handler;
    }
    constexpr static const CryptError FailedCode=CryptError::SIGN_FAILED;
    template <typename DigestT>
    static Error additionalInit(DigestT* digest,EVP_PKEY_CTX * pctx) noexcept
    {
        std::ignore=digest;
        std::ignore=pctx;
        return Error();
    }
};

}

template <typename BaseT,CryptAlgorithm::Type AlgT> class OpenSslDigestSign :
                          public common::NativeHandlerContainer<EVP_MD_CTX,detail::DigestSignTraits,BaseT,OpenSslDigestSign<BaseT,AlgT>>
{
    public:

        using common::NativeHandlerContainer<EVP_MD_CTX,detail::DigestSignTraits,BaseT,OpenSslDigestSign<BaseT,AlgT>>::NativeHandlerContainer;

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

#endif // HATNOPENSSLDIGESTSIGN_H
