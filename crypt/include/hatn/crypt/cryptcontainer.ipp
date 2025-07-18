/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file crypt/cryptcontainer.ipp
 *
 *      Encrypted container inline and template methods
 *
 */
/****************************************************************************/

#ifndef HATNCRYPTCRYPCONTAINER_IPP
#define HATNCRYPTCRYPCONTAINER_IPP

#include <hatn/common/containerutils.h>

#include <hatn/crypt/cryptcontainer.h>

#include <hatn/dataunit/visitors.h>

HATN_CRYPT_NAMESPACE_BEGIN

/*********************** CryptContainer **************************/

//---------------------------------------------------------------
inline void CryptContainer::setMasterKey(const SymmetricKey* key) noexcept
{
    m_masterKey=key;
}

//---------------------------------------------------------------
inline const SymmetricKey* CryptContainer::masterKey() const noexcept
{
    return m_masterKey;
}

inline void CryptContainer::setPassphrase(common::MemoryLockedArray passphrase)
{
    m_passphrase=std::move(passphrase);
}

//---------------------------------------------------------------
inline const common::MemoryLockedArray& CryptContainer::passphrase() const noexcept
{
    return m_passphrase;
}

//---------------------------------------------------------------
inline void CryptContainer::setKdfType(container_descriptor::KdfType type) noexcept
{
    m_descriptor.setFieldValue(container_descriptor::kdf_type,type);
}

//---------------------------------------------------------------
inline container_descriptor::KdfType CryptContainer::kdfType() const noexcept
{
    return m_descriptor.fieldValue(container_descriptor::kdf_type);
}

//---------------------------------------------------------------
inline void CryptContainer::setSalt(const common::ConstDataBuf& salt)
{
    if (salt.size()>std::numeric_limits<uint16_t>::max())
    {
        throw common::ErrorException(common::Error(common::CommonError::INVALID_SIZE));
    }
    m_descriptor.setFieldValue(container_descriptor::salt,salt.data(),salt.size());
}

//---------------------------------------------------------------
inline common::ConstDataBuf CryptContainer::salt() const noexcept
{
    const auto& field=m_descriptor.field(container_descriptor::salt);
    return common::ConstDataBuf(field.buf()->data(),field.buf()->size());
}

//---------------------------------------------------------------
inline void CryptContainer::setChunkMaxSize(uint32_t size) noexcept
{
    m_descriptor.setFieldValue(container_descriptor::chunk_max_size,size);
}

//---------------------------------------------------------------
inline uint32_t CryptContainer::chunkMaxSize() const noexcept
{
    return m_descriptor.fieldValue(container_descriptor::chunk_max_size);
}

//---------------------------------------------------------------
inline void CryptContainer::setFirstChunkMaxSize(uint32_t size) noexcept
{
    m_descriptor.setFieldValue(container_descriptor::first_chunk_max_size,size);
}

//---------------------------------------------------------------
inline uint32_t CryptContainer::firstChunkMaxSize() const noexcept
{
    return m_descriptor.fieldValue(container_descriptor::first_chunk_max_size);
}

//---------------------------------------------------------------
inline uint32_t CryptContainer::packedExtraSize() const
{
    size_t maxExtraSize=0;
    size_t sizePrefixLength=0;

    if (!m_streamingMode)
    {
        // size is prepended to data in chunk
        sizePrefixLength=sizeof(uint32_t);

        // find max extra size that can be used for padding
        if (m_enc)
        {
            maxExtraSize=m_enc->maxExtraSize();
        }
        else
        {
            HATN_CHECK_THROW(const_cast<CryptContainer*>(this)->checkOrCreateDecryptor())
            maxExtraSize=m_dec->maxExtraSize();
        }
    }
    else
    {
        auto prefixSize=streamPrefixSize();
        if (!prefixSize)
        {
            maxExtraSize=prefixSize.value();
        }
    }

    return static_cast<uint32_t>(maxExtraSize+sizePrefixLength);
}

