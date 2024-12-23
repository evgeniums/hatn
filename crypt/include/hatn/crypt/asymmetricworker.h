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

struct AsymmetricReceiverMeta
{
    common::ByteArray encryptedKey;
};

//! Base class for asymmetric encryptors
class AEncryptor : public SymmetricWorker<true>
{
    public:

        using Base=SymmetricWorker<true>;

        AEncryptor()
        {
            setImplicitKeyMode(true);
        }

        common::Error init(
                const common::SharedPtr<PublicKey>& receiverKey,
                common::ByteArray& iv,
                common::ByteArray& encryptedKey
            )
        {
            return initT(receiverKey,iv,encryptedKey);
        }

        common::Error init(
            const common::SharedPtr<X509Certificate>& receiverCert,
            common::ByteArray& iv,
            common::ByteArray& encryptedKey
        )
        {
            return initT(receiverCert,iv,encryptedKey);
        }

        common::Error init(
            const std::vector<common::SharedPtr<PublicKey>>& receiverKeys,
            common::ByteArray& iv,
            std::vector<common::ByteArray>& encryptedKeys
        )
        {
            return initT(receiverKeys,iv,encryptedKeys);
        }

        common::Error init(
            const std::vector<common::SharedPtr<X509Certificate>>& receiverCerts,
            common::ByteArray& iv,
            std::vector<common::ByteArray>& encryptedKeys
        )
        {
            return initT(receiverCerts,iv,encryptedKeys);
        }

        template <typename ReceiverT, typename EncryptedKeyT, typename ContainerInT, typename ContainerOutT>
        common::Error runPack(
            const ReceiverT& receiverKeys,
            EncryptedKeyT& encryptedKey,
            const ContainerInT& dataIn,
            ContainerOutT& dataOut,
            size_t offsetIn=0,
            size_t sizeIn=0,
            size_t offsetOut=0
            )
        {
            if (!checkInContainerSize(dataIn.size(),offsetIn,sizeIn) || !checkOutContainerSize(dataOut.size(),offsetOut))
            {
                return common::Error(common::CommonError::INVALID_SIZE);
            }
            if (!checkEmptyOutContainerResult(sizeIn,dataOut,offsetOut))
            {
                return common::Error();
            }

            common::ByteArray iv;
            auto ec=init(receiverKeys,iv,encryptedKey);
            HATN_CHECK_EC(ec)

            dataOut.resize(offsetOut+iv.size());
            memcpy(dataOut.data()+offsetOut,iv.data(),iv.size());

            ec=processAndFinalize(dataIn,dataOut,offsetIn,sizeIn,dataOut.size());
            reset();

            return ec;
        }

    protected:

        virtual common::Error doInit(
            const common::SharedPtr<PublicKey>& receiverKey,
            common::ByteArray& iv,
            common::ByteArray& encryptedKey
            ) =0;

        virtual common::Error doInit(
            const common::SharedPtr<X509Certificate>& receiverCert,
            common::ByteArray& iv,
            common::ByteArray& encryptedKey
            ) =0;

        virtual common::Error doInit(
            const std::vector<common::SharedPtr<PublicKey>>& receiverKeys,
            common::ByteArray& iv,
            std::vector<common::ByteArray>& encryptedKeys
            ) =0;

        virtual common::Error doInit(
            const std::vector<common::SharedPtr<X509Certificate>>& receiverCerts,
            common::ByteArray& iv,
            std::vector<common::ByteArray>& encryptedKeys
            ) =0;

        virtual common::Error doInit(const char* /*initVector*/, size_t /*size*/=0) override
        {
            return CommonError::UNSUPPORTED;
        }

    private:

        template <typename ReceiverT, typename EncryptedKeyT>
        Error initT(
                const ReceiverT& receiver,
                common::ByteArray& iv,
                EncryptedKeyT& encryptedKey
            )
        {
            HATN_CHECK_RETURN(checkAlg())

            if (alg()->isNone())
            {
                m_initialized=true;
                return Error();
            }
            reset();

            auto ec=doInit(receiver,iv,encryptedKey);

            m_initialized=!ec;
            return ec;
        }
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
