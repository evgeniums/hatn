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

#ifndef HATNOPENSSLMAC_H
#define HATNOPENSSLMAC_H

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>

#include <openssl/evp.h>
#include <openssl/core_names.h>

#include <hatn/common/nativehandler.h>
#include <hatn/common/utils.h>
#include <hatn/common/makeshared.h>

#include <hatn/crypt/mac.h>

#include <hatn/crypt/plugins/openssl/opensslerror.h>
#include <hatn/crypt/plugins/openssl/opensslmackey.h>
#include <hatn/crypt/plugins/openssl/openssldigestsign.h>

HATN_OPENSSL_NAMESPACE_BEGIN

#if OPENSSL_API_LEVEL >= 30100

//! Any MAC method stub
class MacMethod
{
    public:

        static const MacMethod* stub()
        {
            static MacMethod m;
            return &m;
        }
};

//! MAC algorithm
class HATN_OPENSSL_EXPORT MACAlg : public CryptAlgorithm
{
    public:

    MACAlg(const CryptEngine* engine, const char* name, std::string algName, int nativeType, CryptAlgorithm::Type type=CryptAlgorithm::Type::MAC) noexcept
                : CryptAlgorithm(engine,type,name),
                  m_nativeType(nativeType),
                  m_algName(std::move(algName))
            {}

            static MACAlg& masterReference() noexcept;

            int nativeType() const noexcept
            {
                return m_nativeType;
            }

            virtual common::SharedPtr<MACKey> createMACKey() const override
            {
                auto key=common::makeShared<MACKey>();
                key->setAlg(this);
                return key;
            }

            const char* algName() const noexcept
            {
                return m_algName.c_str();
            }

            virtual size_t prepareParams(OSSL_PARAM* params, size_t maxCount) const
            {
                std::ignore=params;
                std::ignore=maxCount;
                return 0;
            }

    private:

        int m_nativeType;
        std::string m_algName;
};

//! MAC algorithm based on HMAC
class MACAlg_HMAC : public MACAlg
{
    public:

        MACAlg_HMAC(const CryptEngine* engine, const char* name, const char* digestName, CryptAlgorithm::Type type=CryptAlgorithm::Type::MAC) noexcept
            : MACAlg(engine,name,"HMAC",EVP_PKEY_HMAC,type)
        {
            setHandler(EVP_get_digestbyname(digestName));
        }

        virtual size_t keySize() const override
        {
            // can be of any size, let it be the size of hash result
            return EVP_MD_size(nativeHandler<EVP_MD>());
        }

        virtual size_t hashSize(bool) const override
        {
            return static_cast<size_t>(::EVP_MD_size(nativeHandler<EVP_MD>()));
        }

        virtual size_t blockSize() const override
        {
            return static_cast<size_t>(::EVP_MD_block_size(nativeHandler<EVP_MD>()));
        }

        virtual size_t prepareParams(OSSL_PARAM* params, size_t maxCount) const
        {
            std::ignore=params;
            std::ignore=maxCount;
            params[0]=OSSL_PARAM_construct_utf8_string(OSSL_MAC_PARAM_DIGEST,const_cast<char*>(EVP_MD_get0_name(nativeHandler<EVP_MD>())),0);
            return 1;
        }
};

//! MAC algorithm based on Poly1305
class MACAlg_Poly1305 : public MACAlg
{
    public:

        MACAlg_Poly1305(const CryptEngine* engine, const char* name) noexcept
            : MACAlg(engine,name,"POLY1305",EVP_PKEY_POLY1305)
        {
            setHandler(MacMethod::stub());
        }

        virtual size_t keySize() const override
        {
            return 32;
        }

        virtual size_t hashSize(bool) const override
        {
            return 16;
        }

        virtual size_t blockSize() const override
        {
            return 16;
        }
};

//! MAC algorithm based on SIPHASH
class MACAlg_SIPHASH : public MACAlg
{
    public:

        MACAlg_SIPHASH(const CryptEngine* engine, const char* name, size_t hashSize=16) noexcept
            : MACAlg(engine,name,"SIPHASH",EVP_PKEY_SIPHASH),
              m_hashSize(hashSize)
        {
            setHandler(MacMethod::stub());
        }

        virtual size_t keySize() const override
        {
            return 16;
        }

        virtual size_t hashSize(bool) const override
        {
            return m_hashSize;
        }

        virtual size_t blockSize() const override
        {
            return 16;
        }