//---------------------------------------------------------------
inline uint32_t CryptContainer::maxPackedChunkSize(uint32_t seqnum, uint32_t containerSize) const
{
    if (chunkMaxSize()!=0 || firstChunkMaxSize()!=0)
    {
        return maxPackedChunkSize(seqnum);
    }

    uint32_t chunkSizeM=0;
    if (seqnum==0)
    {
        // first chunk
        chunkSizeM=firstChunkMaxSize();
        if (chunkSizeM==0)
        {
            // if zero then fallback to ordinary chunk size
            chunkSizeM=chunkMaxSize();
            if (chunkSizeM==0)
            {
                // if zero then fallback to containerSize
                chunkSizeM=containerSize;
            }
        }
    }
    else if (chunkSizeM==0)
    {
        // all other chunks
        chunkSizeM=chunkMaxSize();
        if (chunkSizeM==0)
        {
            // if zero then fallback to containerSize without first chunk size
            chunkSizeM=containerSize-maxPackedChunkSize(0,containerSize);
        }
    }

    // done
    return static_cast<uint32_t>(chunkSizeM+packedExtraSize());
}

//---------------------------------------------------------------
inline uint32_t CryptContainer::maxPackedChunkSize(uint32_t seqnum) const
{
    if (seqnum!=0)
    {
        if (m_maxPackedChunkSize)
        {
            return m_maxPackedChunkSize.value();
        }
    }
    else if (m_maxPackedFirstChunkSize)
    {
        return m_maxPackedFirstChunkSize.value();
    }

    uint32_t chunkSizeM=0;
    if (seqnum==0)
    {
        // first chunk
        chunkSizeM=firstChunkMaxSize();
        if (chunkSizeM==0)
        {
            // if zero then fallback to ordinary chunk size
            chunkSizeM=chunkMaxSize();
        }
    }
    else if (chunkSizeM==0)
    {
        // all other chunks
        chunkSizeM=chunkMaxSize();
        if (chunkSizeM==0)
        {
            // if zero then fallback to containerSize without first chunk size
            chunkSizeM=maxPackedChunkSize(0);
        }
    }
    Assert(chunkSizeM!=0,"Chunk size must not be zero");

    // store calculated size
    uint32_t result=static_cast<uint32_t>(chunkSizeM+packedExtraSize());
    if (seqnum==0)
    {
        m_maxPackedFirstChunkSize=result;
    }
    else
    {
        m_maxPackedChunkSize=result;
    }

    // done
    return result;
}

//---------------------------------------------------------------
inline uint32_t CryptContainer::maxPlainChunkSize(uint32_t seqnum) const
{
    uint32_t chunkSizeM=0;
    if (seqnum==0)
    {
        chunkSizeM=firstChunkMaxSize();
        if (chunkSizeM==0)
        {
            chunkSizeM=chunkMaxSize();
        }
    }
    else
    {
        chunkSizeM=chunkMaxSize();
    }
    return chunkSizeM;
}

//---------------------------------------------------------------
inline void CryptContainer::setCipherSuite(const CipherSuite* suite) noexcept
{
    m_cipherSuite=suite;
    if (m_cipherSuite!=nullptr && (m_suites==nullptr || m_suites==&CipherSuitesGlobal::instance()))
    {
        m_suites=m_cipherSuite->suites();
    }
}

//---------------------------------------------------------------
inline const CipherSuite* CryptContainer::cipherSuite() const noexcept
{
    return m_cipherSuite;
}

//---------------------------------------------------------------
inline void CryptContainer::setAttachCipherSuiteEnabled(bool enable) noexcept
{
    m_attachSuite=enable;
}

//---------------------------------------------------------------
inline bool CryptContainer::isAttachCipherSuiteEnabled() const noexcept
{
    return m_attachSuite;
}

//---------------------------------------------------------------
template <typename ContainerT>
common::Error CryptContainer::packHeader(
        ContainerT& result,
        uint16_t descriptorSize,
        uint64_t contentSize,
        size_t offsetOut
    )
{
    if (offsetOut>result.size())
    {
        return common::Error(common::CommonError::INVALID_SIZE);
    }

    result.resize(offsetOut+CryptContainerHeader::size());
    return packHeader(result,descriptorSize,contentSize,offsetOut);
}

