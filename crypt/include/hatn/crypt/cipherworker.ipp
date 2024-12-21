/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file crypt/cipherworker.ipp
 *
 *      Base classes for implementation of symmetric encryption
 *
 */
/****************************************************************************/

#ifndef HATNCRYPTCIPHERWORKER_IPP
#define HATNCRYPTCIPHERWORKER_IPP

#include <hatn/crypt/cipherworker.h>

HATN_CRYPT_NAMESPACE_BEGIN

namespace detail
{

/********************** CipherTraits **********************************/

//! Symmetric cipher traits
template <bool Encrypt>
struct CipherTraits
{
};

//! Symmetric encryptor traits
template <>
struct CipherTraits<true>
{
    //! Encrypt data and pack IV with ciphertext
    template <typename Cipher,typename ContainerInT,typename ContainerOutT>
    static common::Error runPack(
        Cipher* cipher,
        const ContainerInT& dataIn,
        ContainerOutT& dataOut,
        size_t offsetIn=0,
        size_t sizeIn=0,
        size_t offsetOut=0
    );

    //! Encrypt data from scattered buffers and pack IV with ciphertext
    template <typename Cipher,typename ContainerOutT>
    static common::Error runPack(
        Cipher* cipher,
        const common::SpanBuffers& dataIn,
        ContainerOutT& dataOut,
        size_t offsetOut=0
    );
};

//! Symmetric decryptor traits
template <>
struct CipherTraits<false>
{
    //! Decrypt data in single buffer taking IV from beggining of ciphered input
    template <typename Cipher,typename ContainerInT,typename ContainerOutT>
    static common::Error runPack(
        Cipher* cipher,
        const ContainerInT& dataIn,
        ContainerOutT& dataOut,
        size_t offsetIn=0,
        size_t sizeIn=0,
        size_t offsetOut=0
    );

    //! Decrypt data from scattered buffers taking IV from beggining of the first inpt buffer
    template <typename Cipher,typename ContainerOutT>
    static common::Error runPack(
        Cipher* cipher,
        const common::SpanBuffers& dataIn,
        ContainerOutT& dataOut,
        size_t offsetOut=0
    );
};

// ------------------- CipherTraits<true> -------------------

//---------------------------------------------------------------
template <typename Cipher,typename ContainerInT,typename ContainerOutT>
common::Error CipherTraits<true>::runPack(
    Cipher* cipher,
    const ContainerInT& dataIn,
    ContainerOutT& dataOut,
    size_t offsetIn,
    size_t sizeIn,
    size_t offsetOut
)
{
    HATN_CHECK_RETURN(cipher->generateIV(dataOut,offsetOut))
    common::ConstDataBuf iv(dataOut.data()+offsetOut,dataOut.size()-offsetOut);
    HATN_CHECK_RETURN(cipher->init(iv));
    return cipher->processAndFinalize(dataIn,dataOut,offsetIn,sizeIn,dataOut.size());
}

//---------------------------------------------------------------
template <typename Cipher,typename ContainerOutT>
common::Error CipherTraits<true>::runPack(
    Cipher* cipher,
    const common::SpanBuffers& dataIn,
    ContainerOutT& dataOut,
    size_t offsetOut
)
{
    try
    {
        size_t ivSize=cipher->ivSize();
        dataOut.resize(offsetOut+ivSize);

        HATN_CHECK_RETURN(cipher->generateIV(dataOut,offsetOut))
        common::ConstDataBuf iv(dataOut.data()+offsetOut,ivSize);
        HATN_CHECK_RETURN(cipher->init(iv));

        for (const auto& it:dataIn)
        {
            auto view=it.view();

            size_t size=0;
            HATN_CHECK_RETURN(cipher->process(view,dataOut,size,0,0,dataOut.size()))
        }

        size_t finalSize=0;
        return finalize(dataOut,finalSize,dataOut.size());
    }
    catch (const common::ErrorException& e)
    {
        return e.error();
    }
    return common::Error();
}

// ------------------- CipherTraits<false> -------------------

//---------------------------------------------------------------
template <typename Cipher,typename ContainerInT,typename ContainerOutT>
common::Error CipherTraits<false>::runPack(
    Cipher* cipher,
    const ContainerInT& dataIn,
    ContainerOutT& dataOut,
    size_t offsetIn,
    size_t sizeIn,
    size_t offsetOut
)
{
    size_t ivLength=cipher->ivSize();
    if ((ivLength+offsetIn)>sizeIn)
    {
        return cryptError(CryptError::DECRYPTION_FAILED);
    }

    common::ConstDataBuf iv(dataIn.data()+offsetIn,ivLength);
    HATN_CHECK_RETURN(cipher->init(iv));
    return cipher->processAndFinalize(dataIn,dataOut,offsetIn+ivLength,sizeIn-ivLength,offsetOut);
}

//---------------------------------------------------------------
template <typename Cipher,typename ContainerOutT>
common::Error CipherTraits<false>::runPack(
    Cipher* cipher,
    const common::SpanBuffers& dataIn,
    ContainerOutT& dataOut,
    size_t offsetOut
)
{
    if (dataIn.empty())
    {
        (offsetOut==0)?dataOut.clear():dataOut.resize(offsetOut);
        return common::Error();
    }

    size_t i=0;
    for (const auto& it:dataIn)
    {
        auto view=it.view();

        if (i++==0)
        {
            size_t ivLength=cipher->ivSize();
            if (ivLength>view.size())
            {
                return cryptError(CryptError::DECRYPTION_FAILED);
            }
            common::ConstDataBuf iv(view.data(),ivLength);
            HATN_CHECK_RETURN(cipher->init(iv));

            size_t size=0;
            HATN_CHECK_RETURN(cipher->process(view,dataOut,size,ivLength,view.size()-ivLength,offsetOut))
        }
        else
        {
            size_t size=0;
            HATN_CHECK_RETURN(cipher->process(view,dataOut,size,0,0,dataOut.size()))
        }
    }

    size_t finalSize=0;
    return cipher->finalize(dataOut,finalSize,dataOut.size());
}

//---------------------------------------------------------------
} // namespace detail

