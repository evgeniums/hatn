/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/openssldigest.h
 *
 * 	Digest/hash implementation with OpenSSL backend
 *
 */
/****************************************************************************/

#ifndef HATNOPENSSLDIGEST_H
#define HATNOPENSSLDIGEST_H

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>

#include <openssl/evp.h>

#include <hatn/common/nativehandler.h>

#include <hatn/crypt/digest.h>

#include <hatn/crypt/plugins/openssl/opensslerror.h>

HATN_OPENSSL_NAMESPACE_BEGIN

namespace detail
{
struct DigestTraits
{
    template <typename T> static Error init(T* obj) noexcept
    {
        if (obj->nativeHandler().isNull())
        {
            obj->nativeHandler().handler = ::EVP_MD_CTX_create();
            if (obj->nativeHandler().isNull())
            {
                return makeLastSslError(CryptError::NOT_SUPPORTED_BY_PLUGIN);
            }
        }
        return Error();
    }

    template <typename T> static void reset(T* obj) noexcept
    {
        if (!obj->nativeHandler().isNull())
        {
            ::EVP_MD_CTX_reset(obj->nativeHandler().handler);
        }
    }
};
}

namespace detail
{
struct MDTraits
{
    static void free(EVP_MD_CTX* ctx)
    {
        ::EVP_MD_CTX_free(ctx);
    }
};
}

class DigestAlg : public CryptAlgorithm
{
    public:

        DigestAlg(const CryptEngine* engine, const char* name) noexcept
            : CryptAlgorithm(engine,CryptAlgorithm::Type::DIGEST,name,0,::EVP_get_digestbyname(name))
        {
        }

        virtual size_t hashSize(bool) const override
        {
            return static_cast<size_t>(::EVP_MD_size(nativeHandler<EVP_MD>()));
        }

        virtual size_t blockSize() const override
        {
            return static_cast<size_t>(::EVP_MD_block_size(nativeHandler<EVP_MD>()));
        }
};

//! Digest/hash implementation
class HATN_OPENSSL_EXPORT OpenSslDigest :
                            public common::NativeHandlerContainer<EVP_MD_CTX,detail::MDTraits,Digest,OpenSslDigest>
{
    public:

        using common::NativeHandlerContainer<EVP_MD_CTX,detail::MDTraits,Digest,OpenSslDigest>::NativeHandlerContainer;

        static common::Error findNativeAlgorithm(
            std::shared_ptr<CryptAlgorithm> &alg,
            const char *name,
            CryptEngine* engine
        ) noexcept
        {
            alg=std::make_shared<DigestAlg>(engine,name);
            return common::Error();
        }

        static std::vector<std::string> listDigests();

    protected:

        /**
         * @brief Actually process data in derived class
         * @param buf Input buffer
         * @param size Size of input data
         * @return Operation status
         */
        virtual common::Error doProcess(
            const char* buf,
            size_t size
        ) noexcept override
        {
            if (::EVP_DigestUpdate(nativeHandler().handler,buf,size) != 1)
            {
                return makeLastSslError(CryptError::DIGEST_FAILED);
            }
            return Error();
        }

        /**
         * @brief Actually finalize processing and put result to buffer
         * @param buf Output buffer
         * @return Operation status
         */
        virtual common::Error doFinalize(
            char* buf
        ) noexcept override
        {
            if (::EVP_DigestFinal(nativeHandler().handler,reinterpret_cast<unsigned char*>(buf),nullptr) != 1)
            {
                return makeLastSslError(CryptError::DIGEST_FAILED);
            }
            return Error();
        }

        /**
         * @brief Init digest
         * @param nativeAlg Digest algoritnm, if null then use previuosly set algorithm
         * @return Operation status
         */
        virtual common::Error doInit() noexcept override
        {
            HATN_CHECK_RETURN(detail::DigestTraits::init(this))

            if (::EVP_DigestInit_ex(nativeHandler().handler,alg()->nativeHandler<EVP_MD>(),
                                alg()->engine()->nativeHandler<ENGINE>()
                                ) != 1)
            {
                return makeLastSslError(CryptError::DIGEST_FAILED);
            }
            return Error();
        }

        /**
         * @brief Reset digest so that it can be used again with new data
         *
         * @return Operation status
         */
        virtual void doReset() noexcept override
        {
            detail::DigestTraits::reset(this);
        }
};

HATN_OPENSSL_NAMESPACE_END

#endif // HATNOPENSSLDIGEST_H