//---------------------------------------------------------------
inline void CryptContainer::setCiphertextSize(char* headerPtr,uint64_t size) noexcept
{
    CryptContainerHeader header(headerPtr);
    header.setCiphertextSize(size);
}

//---------------------------------------------------------------
inline uint64_t CryptContainer::cipherTextSize(const char *headerPtr) const noexcept
{
    CryptContainerHeader header(headerPtr);
    return header.ciphertextSize();
}

//---------------------------------------------------------------
inline common::Error CryptContainer::packHeader(
        char* headerPtr,
        uint16_t descriptorSize,
        uint64_t contentSize
    ) noexcept
{
    CryptContainerHeader header(headerPtr);
    header.reset(m_streamingMode);
    header.setPlaintextSize(contentSize);
    header.setDescriptorSize(descriptorSize);

    return OK;
}

//---------------------------------------------------------------
template <typename ContainerT>
common::Error CryptContainer::packHeaderAndDescriptor(
        ContainerT& result,
        uint64_t contentSize,
        size_t offsetOut
    )
{
    if (offsetOut>result.size())
    {
        return common::Error(common::CommonError::INVALID_SIZE);
    }

    size_t presize=offsetOut+headerSize();
    result.resize(presize);
    HATN_CHECK_RETURN(packDescriptor(result,result.size()))

    return packHeader(result.data(),static_cast<uint16_t>(result.size()-presize),contentSize);
}

//---------------------------------------------------------------
template <typename ContainerT>
common::Error CryptContainer::unpackHeader(
        const ContainerT& container,
        uint64_t& plaintextSize,
        uint16_t& descriptorSize,
        uint64_t& ciphertextSize
    )
{
    if (container.size()<headerSize())
    {
        return common::Error(common::CommonError::INVALID_SIZE);
    }

    CryptContainerHeader header(container.data());
    if (!header.checkPrefix())
    {
        return cryptError(CryptError::INVALID_CONTENT_FORMAT);
    }
    if (header.version()>CryptContainerHeader::VERSION)
    {
        return cryptError(CryptError::UNSUPPORTED_FORMAT_VERSION);
    }
    descriptorSize=static_cast<uint16_t>(header.descriptorSize());
    plaintextSize=header.plaintextSize();
    ciphertextSize=header.ciphertextSize();
    m_streamingMode=header.isStreamingMode();

    return OK;
}

//---------------------------------------------------------------
template <typename ContainerT>
common::Error CryptContainer::packDescriptor(
        ContainerT& result,
        size_t offsetOut
    )
{
    HATN_CHECK_RETURN(checkState())

    if (salt().empty() && m_autoSalt)
    {
        auto randGen=m_suites->defaultRandomGenerator();
        if (randGen!=nullptr)
        {
            common::ByteArray saltBuf;
            auto ec=randGen->randContainer(saltBuf,16,8);
            if (ec)
            {
                return cryptError(CryptError::SALT_GEN_FAILED,ec);
            }
            setSalt(saltBuf);
        }
        else
        {
            return cryptError(CryptError::SALT_GEN_FAILED,cryptError((CryptError::RANDOM_GENERATOR_NOT_DEFINED)));
        }
    }

    if (m_attachSuite)
    {
        m_descriptor.setFieldValue(container_descriptor::cipher_suite,m_cipherSuite->suite());
        m_descriptor.resetField(container_descriptor::cipher_suite_id);
    }
    else
    {
        m_descriptor.resetField(container_descriptor::cipher_suite);
        m_descriptor.setFieldValue(container_descriptor::cipher_suite_id,m_cipherSuite->id());
    }

    if (du::io::serializeToBuf(m_descriptor,result,offsetOut)<0)
    {
        return cryptError(CryptError::SERIALIZE_CONTAINER_FAILED);
    }
    return OK;
}

