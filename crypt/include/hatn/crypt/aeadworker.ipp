/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file crypt/aeadworker.ipp
 *
 *      Base classes for implementation of AEAD ciphers
 *
 */
/****************************************************************************/

#ifndef HATNCRYPTAEADWORKER_IPP
#define HATNCRYPTAEADWORKER_IPP

#include <hatn/crypt/aeadworker.h>
#include <hatn/crypt/cipherworker.ipp>

HATN_CRYPT_NAMESPACE_BEGIN

namespace detail {

/********************** AeadArgTraits **********************************/

//! Traits for operations in AEAD mode for SpanBuffer(s)
struct AeadArgTraits
{
    //! Process unencrypted data for authentication
    template <typename Cipher,typename ContainerOutT>
    static common::Error auth(
        Cipher* enc,
        const common::SpanBuffers& authdata,
        ContainerOutT& ciphertext
    );

    //! Encrypt data
    template <typename Cipher,typename ContainerOutT>
    static common::Error encrypt(
        Cipher* enc,
        const common::SpanBuffers& plaintext,
        ContainerOutT& ciphertext
    );

    //! Decrypt data
    template <typename Cipher,typename ContainerOutT>
    static common::Error decrypt(
        Cipher* dec,
        const common::SpanBuffers& ciphertext,
        ContainerOutT& plaintext,
        size_t offset
    );

    //! Extract IV and tag
    template <typename Cipher>
    static std::pair<common::ConstDataBuf,common::ConstDataBuf> extractIvAndTag(
        Cipher* dec,
        const common::SpanBuffers& ciphertext,
        common::Error& ec
    );

    //! Process unencrypted data for authentication
    template <typename Cipher,typename ContainerOutT>
    static common::Error auth(
            Cipher* enc,
            const common::SpanBuffer& authdata,
            ContainerOutT& ciphertext
        );

    //! Encrypt data
    template <typename Cipher,typename ContainerOutT>
    static common::Error encrypt(
            Cipher* enc,
            const common::SpanBuffer& plaintext,
            ContainerOutT& ciphertext
        );

    //! Decrypt data
    template <typename Cipher,typename ContainerOutT>
    static common::Error decrypt(
        Cipher* dec,
        const common::SpanBuffer& ciphertext,
        ContainerOutT& plaintext,
        size_t offset
    );