/********************** CipherWorker **********************************/

//---------------------------------------------------------------
template <bool Encrypt>
CipherWorker<Encrypt>::CipherWorker(const SymmetricKey* key):m_key(key),m_initialized(false),m_alg(nullptr)
{
    setKey(key,false);
}

//---------------------------------------------------------------
template <bool Encrypt>
void CipherWorker<Encrypt>::setKey(const SymmetricKey* key, bool updateInherited)
{
    if (key!=nullptr)
    {
        Assert(key->hasRole(SecureKey::Role::ENCRYPT_SYMMETRIC),"Key must have role allowing for symmetric encryption");
        if (m_alg!=nullptr)
        {
            Assert(key->alg()==m_alg,"Invalid crypt algorithm");
        }
        else
        {
            setAlg(key->alg());
        }
    }
    m_key=key;
    if (updateInherited)
    {
        doUpdateKey();
    }
}

//---------------------------------------------------------------
template <bool Encrypt>
template <typename ContainerIvT>
common::Error CipherWorker<Encrypt>::init(const ContainerIvT& iv)
{
    if (m_alg==nullptr)
    {
        return cryptError(CryptError::INVALID_ALGORITHM);
    }
    if (m_alg->isNone())
    {
        m_initialized=true;
        return common::Error();
    }

    Assert(key()!=nullptr,"Key is not set");
    if (!key()->isAlgDefined()
            ||
        !(key()->alg()->isType(CryptAlgorithm::Type::SENCRYPTION)
         ||
         key()->alg()->isType(CryptAlgorithm::Type::AEAD)
         )
        )
    {
        return cryptError(CryptError::INVALID_KEY_TYPE);
    }

    reset();

    size_t ivS=ivSize();
    if ((iv.size())>=ivS)
    {
        auto ec=doInit(iv.data());
        m_initialized=!ec;
        return ec;
    }

    // pad with zeros on the left
    common::ByteArray padIV;
    padIV.resize(ivS);
    padIV.fill(0);
    if (iv.size()!=0)
    {
        std::copy(iv.data(),iv.data()+iv.size(),padIV.data()+padIV.size()-iv.size());
    }
    auto ec=doInit(padIV.data());
    m_initialized=!ec;
    return ec;
}

//---------------------------------------------------------------
template <bool Encrypt>
common::Error CipherWorker<Encrypt>::init(const common::SpanBuffer &iv)
{
    try
    {
        return init(iv.view());
    }
    catch (const common::ErrorException& e)
    {
        return e.error();
    }
    return common::Error();
}

//---------------------------------------------------------------
template <bool Encrypt>
size_t CipherWorker<Encrypt>::maxPadding() const noexcept
{
    size_t padding=blockSize();
    if (padding>1)
    {
        return padding;
    }
    return 0;
}