//---------------------------------------------------------------
template <typename ContainerT>
common::Error CryptContainer::unpackDescriptor(
        const ContainerT& container
    )
{
    // deserialize descriptor
    if (!du::io::deserializeInline(m_descriptor,container))
    {
        return cryptError(CryptError::PARSE_CONTAINER_FAILED);
    }

#if 0
    std::cout << "Unpacked descriptor " << m_descriptor.toString(true) << std::endl;
#endif

    // find suite
    const auto& suiteField=m_descriptor.field(container_descriptor::cipher_suite);
    if (suiteField.isSet())
    {
        m_extractedSuite.setSuite(suiteField.get());
        m_cipherSuite=&m_extractedSuite;
    }
    else
    {
        const auto& suiteIdField=m_descriptor.field(container_descriptor::cipher_suite_id);
        if (suiteIdField.isSet() && suiteIdField.size()!=0)
        {
            m_cipherSuite=m_suites->suite(suiteIdField.buf()->data(),suiteIdField.buf()->size());
        }
    }
    if (m_cipherSuite==nullptr)
    {
        m_cipherSuite=m_suites->defaultSuite();
        if (m_cipherSuite==nullptr)
        {
            return cryptError(CryptError::INVALID_CIPHER_SUITE);
        }
    }

    // create master key from passphrase if not set yet
    if (m_masterKey==nullptr)
    {
        Error ec;
        m_passphraseMasterKey=m_cipherSuite->createPassphraseKey(ec);
        HATN_CHECK_EC(ec)
        m_passphraseMasterKey->set(m_passphrase);
    }

    // done
    return OK;
}

//---------------------------------------------------------------
template <typename ContainerT>
common::Error CryptContainer::unpackHeaderAndDescriptor(
    const ContainerT& container,
    uint64_t& plaintextSize,
    uint64_t& ciphertextSize,
    size_t& consumedSize
)
{
    uint16_t descriptorSize=0;
    HATN_CHECK_RETURN(unpackHeader(container,plaintextSize,descriptorSize,ciphertextSize))
    consumedSize+=headerSize();

    if (container.size()<(consumedSize+descriptorSize))
    {
        return common::Error(common::CommonError::INVALID_SIZE);
    }
    if (ciphertextSize<plaintextSize)
    {
        return common::Error(common::CommonError::INVALID_SIZE);
    }

    common::ConstDataBuf descriptor(container.data()+consumedSize,descriptorSize);
    HATN_CHECK_RETURN(unpackDescriptor(descriptor))
    consumedSize+=descriptorSize;
    return OK;
}

//---------------------------------------------------------------
template <typename ContainerInT, typename ContainerOutT>
common::Error CryptContainer::pack(
        const ContainerInT& plaintext,
        ContainerOutT& result,
        const common::ConstDataBuf& salt
    )
{
    if (m_streamingMode)
    {
        return cryptError(CryptError::INVALID_CRYPT_CONTAINER_STREAM_MODE);
    }

    reset();
    bool success=false;
    common::RunOnScopeExit guard(
        [this,&success,&result]()
        {
            reset();
            if (!success)
            {
                result.clear();
            }
        }
    );

    try
    {
        if (!salt.isEmpty())
        {
            setSalt(salt);
        }

        auto initialResultOffset=result.size();
        HATN_CHECK_RETURN(packHeaderAndDescriptor(result,plaintext.size(),result.size()))

        auto maxChunkSize=firstChunkMaxSize();
        if (maxChunkSize==0)
        {
            maxChunkSize=chunkMaxSize();
        }

        auto initialResultSize=result.size();
        uint32_t seqnum=0;
        size_t offset=0;
        while(offset<plaintext.size() || plaintext.empty())
        {
            auto size=plaintext.size()-offset;
            if (size>maxChunkSize && maxChunkSize!=0)
            {
                size=maxChunkSize;
            }
            HATN_CHECK_RETURN(packChunk(common::SpanBuffer(plaintext,offset,size),result,seqnum,result.size()))
            if (seqnum==0)
            {
                maxChunkSize=chunkMaxSize();
            }
            ++seqnum;
            offset+=size;
            if (plaintext.empty())
            {
                break;
            }
        }

        setCiphertextSize(result.data()+initialResultOffset,result.size()-initialResultSize);
    }
    catch (const common::ErrorException& e)
    {
        return e.error();
    }
    success=true;
    return OK;
}

