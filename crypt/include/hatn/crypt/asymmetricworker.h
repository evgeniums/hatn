/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/asymmetricworker.h
 *
 *      Base classes for implementation of asymmetric encryption
 *
 */
/****************************************************************************/

#ifndef HATNCRYPTASYMMETRICWORKER_H
#define HATNCRYPTASYMMETRICWORKER_H

#include <hatn/crypt/crypt.h>
#include <hatn/crypt/symmetricworker.h>
#include <hatn/crypt/x509certificate.h>

HATN_CRYPT_NAMESPACE_BEGIN

/********************** AEncryptor **********************************/

//! Base class for asymmetric encryptors
class AEncryptor : public SymmetricWorker<true>
{
    public:

        using Base=SymmetricWorker<true>;

        AEncryptor() : m_skipReset(false)
        {
            setImplicitKeyMode(true);
        }

        common::Error init(
            const common::ConstDataBuf& iv,
            const common::SharedPtr<PublicKey>& receiverKey,
            common::ByteArray& encryptedSymmetricKey
            )
        {
            return initT(iv,receiverKey,encryptedSymmetricKey);
        }

        common::Error init(
            const common::ConstDataBuf& iv,
            const common::SharedPtr<X509Certificate>& receiverCert,
            common::ByteArray& encryptedSymmetricKey
        )
        {
            return initT(iv,receiverCert,encryptedSymmetricKey);
        }

        common::Error init(
            const common::ConstDataBuf& iv,
            const std::vector<common::SharedPtr<PublicKey>>& receiverKeys,
            std::vector<common::ByteArray>& encryptedSymmetricKey
        )
        {
            return initT(iv,receiverKeys,encryptedSymmetricKey);
        }

        common::Error init(
            const common::ConstDataBuf& iv,
            const std::vector<common::SharedPtr<X509Certificate>>& receiverCerts,
            std::vector<common::ByteArray>& encryptedSymmetricKey
        )
        {
            return initT(iv,receiverCerts,encryptedSymmetricKey);
        }

//! @todo Test AEncryptor::run before use
#if 0
        template <typename ReceicerT, typename EncryptedKeyT, typename PlaintextT, typename CiphertextT>
        common::Error run(
                const common::ConstDataBuf& iv,
                const ReceicerT& receiverKeys,
                EncryptedKeyT& encryptedSymmetricKey,
                const PlaintextT& plaintext,
                CiphertextT& ciphertext
            )
        {
            plaintext.clear();
            HATN_CHECK_RETURN(initT(iv,receiverKeys,encryptedSymmetricKey));
            auto ec=processAndFinalize(plaintext,ciphertext);
            reset();
            return ec;
        }

        //! Overloaded runPack
        template <typename ReceicerT, typename EncryptedKeyT, typename ContainerOutT>
        common::Error runPack(
            const ReceicerT& receiverKeys,
            EncryptedKeyT& encryptedSymmetricKey,
            const common::SpanBuffers& dataIn,
            ContainerOutT& dataOut,
            size_t offsetOut=0
        )
        {
            prepare(receiverKeys,encryptedSymmetricKey);
            m_skipReset=true;
            auto ec=Base::runPack(dataIn,dataOut,offsetOut);
            m_skipReset=false;
            reset();
            return ec;
        }
#endif

        template <typename ReceicerT, typename EncryptedKeyT, typename ContainerInT, typename ContainerOutT>
        common::Error runPack(
            const ReceicerT& receiverKeys,
            EncryptedKeyT& encryptedSymmetricKey,
            const ContainerInT& dataIn,
            ContainerOutT& dataOut,
            size_t offsetIn=0,
            size_t sizeIn=0,
            size_t offsetOut=0
            )
        {
            prepare(receiverKeys,encryptedSymmetricKey);
            m_skipReset=true;
            auto ec=Base::runPack(dataIn,dataOut,offsetIn,sizeIn,offsetOut);
            m_skipReset=false;
            reset();
            return ec;
        }

    protected:

        virtual common::Error doInit(
            const common::ConstDataBuf& iv,
            const common::SharedPtr<PublicKey>& receiverKey,
            common::ByteArray& encryptedSymmetricKey
        ) =0;

        virtual common::Error doInit(
            const common::ConstDataBuf& iv,
            const common::SharedPtr<X509Certificate>& receiverCert,
            common::ByteArray& encryptedSymmetricKey
        ) =0;

        virtual common::Error doInit(
            const common::ConstDataBuf& iv,
            const std::vector<common::SharedPtr<PublicKey>>& receiverKeys,
            std::vector<common::ByteArray>& encryptedSymmetricKey
        ) =0;

        virtual common::Error doInit(
            const common::ConstDataBuf& iv,
            const std::vector<common::SharedPtr<X509Certificate>>& receiverCerts,
            std::vector<common::ByteArray>& encryptedSymmetricKey
        ) =0;

        virtual void backendReset() =0;

        virtual common::Error doInit(const char* initVector, size_t size=0) override
        {
            auto sz=size;
            if (sz==0)
            {
                sz=ivSize();
            }
            common::ConstDataBuf iv{initVector,sz};
            Assert(m_forwardInitFn,"Invalid initialization sequence");
            return m_forwardInitFn(iv);
        }