//---------------------------------------------------------------
template <bool Encrypt>
void CipherWorker<Encrypt>::reset() noexcept
{
    m_initialized=false;
    doReset();
}

//---------------------------------------------------------------
template <bool Encrypt>
template <typename ContainerInT, typename ContainerOutT>
common::Error CipherWorker<Encrypt>::process(
    const ContainerInT& dataIn,
    ContainerOutT& dataOut,
    size_t& sizeOut,
    size_t sizeIn,
    size_t offsetIn,
    size_t offsetOut,
    bool lastBlock,
    bool noResize
)
{
    if (!m_initialized)
    {
        return cryptError(CryptError::INVALID_CIPHER_STATE);
    }

    if (!checkInContainerSize(dataIn.size(),offsetIn,sizeIn) || !checkOutContainerSize(dataOut.size(),offsetOut))
    {
        return common::Error(common::CommonError::INVALID_SIZE);
    }

    auto bufIn=dataIn.data()+offsetIn;
    if (sizeIn==0)
    {
        if (lastBlock)
        {
            bufIn=nullptr;
        }
        else
        {
            sizeOut=offsetOut;
            return common::Error();
        }
    }

    if (m_alg->isNone())
    {
        sizeOut=sizeIn+offsetOut;
        if (dataOut.size()<sizeOut)
        {
            dataOut.resize(sizeOut);
        }
        std::copy(bufIn,bufIn+sizeIn,dataOut.data()+offsetOut);
        return common::Error();
    }

    if (!noResize)
    {
        size_t prepareSize=sizeIn+maxPadding();
        if (dataOut.size()<(prepareSize+offsetOut))
        {
            dataOut.resize(prepareSize+offsetOut);
        }
    }

    HATN_CHECK_RETURN(doProcess(bufIn,sizeIn,dataOut.data()+offsetOut,sizeOut,lastBlock))
    sizeOut+=offsetOut;
    if (!noResize)
    {
        dataOut.resize(sizeOut);
    }
    return common::Error();
}

//---------------------------------------------------------------
template <bool Encrypt>
template <typename ContainerOutT>
common::Error CipherWorker<Encrypt>::finalize(
    ContainerOutT& dataOut,
    size_t& sizeOut,
    size_t offsetOut
)
{
    if (!m_initialized)
    {
        return cryptError(CryptError::INVALID_CIPHER_STATE);
    }
    m_initialized=false;

    if (m_alg->isNone())
    {
        sizeOut=offsetOut;
        dataOut.resize(sizeOut);
        return common::Error();
    }

    size_t prepareSize=offsetOut;
    size_t possiblePadding=blockSize();
    if (possiblePadding>1)
    {
        prepareSize+=possiblePadding;
    }
    if (dataOut.size()<prepareSize)
    {
        dataOut.resize(prepareSize);
    }

    HATN_CHECK_RETURN(doProcess(nullptr,0,dataOut.data()+offsetOut,sizeOut,true))
    sizeOut+=offsetOut;
    dataOut.resize(sizeOut);

    return common::Error();
}

//---------------------------------------------------------------
template <bool Encrypt>
template <typename ContainerInT, typename ContainerOutT>
common::Error CipherWorker<Encrypt>::processAndFinalize(
    const ContainerInT& dataIn,
    ContainerOutT& dataOut,
    size_t offsetIn,
    size_t sizeIn,
    size_t offsetOut
)
{
    if (key()->alg()->isType(CryptAlgorithm::Type::AEAD))
    {
        return cryptError(CryptError::INVALID_OPERATION);
    }

    if (!checkInContainerSize(dataIn.size(),offsetIn,sizeIn) || !checkOutContainerSize(dataOut.size(),offsetOut))
    {
        return common::Error(common::CommonError::INVALID_SIZE);
    }
    if (!checkEmptyOutContainerResult(sizeIn,dataOut,offsetOut))
    {
        return common::Error();
    }

    size_t size=0;
    HATN_CHECK_RETURN(process(dataIn,dataOut,size,sizeIn,offsetIn,offsetOut))

    size_t finalSize=0;
    return finalize(dataOut,finalSize,size);
}