//---------------------------------------------------------------
template <typename ContainerInT, typename ContainerOutT>
common::Error CryptContainer::unpack(
        const ContainerInT& input,
        ContainerOutT& plaintext
    )
{
    if (m_streamingMode)
    {
        return cryptError(CryptError::INVALID_CRYPT_CONTAINER_STREAM_MODE);
    }

    reset(true);
    bool success=false;
    common::RunOnScopeExit guard(
        [this,&success,&plaintext]()
        {
            reset(true);
            if (!success)
            {
                plaintext.clear();
            }
        }
    );

    // unpack header
    size_t dataOffset=0;
    uint64_t plaintextSize=0;
    uint64_t ciphertextSize=0;
    HATN_CHECK_RETURN(unpackHeaderAndDescriptor(input,plaintextSize,ciphertextSize,dataOffset))

    // there must be at least one chunk
    if (dataOffset==input.size())
    {
        return common::Error(common::CommonError::INVALID_SIZE);
    }

    // create decryptor if not exists
    HATN_CHECK_RETURN(checkOrCreateDecryptor())

    // setup sizes of chunks
    auto dataSize=input.size()-dataOffset;
    if (dataSize<ciphertextSize)
    {
        return common::Error(common::CommonError::INVALID_SIZE);
    }
    uint32_t firstChunkSizeM=static_cast<uint32_t>(maxPackedChunkSize(0,static_cast<uint32_t>(ciphertextSize)));
    uint32_t chunkSizeM=static_cast<uint32_t>(maxPackedChunkSize(1,static_cast<uint32_t>(ciphertextSize)));
    size_t initialResultSize=plaintext.size();

    // unpack chunks
    uint32_t seqnum=0u;
    uint32_t maxChunkSize=firstChunkSizeM;
    uint64_t processedSize=0;
    while(processedSize<ciphertextSize)
    {
        size_t chunkSize= static_cast<size_t>(ciphertextSize-processedSize);
        if (maxChunkSize!=0 && chunkSize>maxChunkSize)
        {
            chunkSize=maxChunkSize;
        }
        common::SpanBuffer chunk(input,dataOffset+static_cast<size_t>(processedSize),chunkSize);
        HATN_CHECK_RETURN(unpackChunk(chunk,plaintext,seqnum))
        processedSize+=chunkSize;
        if (seqnum==0)
        {
            maxChunkSize=chunkSizeM;
        }
        ++seqnum;
    }

    uint64_t decryptedSize=plaintext.size()-initialResultSize;
    if (decryptedSize!=plaintextSize)
    {
        return common::Error(common::CommonError::INVALID_SIZE);
    }

    // done
    success=true;
    return OK;
}