    //! Extract IV and tag
    template <typename Cipher>
    static std::pair<common::ConstDataBuf,common::ConstDataBuf> extractIvAndTag(
        Cipher* dec,
        const common::SpanBuffer& ciphertext,
        common::Error& ec
    );
};

//----------------------------------------------------------------
template <typename Cipher,typename ContainerOutT>
common::Error AeadArgTraits::auth(
        Cipher* enc,
        const common::SpanBuffers& authdata,
        ContainerOutT& ciphertext
    )
{
    try
    {
        if (!authdata.empty())
        {
            enc->beginNotEncrypted();
            for (const auto& it:authdata)
            {
                size_t stubSize=0;
                HATN_CHECK_RETURN(enc->process(it.view(),ciphertext,stubSize,0,0,ciphertext.size()))
            }
            enc->endNotEncrypted();
        }
    }
    catch (const common::ErrorException& e)
    {
        return e.error();
    }
    return common::Error();
}

//---------------------------------------------------------------
template <typename Cipher,typename ContainerOutT>
common::Error AeadArgTraits::encrypt(
        Cipher* enc,
        const common::SpanBuffers& plaintext,
        ContainerOutT& ciphertext
    )
{
    try
    {
        if (!plaintext.empty())
        {
            for (const auto& it:plaintext)
            {
                size_t stubSize=0;
                HATN_CHECK_RETURN(enc->process(it.view(),ciphertext,stubSize,0,0,ciphertext.size()))
            }
        }
    }
    catch (const common::ErrorException& e)
    {
        return e.error();
    }
    return common::Error();
}

//---------------------------------------------------------------
template <typename Cipher,typename ContainerOutT>
common::Error AeadArgTraits::decrypt(
        Cipher* enc,
        const common::SpanBuffers& ciphertext,
        ContainerOutT& plaintext,
        size_t offset
    )
{
    try
    {
        if (!ciphertext.empty())
        {
            size_t i=0;
            for (const auto& it:ciphertext)
            {
                auto view=it.view();
                size_t stubSize;

                if (i++==0)
                {
                    // IV and tag must in the first buffer

                    if (view.size()==offset)
                    {
                        continue;
                    }
                    HATN_CHECK_RETURN(enc->process(view,plaintext,stubSize,0,offset,plaintext.size()))
                }
                else
                {
                    HATN_CHECK_RETURN(enc->process(view,plaintext,stubSize,0,0,plaintext.size()))
                }
            }
        }
    }
    catch (const common::ErrorException& e)
    {
        return e.error();
    }
    return common::Error();
}

//---------------------------------------------------------------
template <typename Cipher>
std::pair<common::ConstDataBuf,common::ConstDataBuf>
    AeadArgTraits::extractIvAndTag(
        Cipher* dec,
        const common::SpanBuffers& ciphertext,
        common::Error& ec
    )
{
    return AeadArgTraits::extractIvAndTag(dec,ciphertext.front(),ec);
}

//---------------------------------------------------------------
template <typename Cipher,typename ContainerOutT>
common::Error AeadArgTraits::auth(
            Cipher* enc,
            const common::SpanBuffer& authdata,
            ContainerOutT& ciphertext
        )
{
    try
    {
        if (!authdata.isEmpty())
        {
            enc->beginNotEncrypted();
            size_t stubSize=0;
            HATN_CHECK_RETURN(enc->process(authdata.view(),ciphertext,stubSize,0,0,ciphertext.size()))
            enc->endNotEncrypted();
        }
    }
    catch (const common::ErrorException& e)
    {
        return e.error();
    }
    return common::Error();
}

//---------------------------------------------------------------
template <typename Cipher,typename ContainerOutT>
common::Error AeadArgTraits::encrypt(
        Cipher* enc,
        const common::SpanBuffer& plaintext,
        ContainerOutT& ciphertext
    )
{
    try
    {
        if (!plaintext.isEmpty())
        {
            size_t stubSize=0;
            HATN_CHECK_RETURN(enc->process(plaintext.view(),ciphertext,stubSize,0,0,ciphertext.size()))
        }
    }
    catch (const common::ErrorException& e)
    {
        return e.error();
    }
    return common::Error();
}

//---------------------------------------------------------------
template <typename Cipher,typename ContainerOutT>
common::Error AeadArgTraits::decrypt(
        Cipher* enc,
        const common::SpanBuffer& ciphertext,
        ContainerOutT& plaintext,
        size_t offset
    )
{
    try
    {
        if (!ciphertext.isEmpty())
        {
            size_t stubSize=0;
            HATN_CHECK_RETURN(enc->process(ciphertext.view(),plaintext,stubSize,0,offset,plaintext.size()))
        }
    }
    catch (const common::ErrorException& e)
    {
        return e.error();
    }
    return common::Error();
}

//---------------------------------------------------------------
template <typename Cipher>
std::pair<common::ConstDataBuf,common::ConstDataBuf>
    AeadArgTraits::extractIvAndTag(
        Cipher* dec,
        const common::SpanBuffer& ciphertext,
        common::Error& ec
    )
{
    try
    {
        auto ivSize=dec->ivSize();
        auto tagSize=dec->getTagSize();
        auto view=ciphertext.view();

        if (view.size()>=(ivSize+tagSize))
        {
            return std::make_pair(
                            common::ConstDataBuf(view.data()+tagSize,ivSize),
                            common::ConstDataBuf(view.data(),tagSize)
                        );
        }
        ec=cryptError(CryptError::DECRYPTION_FAILED);
    }
    catch (const common::ErrorException& e)
    {
        ec=e.error();
    }
    return std::pair<common::ConstDataBuf,common::ConstDataBuf>();
}

/********************** AeadTraits **********************************/

//! AEAD traits
template <bool Encrypt>
struct AeadTraits
{
};

//! AEAD encryptor traits
template <>
struct AeadTraits<true>
{
    //! Encrypt data using pre-initialized encryptor
    template <typename Cipher,typename ContainerOutT,typename ContainerTagT, typename PlainTextT, typename AuthDataT>
    static common::Error encryptAndFinalize(
        Cipher* enc,
        const PlainTextT& plaintext,
        const AuthDataT& authdata,
        ContainerOutT& ciphertext,
        ContainerTagT& tag
    );