        virtual size_t prepareParams(OSSL_PARAM* params, size_t maxCount) const
        {
            std::ignore=maxCount;
            params[0]=OSSL_PARAM_construct_uint64(OSSL_MAC_PARAM_SIZE,const_cast<size_t*>(&m_hashSize));
            return 1;
        }

    private:

        size_t m_hashSize;
};

//! MAC algorithm based on CMAC
class MACAlg_CMAC : public MACAlg
{
    public:

        MACAlg_CMAC(const CryptEngine* engine, const char* name) noexcept
            : MACAlg(engine,name,"CMAC",EVP_PKEY_CMAC)
        {
            setHandler(MacMethod::stub());
        }

        virtual size_t keySize() const override
        {
            return 16;
        }

        virtual size_t hashSize(bool) const override
        {
            return 16;
        }

        virtual size_t blockSize() const override
        {
            return 16;
        }

        virtual size_t prepareParams(OSSL_PARAM* params, size_t maxCount) const
        {
            std::ignore=maxCount;
            params[0]=OSSL_PARAM_construct_utf8_string(OSSL_MAC_PARAM_CIPHER,const_cast<char*>("aes-128-cbc"),0);
            return 1;
        }
};

//! @todo OpenSSL also implements KMAC, GMAC and BLAKE2

namespace detail
{
    struct OpenSslMACHandler
    {
        EVP_MAC* m_mac=nullptr;
        EVP_MAC_CTX* m_macCtx=nullptr;
    };

    struct OpenSslMACHandlerTraits
    {
        static void free(OpenSslMACHandler* handler)
        {
            if (handler->m_macCtx!=nullptr)
            {
                EVP_MAC_CTX_free(handler->m_macCtx);
            }
            if (handler->m_macCtx!=nullptr)
            {
                EVP_MAC_free(handler->m_mac);
            }
            delete handler;
        }
    };
}

//! MAC (Message Authentication Code) implementation
template <typename BaseT=MAC>
class OpenSslMACT : public BaseT
{
    public:

        explicit OpenSslMACT(const SymmetricKey* key=nullptr);

        OpenSslMACT(const OpenSslMACT& other)=delete;
        OpenSslMACT& operator= (const OpenSslMACT& other)=delete;

        OpenSslMACT(OpenSslMACT&& other) noexcept :
            m_mac(other.m_mac),
            m_macCtx(other.m_macCtx)
        {
            other.m_mac=nullptr;
            other.m_macCtx=nullptr;
        }

        inline OpenSslMACT& operator= (OpenSslMACT&& other) noexcept
        {
            if (&other!=this)
            {
                m_mac=other.m_mac;
                m_macCtx=other.m_macCtx;
                other.m_mac=nullptr;
                other.m_macCtx=nullptr;
            }
            return *this;
        }

        ~OpenSslMACT();

        static common::Error findNativeAlgorithm(
            std::shared_ptr<CryptAlgorithm> &alg,
            const char *name,
            CryptEngine* engine
        ) noexcept;

        static std::vector<std::string> listMACs();

        virtual const CryptAlgorithm* digestAlg() const noexcept override;

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
        virtual common::Error doInit() noexcept override
        {
            return prepare();
        }

        /**
         * @brief Reset digest so that it can be used again with new data
         *
         * @return Operation status
         */
        virtual void doReset() noexcept override
        {
            freeHandlers();
        }

    private:

        common::Error prepare() noexcept;

        void freeHandlers();

        EVP_MAC* m_mac;
        EVP_MAC_CTX* m_macCtx;
        size_t m_resultSize;
};

using OpenSslMAC=OpenSslMACT<MAC>;

#ifdef _WIN32
template class HATN_OPENSSL_EXPORT OpenSslMACT<MAC>;
template class HATN_OPENSSL_EXPORT OpenSslMACT<HMAC>;
#endif

//! HMAC (Hash Message Authentication Code) implementation
class HATN_OPENSSL_EXPORT OpenSslHMAC : public OpenSslMACT<HMAC>
{
    public:

        using OpenSslMACT<HMAC>::OpenSslMACT;

        static common::Error findNativeAlgorithm(
            std::shared_ptr<CryptAlgorithm> &alg,
            const char *name,
            CryptEngine* engine
        ) noexcept;
};

#endif

HATN_OPENSSL_NAMESPACE_END

#endif // HATNOPENSSLMAC_H