        void doReset() noexcept override
        {
            if (!m_skipReset)
            {
                m_forwardInitFn=std::function<Error (const common::ConstDataBuf& iv)>{};
            }
            backendReset();
        }

    private:

        std::function<Error (const common::ConstDataBuf& iv)> m_forwardInitFn;

        template <typename ReceicerT, typename EncryptedKeyT>
        void prepare(
            const ReceicerT& receiver,
            EncryptedKeyT& encryptedSymmetricKey
            )
        {
            m_forwardInitFn=[this,&receiver,&encryptedSymmetricKey](const common::ConstDataBuf& iv)
            {
                return doInit(iv,receiver,encryptedSymmetricKey);
            };
        }

        template <typename ReceicerT, typename EncryptedKeyT, typename ContainerIvT>
        Error initT(
            const ContainerIvT& iv,
            const ReceicerT& receiver,
            EncryptedKeyT& encryptedSymmetricKey
            )
        {
            m_skipReset=true;
            prepare(receiver,encryptedSymmetricKey);
            auto ec=Base::init(iv);
            m_skipReset=false;
            return ec;
        }

        bool m_skipReset;
};

//! Base class for asymmetric decryptors
class ADecryptor : public SymmetricWorker<false>
{
    public:

        using Base=SymmetricWorker<false>;

        ADecryptor(const PrivateKey* key) : m_key(key), m_skipReset(false)
        {
            setImplicitKeyMode(true);
        }

        ADecryptor():ADecryptor(nullptr)
        {}

        /**
         * @brief Set private key
         * @param key Key
         *
         * @throws ErrorException if the key can not be set
         */
        void setKey(const PrivateKey* key)
        {
            if (key!=nullptr)
            {
                Assert(key->hasRole(SecureKey::Role::ENCRYPT_ASYMMETRIC),"Key must have role allowing for asymmetric decryption");
            }
            m_key=key;
        }

        //! Get private key
        inline const PrivateKey* key() const noexcept
        {
            return m_key;
        }

        common::Error init(
                const common::ConstDataBuf& iv,
                const common::ConstDataBuf& encryptedSymmetricKey
            )
        {
            m_skipReset=true;
            m_encryptedSymmetricKey.set(encryptedSymmetricKey);
            auto ec=Base::init(iv);
            m_skipReset=false;
            return ec;
        }

//! @todo Test ADecryptor::run before use
#if 0
        template <typename EncryptedKeyT, typename PlaintextT, typename CiphertextT>
        common::Error run(
                const common::ConstDataBuf& iv,
                const EncryptedKeyT& encryptedSymmetricKey,
                const CiphertextT& ciphertext,
                PlaintextT& plaintext
            )
        {
            plaintext.clear();
            HATN_CHECK_RETURN(init(iv,encryptedSymmetricKey));
            auto ec=processAndFinalize(ciphertext,plaintext);
            reset();
            return ec;
        }

        //! Overloaded runPack
        template <typename EncryptedKeyT, typename ContainerOutT>
        common::Error runPack(
            const EncryptedKeyT& encryptedSymmetricKey,
            const common::SpanBuffers& dataIn,
            ContainerOutT& dataOut,
            size_t offsetOut=0
            )
        {
            m_skipReset=true;
            m_encryptedSymmetricKey.set(encryptedSymmetricKey);
            auto ec=Base::runPack(dataIn,dataOut,offsetOut);
            m_skipReset=false;
            reset();
            return ec;
        }

#endif
        template <typename EncryptedKeyT, typename ContainerInT, typename ContainerOutT>
        common::Error runPack(
            const EncryptedKeyT& encryptedSymmetricKey,
            const ContainerInT& dataIn,
            ContainerOutT& dataOut,
            size_t offsetIn=0,
            size_t sizeIn=0,
            size_t offsetOut=0
            )
        {
            m_skipReset=true;
            m_encryptedSymmetricKey.set(encryptedSymmetricKey);
            auto ec=Base::runPack(dataIn,dataOut,offsetIn,sizeIn,offsetOut);
            m_skipReset=false;
            reset();
            return ec;
        }

    protected:

        virtual common::Error doInit(
            const common::ConstDataBuf& iv,
            const common::ConstDataBuf& encryptedSymmetricKey
        ) =0;

        virtual void backendReset() =0;

        virtual common::Error doInit(const char* initVector, size_t size=0) override
        {
            auto sz=size;
            if (sz==0)
            {
                sz=ivSize();
            }
            common::ConstDataBuf iv{initVector,sz};
            Assert(!m_encryptedSymmetricKey.empty(),"Invalid initialization sequence");
            return doInit(iv,m_encryptedSymmetricKey);
        }

        void doReset() noexcept override
        {
            if (!m_skipReset)
            {
                m_encryptedSymmetricKey.reset();
            }
            backendReset();
        }

        virtual common::Error doGenerateIV(char*, size_t*) const override
        {
            return CommonError::UNSUPPORTED;
        }

    private:

        common::ConstDataBuf m_encryptedSymmetricKey;
        const PrivateKey* m_key;
        bool m_skipReset;
};

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTASYMMETRICWORKER_H