//---------------------------------------------------------------
template <bool Encrypt>
template <typename ContainerOutT>
common::Error CipherWorker<Encrypt>::processAndFinalize(
    const common::SpanBuffer& dataIn,
    ContainerOutT& dataOut,
    size_t offsetOut
)
{
    try
    {
        auto view=dataIn.view();
        return processAndFinalize(view,dataOut,0,0,offsetOut);
    }
    catch (const common::ErrorException& e)
    {
        return e.error();
    }
    return common::Error();
}

//---------------------------------------------------------------
template <bool Encrypt>
template <typename ContainerOutT>
common::Error CipherWorker<Encrypt>::processAndFinalize(
    const common::SpanBuffers& dataIn,
    ContainerOutT& dataOut,
    size_t offsetOut
)
{
    try
    {
        if (offsetOut>0)
        {
            dataOut.resize(offsetOut);
        }
        for (const auto& it:dataIn)
        {
            auto view=it.view();

            size_t size=0;
            HATN_CHECK_RETURN(process(view,dataOut,size,0,0,dataOut.size()))
        }
        size_t finalSize=0;
        return finalize(dataOut,finalSize,dataOut.size());
    }
    catch (const common::ErrorException& e)
    {
        return e.error();
    }
    return common::Error();
}

//---------------------------------------------------------------
template <bool Encrypt>
template <typename ContainerInT,typename ContainerOutT>
common::Error CipherWorker<Encrypt>::runPack(
    const ContainerInT& dataIn,
    ContainerOutT& dataOut,
    size_t offsetIn,
    size_t sizeIn,
    size_t offsetOut
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

    return detail::CipherTraits<Encrypt>::runPack(this,dataIn,dataOut,offsetIn,sizeIn,offsetOut);
}

//---------------------------------------------------------------
template <bool Encrypt>
template <typename ContainerOutT>
common::Error CipherWorker<Encrypt>::runPack(
    const common::SpanBuffers& dataIn,
    ContainerOutT& dataOut,
    size_t offsetOut
)
{
    if (!checkEmptyOutContainerResult(dataIn.size(),dataOut,offsetOut))
    {
        return common::Error();
    }
    return detail::CipherTraits<Encrypt>::runPack(this,dataIn,dataOut,offsetOut);
}

//---------------------------------------------------------------
template <bool Encrypt>
template <typename ContainerT>
common::Error CipherWorker<Encrypt>::generateIV(ContainerT& iv, size_t offset) const
{
    auto ivsz=ivSize();
    if (ivsz==0)
    {
        return common::Error();
    }
    if (iv.size()<(ivsz+offset))
    {
        iv.resize(ivsz+offset);
    }
    return doGenerateIV(iv.data()+offset);
}

//---------------------------------------------------------------
template <typename ContainerT>
common::Error SEncryptor::initStream(
        ContainerT& result,
        size_t offset
    )
{
    HATN_CHECK_RETURN(generateIV(result,offset))
    common::ConstDataBuf iv(result.data()+offset,result.size()-offset);
    return init(iv);
}

//---------------------------------------------------------------
template <typename BufferT, typename ContainerT>
common::Error SEncryptor::encryptStream(
        const BufferT& plaintext,
        ContainerT& ciphertext,
        size_t offset
    )
{
    size_t sizeOut=0;
    HATN_CHECK_RETURN(process(plaintext,ciphertext,sizeOut,0,0,offset))
    if (sizeOut!=(plaintext.size()+offset))
    {
        return cryptError(CryptError::UNEXPECTED_STREAM_CIPHER_PADDING);
    }
    return OK;
}

//---------------------------------------------------------------
template <typename ContainerT>
Result<size_t> SDecryptor::initStream(
        const ContainerT& ciphertext,
        size_t offset
    )
{
    size_t ivLength=ivSize();
    if (ivLength>(ciphertext.size()-offset))
    {
        return cryptError(CryptError::DECRYPTION_FAILED);
    }

    common::ConstDataBuf iv(ciphertext.data()+offset,ivLength);
    HATN_CHECK_RETURN(init(iv))

    return offset+ivLength;
}

//---------------------------------------------------------------
template <typename BufferT, typename ContainerT>
common::Error SDecryptor::decryptStream(
        const BufferT& ciphertext,
        ContainerT& plaintext,
        size_t offset
    )
{
    size_t sizeOut=0;
    HATN_CHECK_RETURN(process(ciphertext,plaintext,sizeOut,0,0,offset))
    if (sizeOut!=plaintext.size())
    {
        return cryptError(CryptError::UNEXPECTED_STREAM_CIPHER_PADDING);
    }
    return OK;
}

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTCIPHERWORKER_IPP