//---------------------------------------------------------------
template <typename BufferT, typename ContainerOutT>
common::Error CryptContainer::packChunk(
        const BufferT& plaintext,
        ContainerOutT& result,
        const common::ConstDataBuf& info,
        size_t offsetOut
    )
{
    // check size
    uint32_t maxChunkSize=0;
    size_t checkInputSize=0;
    try
    {
        maxChunkSize=chunkMaxSize();
        if (maxChunkSize!=0)
        {
            auto checkInputSize=common::SpanBufferTraits::size(plaintext);
            if (checkInputSize>maxChunkSize)
            {
                return common::Error(common::CommonError::INVALID_SIZE);
            }
        }
    }
    catch (const common::ErrorException& e)
    {
        return e.error();
    }
    if (offsetOut>result.size())
    {
        return common::Error(common::CommonError::INVALID_SIZE);
    }

    // check state
    HATN_CHECK_RETURN(checkState())

    // forward to stream processing if in streaming mode
    if (m_streamingMode)
    {
        auto prefixSize=streamPrefixSize();
        HATN_CHECK_RESULT(prefixSize)
        result.resize(plaintext.size()+offsetOut+prefixSize.value());
        HATN_CHECK_RETURN(initStreamEncryptor(result,info,offsetOut))
        return encryptStream(plaintext,result,offsetOut+prefixSize.value());
    }

    // derive key
    common::SharedPtr<SymmetricKey> derivedKey;
    const SymmetricKey* key=nullptr;
    HATN_CHECK_RETURN(deriveKey(key,derivedKey,info))

    // create encryptor if not exists
    if (m_enc.isNull())
    {
        common::Error ec;
        m_enc=m_cipherSuite->createAeadEncryptor(ec);
        HATN_CHECK_EC(ec)
        if (m_enc.isNull())
        {
            return cryptError(CryptError::NOT_SUPPORTED_BY_CIPHER_SUITE);
        }
    }

    // prepare auth data
    common::SpanBuffers authData{salt(),info};

    // reserve 4 bytes for chunk size
    uint32_t size=0;
    size_t sizePrefixLength=sizeof(size);
    result.resize(offsetOut+sizePrefixLength);

    // pack data
    HATN_CHECK_RETURN(AEAD::encryptPack(m_enc.get(),key,plaintext,authData,result,common::SpanBuffer(),result.size()))

    // fill chunk size
    auto targetSize=result.size();
    size=static_cast<uint32_t>(targetSize-offsetOut-sizePrefixLength);
    uint32_t littleEndianSize=boost::endian::native_to_little(size);
    memcpy(result.data()+offsetOut,&littleEndianSize,sizeof(littleEndianSize));

    // if chunk is of max size then it must be aligned to size to fit max possible extra size
    // thus, all chunks for plaintext of maxChunkSize will have the same size regardless of the actual encrypted size
    if (checkInputSize!=0 && checkInputSize==maxChunkSize)
    {
        size_t resultSize=maxChunkSize+m_enc->maxExtraSize();
        Assert(resultSize>=size,"Invalid size");
        result.resize(offsetOut+resultSize+sizePrefixLength);
        result.fill(0,offsetOut+targetSize);
    }

    // done
    return OK;
}

//---------------------------------------------------------------
template <typename BufferT, typename ContainerOutT>
common::Error CryptContainer::packFirstChunk(
        const BufferT& plaintext,
        ContainerOutT& result,
        const common::ConstDataBuf& info,
        size_t offsetOut
    )
{
    auto keepSize=chunkMaxSize();
    auto firstSize=firstChunkMaxSize();
    if (firstSize!=0)
    {
        setChunkMaxSize(firstSize);
    }
    auto ec=packChunk(plaintext,result,info,offsetOut);
    setChunkMaxSize(keepSize);
    return ec;
}

//---------------------------------------------------------------
template <typename BufferT, typename ContainerOutT>
common::Error CryptContainer::packChunk(
        const BufferT& plaintext,
        ContainerOutT& result,
        uint32_t seqnum,
        size_t offsetOut
    )
{
    boost::endian::little_to_native_inplace(seqnum);
    if (seqnum!=0)
    {
        return packChunk(plaintext,result,common::ConstDataBuf(reinterpret_cast<const char*>(&seqnum),sizeof(seqnum)),offsetOut);
    }
    return packFirstChunk(plaintext,result,common::ConstDataBuf(reinterpret_cast<const char*>(&seqnum),sizeof(seqnum)),offsetOut);
}