    //! Encrypt data in AEAD mode and pack IV and AEAD tag with ciphertext
    template <typename Cipher,typename ContainerOutT,typename PlainTextT,typename AuthDataT>
    static common::Error encryptPack(
        Cipher* enc,
        const PlainTextT& plaintext,
        const AuthDataT& authdata,
        ContainerOutT& ciphertext,
        const common::SpanBuffer& iv,
        size_t offsetOut
    );

    //! Stub to avoid compilation errors/warnings
    template <typename Cipher,typename ContainerOutT, typename CipherTextT, typename AuthDataT>
    static common::Error decryptAndFinalize(
        Cipher*,
        const CipherTextT&,
        const AuthDataT&,
        const common::SpanBuffer&,
        ContainerOutT&
    )
    {
        return cryptError(CryptError::INVALID_OPERATION);
    }

    //! Stub to avoid compilation errors/warnings
    template <typename Cipher,typename CipherTextT, typename AuthDataT, typename ContainerOutT>
    static common::Error decryptPack(
        Cipher*,
        const CipherTextT&,
        const AuthDataT&,
        ContainerOutT&
    )
    {
        return cryptError(CryptError::INVALID_OPERATION);
    }
};

//! AEAD decryptor traits
template <>
struct AeadTraits<false>
{
    //! Decrypt data using pre-initialized decryptor
    template <typename Cipher,typename ContainerOutT, typename CipherTextT, typename AuthDataT>
    static common::Error decryptAndFinalize(
        Cipher* dec,
        const CipherTextT& ciphertext,
        const AuthDataT& authdata,
        const common::SpanBuffer& tag,
        ContainerOutT& plaintext
    );

    //! Decrypt data taking IV and Tag from the beggining og ciphertext
    template <typename Cipher,typename CipherTextT,typename AuthDataT,typename ContainerOutT>
    static common::Error decryptPack(
        Cipher* dec,
        const CipherTextT& ciphertext,
        const AuthDataT& authdata,
        ContainerOutT& plaintext
    );

    //! Stub to avoid compilation errors/warnings
    template <typename Cipher,typename ContainerOutT,typename ContainerTagT, typename PlainTextT, typename AuthDataT>
    static common::Error encryptAndFinalize(
        Cipher*,
        const PlainTextT&,
        const AuthDataT&,
        ContainerOutT&,
        ContainerTagT&
    )
    {
        return cryptError(CryptError::INVALID_OPERATION);
    }

