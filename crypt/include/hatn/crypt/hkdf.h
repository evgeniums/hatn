/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file crypt/hkdf.h
 *
 *      Base class for Hash Key Derivation Functions (HKDF)
 *
 */
/****************************************************************************/

#ifndef HATNCRYPTHKDF_H
#define HATNCRYPTHKDF_H

#include <hatn/crypt/crypt.h>
#include <hatn/crypt/securekey.h>
#include <hatn/crypt/kdf.h>

HATN_CRYPT_NAMESPACE_BEGIN

//! Base class for Hash Key Derivation Functions
class HKDF : public KDF
{
    public:

        enum class Mode : int
        {
            Extract_Expand_All,
            First_Extract_Then_Expand,
            Expand_All,
            Extract_All
        };

        /**
         * @brief Ctor
         * @param targetKeyAlg Algorithm the derived keys will be used for
         * @param hashAlg Digest algorithm for hash calculating
         */
        HKDF(const CryptAlgorithm* targetKeyAlg=nullptr, const CryptAlgorithm* hashAlg=nullptr) noexcept
            : KDF(targetKeyAlg,hashAlg),
              m_mode(Mode::First_Extract_Then_Expand),
              m_initialized(false)
        {}

        template <typename SaltT>
        common::Error init(
            const SecureKey* masterKey,
            const SaltT& salt
        )
        {
            Assert(targetKeyAlg()!=nullptr&&kdfAlg()!=nullptr,"Target and hash algorithms must be set before using HKDF");
            if (!masterKey->canBeUsedForHkdf())
            {
                return cryptError(CryptError::KEY_NOT_VALID_FOR_HKDF);
            }
            if (m_initialized)
            {
                reset();
            }
            auto ec=doInit(masterKey,salt.data(),salt.size());
            m_initialized=!ec;
            return ec;
        }

        void reset() noexcept
        {
            m_initialized=false;
            doReset();
        }

        template <typename InfoT>
        common::Error derive(
            common::SharedPtr<SymmetricKey>& derivedKey,
            const InfoT& info
        )
        {
            if (!m_initialized)
            {
                return cryptError(CryptError::INVALID_MAC_STATE);
            }
            return doDerive(derivedKey,info.data(),info.size());
        }

        template <typename InfoT, typename SaltT>
        common::Error deriveAndReplace(
            common::SharedPtr<SymmetricKey>& key,
            const InfoT& info,
            const SaltT& salt
        )
        {
            Assert(!key->hasRole(SecureKey::Role::PASSPHRASE),"Can not derive and replace passphrase key in HKDF");

            HATN_CHECK_RETURN(derive(key,info))

            bool overwriteMode=mode()!=Mode::Extract_Expand_All && mode()!=Mode::Extract_All;
            auto keepMode=mode();
            if (overwriteMode)
            {
                setMode(Mode::First_Extract_Then_Expand);
            }
            auto ec=init(key.get(),salt);
            if (overwriteMode)
            {
                setMode(keepMode);
            }
            return ec;
        }

        inline void setHashAlg(const CryptAlgorithm* hashAlg) noexcept
        {
            setKdfAlg(hashAlg);
        }
        inline const CryptAlgorithm* hashAlg() const noexcept
        {
            return kdfAlg();
        }

        inline void setMode(Mode mode) noexcept
        {
            m_mode=mode;
        }
        inline Mode mode() const noexcept
        {
            return m_mode;
        }

    protected:

        virtual void doReset() noexcept =0;

        virtual common::Error doInit(
            const SecureKey* masterKey,
            const char* saltData,
            size_t saltSize
        ) =0;

        virtual common::Error doDerive(
            common::SharedPtr<SymmetricKey>& derivedKey,
            const char* infoData,
            size_t infoSize
        ) =0;

    private:

        Mode m_mode;
        bool m_initialized;
};

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTHKDF_H