//--------------------------------------------------------------
template <typename BufferT, typename ContainerOutT>
common::Error CryptContainer::unpackChunk(
        const BufferT& ciphertext,
        ContainerOutT& result,
        const common::ConstDataBuf& info
    )
{
    if (ciphertext.empty())
    {
        return OK;
    }

    // check state
    HATN_CHECK_RETURN(checkState())

    // forward to stream processing if in streaming mode
    if (m_streamingMode)
    {
        auto r=initStreamDecryptor(ciphertext,info);
        if (r)
        {
            return r.error();
        }
        common::ConstDataBuf cipherbuf{ciphertext,r.value(),0};
        return decryptStream(cipherbuf,result,result.size());
    }

    // prepare auth data
    common::SpanBuffers authData{salt(),info};

    // derive key
    common::SharedPtr<SymmetricKey> derivedKey;
    const SymmetricKey* key=nullptr;
    HATN_CHECK_RETURN(deriveKey(key,derivedKey,info))

    // create decryptor if not exists
    HATN_CHECK_RETURN(checkOrCreateDecryptor())

    // extract chunk size
    try
    {
        // extract and check size
        common::DataBuf sizeBuf;
        uint32_t size=0;
        auto buffer=common::SpanBufferTraits::extractPrefix(ciphertext,sizeBuf,sizeof(size));
        memcpy(&size,sizeBuf.data(),sizeof(size));
        boost::endian::little_to_native_inplace(size);
        auto actualSize=common::SpanBufferTraits::size(buffer);
        if (size==0)
        {
            // zero size ok
            return OK;
        }
        if (actualSize<size)
        {
            // buffer underflow
            return common::Error(common::CommonError::INVALID_SIZE);
        }
        if (actualSize>size)
        {
            // buffer contains more data than needed for decryption, use only size bytes
            auto buffers=common::SpanBufferTraits::split(buffer,size);
            buffer=buffers.first;
        }

        // unpack chunk
        return AEAD::decryptPack(m_dec.get(),key,buffer,authData,result);
    }
    catch (const common::ErrorException& e)
    {
        return e.error();
    }
    return OK;
}

//---------------------------------------------------------------
template <typename BufferT, typename ContainerOutT>
common::Error CryptContainer::unpackChunk(
        const BufferT& ciphertext,
        ContainerOutT& result,
        uint32_t seqnum
    )
{
    boost::endian::little_to_native_inplace(seqnum);
    return unpackChunk(ciphertext,result,common::ConstDataBuf(reinterpret_cast<const char*>(&seqnum),sizeof(seqnum)));
}

//---------------------------------------------------------------
inline common::Error CryptContainer::checkState() const noexcept
{
    if (m_cipherSuite==nullptr)
    {
        return cryptError(CryptError::INVALID_CIPHER_SUITE);
    }
    if (m_masterKey==nullptr)
    {
        if (m_passphrase.empty())
        {
            return cryptError(CryptError::INVALID_KEY);
        }

        // create master key from passphrase if not set yet
        Error ec;
        m_passphraseMasterKey=m_cipherSuite->createPassphraseKey(ec);
        HATN_CHECK_EC(ec)
        m_passphraseMasterKey->set(m_passphrase);
        m_masterKey=m_passphraseMasterKey.get();
    }
    return OK;
}

//---------------------------------------------------------------
inline const common::pmr::AllocatorFactory* CryptContainer::factory() const noexcept
{
    return m_factory;
}

//---------------------------------------------------------------
inline common::Error CryptContainer::checkOrCreateDecryptor()
{
    if (m_dec.isNull())
    {
        common::Error ec;
        m_dec=m_cipherSuite->createAeadDecryptor(ec);
        HATN_CHECK_EC(ec)
        if (m_dec.isNull())
        {
            return cryptError(CryptError::NOT_SUPPORTED_BY_CIPHER_SUITE);
        }
    }
    return OK;
}

//---------------------------------------------------------------

template <typename ContainerOutT>
common::Error CryptContainer::initStreamEncryptor(
        ContainerOutT& ciphertext,
        uint32_t seqnum,
        size_t offset
    )
{
    boost::endian::little_to_native_inplace(seqnum);
    return initStreamEncryptor(ciphertext,common::ConstDataBuf(reinterpret_cast<const char*>(&seqnum),sizeof(seqnum)),offset);
}

