/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file crypt/kdf.h
 *
 *      Base class for Key Derivation Functions (KDF)
 *
 */
/****************************************************************************/

#ifndef HATNCRYPTKDF_H
#define HATNCRYPTKDF_H

#include <hatn/crypt/crypt.h>
#include <hatn/crypt/securekey.h>

HATN_CRYPT_NAMESPACE_BEGIN

//! Base class for Key Derivation Functions
class KDF
{
    public:

        /**
         * @brief Ctor
         * @param targetKeyAlg Algorithm the derived keys will be used for
         * @param kdfAlg Key derivation algorithm
         */
        KDF(const CryptAlgorithm* targetKeyAlg=nullptr, const CryptAlgorithm* kdfAlg=nullptr) noexcept
            : m_targetKeyAlg(targetKeyAlg),
              m_kdfAlg(kdfAlg)
        {}

        virtual ~KDF()=default;
        KDF(const KDF&)=delete;
        KDF(KDF&&) =default;
        KDF& operator=(const KDF&)=delete;
        KDF& operator=(KDF&&) =default;

        inline void setTargetKeyAlg(const CryptAlgorithm* alg) noexcept
        {
            m_targetKeyAlg.ptr=alg;
        }
        inline const CryptAlgorithm* targetKeyAlg() const noexcept
        {
            return m_targetKeyAlg.ptr;
        }

        inline void setKdfAlg(const CryptAlgorithm* kdfAlg) noexcept
        {
            m_kdfAlg.ptr=kdfAlg;
        }
        inline const CryptAlgorithm* kdfAlg() const noexcept
        {
            return m_kdfAlg.ptr;
        }

    protected:

        common::Error resetKey(common::SharedPtr<SymmetricKey> &key, size_t& keySize)
        {
            keySize=0;
            if (!key)
            {
                if (targetKeyAlg())
                {
                    if (targetKeyAlg()->type()==CryptAlgorithm::Type::MAC
                        ||
                        targetKeyAlg()->type()==CryptAlgorithm::Type::HMAC
                       )
                    {
                        key=targetKeyAlg()->createMACKey();
                    }
                    else
                    {
                        key=targetKeyAlg()->createSymmetricKey();
                    }
                }
                keySize=targetKeyAlg()->keySize();
            }
            else if (key->alg())
            {
                if (key->alg()!=targetKeyAlg())
                {
                    return cryptError(CryptError::INVALID_ALGORITHM);
                }
                keySize=key->alg()->keySize();
                key->content().clear();
            }
            else
            {
                key->setAlg(targetKeyAlg());
            }
            if (!key)
            {
                return cryptError(CryptError::KEY_INITIALIZATION_FAILED);
            }

            if (keySize==0)
            {
                return cryptError(CryptError::INVALID_KEY_LENGTH);
            }

            return common::Error();
        }


    private:

        common::ConstPointerWithInit<CryptAlgorithm> m_targetKeyAlg;
        common::ConstPointerWithInit<CryptAlgorithm> m_kdfAlg;
};

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTKDF_H