    //! Stub to avoid compilation errors/warnings
    template <typename Cipher,typename ContainerOutT, typename PlainTextT, typename AuthDataT>
    static common::Error encryptPack(
        Cipher*,
        const PlainTextT&,
        const AuthDataT&,
        ContainerOutT&,
        const common::SpanBuffer&,
        size_t
    )
    {
        return cryptError(CryptError::INVALID_OPERATION);
    }
};

// ------------------- AeadTraits<true> -------------------

//---------------------------------------------------------------
template <typename Cipher,typename ContainerOutT,typename ContainerTagT, typename PlainTextT, typename AuthDataT>
common::Error AeadTraits<true>::encryptAndFinalize(
    Cipher* enc,
    const PlainTextT& plaintext,
    const AuthDataT& authdata,
    ContainerOutT& ciphertext,
    ContainerTagT& tag
)
{
    // calculate authentication part on unencrypted data
    HATN_CHECK_RETURN(AeadArgTraits::auth(enc,authdata,ciphertext))

    // encrypt data
    HATN_CHECK_RETURN(AeadArgTraits::encrypt(enc,plaintext,ciphertext))

    // finalize
    size_t finalSize=0;
    HATN_CHECK_RETURN(enc->finalize(ciphertext,finalSize,ciphertext.size()))

    // get tag
    return enc->getTag(tag);
}

//---------------------------------------------------------------
template <typename Cipher,typename ContainerOutT,typename PlainTextT,typename AuthDataT>
common::Error AeadTraits<true>::encryptPack(
    Cipher* enc,
    const PlainTextT& plaintext,
    const AuthDataT& authdata,
    ContainerOutT& ciphertext,
    const common::SpanBuffer& iv,
    size_t offsetOut
)
{
    // resize buffer, we always use default IV size set by the cipher
    auto ivSize=enc->ivSize();
    auto tagSize=enc->getTagSize();
    auto ivOffset=tagSize+offsetOut;
    ciphertext.resize(ivOffset+ivSize);
    ciphertext.fill(0,offsetOut);

    if (iv.isEmpty())
    {
        // generate IV and put it in the beginning of the target buffer
        HATN_CHECK_RETURN(enc->generateIV(ciphertext,ivOffset))
    }
    else
    {
        auto span=iv.span();
        if (!span.first)
        {
            return common::Error(common::CommonError::INVALID_SIZE);
        }

        // copy input iv after tag in the beginning of the target buffer
        std::copy(span.second.data(),span.second.data()+span.second.size(),ciphertext.data()+ivOffset);
    }
    common::ConstDataBuf ivBuf(ciphertext,ivOffset,ivSize);
    HATN_CHECK_RETURN(enc->init(ivBuf));

    // calculate authentication part on unencrypted data
    HATN_CHECK_RETURN(AeadArgTraits::auth(enc,authdata,ciphertext))

    // encrypt data
    HATN_CHECK_RETURN(AeadArgTraits::encrypt(enc,plaintext,ciphertext))

    // finalize
    size_t finalSize=0;
    HATN_CHECK_RETURN(enc->finalize(ciphertext,finalSize,ciphertext.size()))

    // put the tag into the beginning of result buffer
    return enc->getTag(ciphertext,offsetOut);
}

// ------------------- AeadTraits<false> -------------------

//---------------------------------------------------------------
template <typename Cipher,typename ContainerOutT, typename CipherTextT, typename AuthDataT>
common::Error AeadTraits<false>::decryptAndFinalize(
    Cipher* dec,
    const CipherTextT& ciphertext,
    const AuthDataT& authdata,
    const common::SpanBuffer& tag,
    ContainerOutT& plaintext
)
{
    // calculate authentication part on unencrypted data
    HATN_CHECK_RETURN(AeadArgTraits::auth(dec,authdata,plaintext))

    // decrypt data
    HATN_CHECK_RETURN(AeadArgTraits::decrypt(dec,ciphertext,plaintext,0))

    // set tag
    HATN_CHECK_RETURN(dec->setTag(tag))

    // finalize
    size_t sizeOut=0;
    return dec->finalize(plaintext,sizeOut,plaintext.size());
}

//---------------------------------------------------------------
template <typename Cipher,typename CipherTextT,typename AuthDataT,typename ContainerOutT>
common::Error AeadTraits<false>::decryptPack(
    Cipher* dec,
    const CipherTextT& ciphertext,
    const AuthDataT& authdata,
    ContainerOutT& plaintext
)
{
    // extract IV and tag
    common::Error ec;
    auto ivAndTag=AeadArgTraits::extractIvAndTag(dec,ciphertext,ec);
    HATN_CHECK_EC(ec)

    // initialize decryptor
    HATN_CHECK_RETURN(dec->init(ivAndTag.first))

    // calculate authentication part on unencrypted data
    HATN_CHECK_RETURN(AeadArgTraits::auth(dec,authdata,plaintext))

    // decrypt data after tag
    HATN_CHECK_RETURN(AeadArgTraits::decrypt(dec,ciphertext,plaintext,
                                                              ivAndTag.first.size()+ivAndTag.second.size()))

    // set tag
    HATN_CHECK_RETURN(dec->setTag(ivAndTag.second))

    // finalize
    size_t sizeOut=0;
    return dec->finalize(plaintext,sizeOut,plaintext.size());
}

} // namespace detail

/********************** AeadWorker **********************************/

//---------------------------------------------------------------
template <bool Encrypt>
AeadWorker<Encrypt>::AeadWorker(const SymmetricKey* key)
    : CipherWorker<Encrypt>(key)
{
}

//---------------------------------------------------------------
template <bool Encrypt>
common::Error AeadWorker<Encrypt>::setTag(const char* data) noexcept
{
    if (
        Encrypt
        ||
        !this->key()->isAlgDefined()
            ||
         !this->key()->alg()->isType(CryptAlgorithm::Type::AEAD)
        )
    {
        return cryptError(CryptError::INVALID_OPERATION);
    }
    return doSetTag(data);
}

//---------------------------------------------------------------
template <bool Encrypt>
template <typename ContainerT>
common::Error AeadWorker<Encrypt>::setTag(const ContainerT& buf, size_t offset) noexcept
{
    size_t size=getTagSize();
    if (!checkInContainerSize(buf.size(),offset,size))
    {
        return common::Error(common::CommonError::INVALID_SIZE);
    }
    return setTag(buf.data()+offset);
}