//---------------------------------------------------------------

template <typename ContainerOutT>
common::Error CryptContainer::initStreamEncryptor(
        ContainerOutT& ciphertext,
        const common::ConstDataBuf& info,
        size_t offset
    )
{
    if (!m_streamingMode)
    {
        return cryptError(CryptError::INVALID_CRYPT_CONTAINER_STREAM_MODE);
    }

    // check state
    HATN_CHECK_RETURN(checkState())

    // derive key
    const SymmetricKey* key=nullptr;
    HATN_CHECK_RETURN(deriveKey(key,m_streamKeyHolder,info))

//! @todo Remove commented
#if 0
    std::string bdata;
    common::ContainerUtils::rawToHex(lib::string_view{key->content().data(),std::min(key->content().size(),size_t(32))},bdata);
    std::cout << "Derive encrypt key \"" << bdata << "...\" offset " << offset << std::endl;
#endif

    // create stream encryptor if not exists
    common::Error ec;
    m_streamEnc=m_cipherSuite->createSEncryptor(ec);
    HATN_CHECK_EC(ec)
    if (m_streamEnc.isNull())
    {
        return cryptError(CryptError::NOT_SUPPORTED_BY_CIPHER_SUITE);
    }
    m_streamEnc->setKey(key);

    // init stream encryptor
    return m_streamEnc->initStream(ciphertext,offset);
}

//---------------------------------------------------------------

template <typename BufferT, typename ContainerOutT>
common::Error CryptContainer::encryptStream(
        const BufferT& plaintext,
        ContainerOutT& ciphertext,
        size_t offset
    )
{
    Assert(m_streamEnc,"Stream encryptor must be initialized first");

    return m_streamEnc->encryptStream(plaintext,ciphertext,offset);
}

//---------------------------------------------------------------

template <typename BufferT>
Result<size_t> CryptContainer::initStreamDecryptor(
        const BufferT& ciphertext,
        uint32_t seqnum,
        size_t offset
    )
{
    boost::endian::little_to_native_inplace(seqnum);
    return initStreamDecryptor(ciphertext,common::ConstDataBuf(reinterpret_cast<const char*>(&seqnum),sizeof(seqnum)),offset);
}

//---------------------------------------------------------------

template <typename BufferT>
Result<size_t> CryptContainer::initStreamDecryptor(
        const BufferT& ciphertext,
        const common::ConstDataBuf& info,
        size_t offset
    )
{
    if (!m_streamingMode)
    {
        return cryptError(CryptError::INVALID_CRYPT_CONTAINER_STREAM_MODE);
    }

    // check state
    HATN_CHECK_RETURN(checkState())

    // derive key
    const SymmetricKey* key=nullptr;
    HATN_CHECK_RETURN(deriveKey(key,m_streamKeyHolder,info))

//! @todo Remove commented
#if 0
    std::string bdata;
    common::ContainerUtils::rawToHex(lib::string_view{key->content().data(),std::min(key->content().size(),size_t(32))},bdata);
    std::cout << "Derive decrypt key \"" << bdata << "...\" offset " << offset << std::endl;
#endif

    // create stream encryptor if not exists
    common::Error ec;
    m_streamDec=m_cipherSuite->createSDecryptor(ec);
    HATN_CHECK_EC(ec)
    if (m_streamDec.isNull())
    {
        return cryptError(CryptError::NOT_SUPPORTED_BY_CIPHER_SUITE);
    }
    m_streamDec->setKey(key);

    // init stream encryptor
    return m_streamDec->initStream(ciphertext,offset);
}

//---------------------------------------------------------------

template <typename BufferT, typename ContainerOutT>
common::Error CryptContainer::decryptStream(
        const BufferT& ciphertext,
        ContainerOutT& plaintext,
        size_t offset
    )
{
    Assert(m_streamDec,"Stream decryptor must be initialized first");

    return m_streamDec->decryptStream(ciphertext,plaintext,offset);
}

//---------------------------------------------------------------
HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTCRYPCONTAINER_IPP