//---------------------------------------------------------------
template <bool Encrypt>
common::Error AeadWorker<Encrypt>::setTag(const common::SpanBuffer &buf) noexcept
{
    try
    {
        return setTag(buf.view());
    }
    catch (const common::ErrorException& e)
    {
        return e.error();
    }
    return common::Error();
}

//---------------------------------------------------------------
template <bool Encrypt>
size_t AeadWorker<Encrypt>::getTagSize() const noexcept
{
    if (this->alg())
    {
        auto tagSize=this->alg()->tagSize();
        if (tagSize!=0)
        {
            return tagSize;
        }
        auto hashSize=this->alg()->hashSize(true);
        if (hashSize!=0)
        {
            return this->alg()->hashSize();
        }
    }
    return 16;
}

//---------------------------------------------------------------
template <bool Encrypt>
size_t AeadWorker<Encrypt>::ivSize() const noexcept
{
    if (this->alg())
    {
        return this->alg()->ivSize();
    }
    return 12;
}

//---------------------------------------------------------------
template <bool Encrypt>
size_t AeadWorker<Encrypt>::maxExtraSize() const
{
    Assert(this->alg()!=nullptr,"Algorithm is not set");
    return this->maxPadding()+ivSize()+getTagSize();
}

//---------------------------------------------------------------
template <bool Encrypt>
common::Error AeadWorker<Encrypt>::getTag(char* data) noexcept
{
    if (
        !Encrypt
        ||
        !this->key()->isAlgDefined()
            ||
         !this->key()->alg()->isType(CryptAlgorithm::Type::AEAD)
        )
    {
        return cryptError(CryptError::INVALID_OPERATION);
    }
    return doGetTag(data);
}

//---------------------------------------------------------------
template <bool Encrypt>
template <typename ContainerT>
common::Error AeadWorker<Encrypt>::getTag(
    ContainerT& tag,
    size_t offset
)
{
    size_t tagSize=getTagSize();
    if (tag.size()<(offset+tagSize))
    {
        tag.resize(offset+tagSize);
    }
    return getTag(tag.data()+offset);
}

//---------------------------------------------------------------
template <bool Encrypt>
template <typename ContainerOutT, typename PlainTextT, typename AuthDataT, typename ContainerTagT>
common::Error AeadWorker<Encrypt>::encryptAndFinalize(
    const PlainTextT& plaintext,
    const AuthDataT& authdata,
    ContainerOutT& ciphered,
    ContainerTagT& tag
)
{
    tag.clear();
    return detail::AeadTraits<Encrypt>::encryptAndFinalize(this,plaintext,authdata,ciphered,tag);
}

//---------------------------------------------------------------
template <bool Encrypt>
template <typename ContainerOutT, typename PlainTextT, typename AuthDataT>
common::Error AeadWorker<Encrypt>::encryptPack(
    const PlainTextT& plaintext,
    const AuthDataT& authdata,
    ContainerOutT& ciphertext,
    const common::SpanBuffer& iv,
    size_t offsetOut
)
{
    return detail::AeadTraits<Encrypt>::encryptPack(this,plaintext,authdata,ciphertext,iv,offsetOut);
}

//---------------------------------------------------------------
template <bool Encrypt>
template <typename ContainerOutT, typename CipherTextT, typename AuthDataT>
common::Error AeadWorker<Encrypt>::decryptAndFinalize(
    const CipherTextT& ciphertext,
    const AuthDataT& authdata,
    const common::SpanBuffer& tag,
    ContainerOutT& plaintext
)
{
    return detail::AeadTraits<Encrypt>::decryptAndFinalize(this,ciphertext,authdata,tag,plaintext);
}

//---------------------------------------------------------------
template <bool Encrypt>
template <typename ContainerOutT, typename CipherTextT, typename AuthDataT>
common::Error AeadWorker<Encrypt>::decryptPack(
    const CipherTextT& ciphertext,
    const AuthDataT& authdata,
    ContainerOutT& plaintext
)
{
    return detail::AeadTraits<Encrypt>::decryptPack(this,ciphertext,authdata,plaintext);
}

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTAEADWORKER_IPP
