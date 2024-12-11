/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/mac.cpp
  *
  *   Base class for MAC (Message Authentication Code) processing
  *
  */

/****************************************************************************/

#include <hatn/crypt/digest.h>
#include <hatn/crypt/cryptplugin.h>

namespace hatn {

using namespace common;

namespace crypt {

/*********************** DigestBase **************************/

//---------------------------------------------------------------
DigestBase::DigestBase():m_initialized(false)
{}

//---------------------------------------------------------------
common::Error DigestBase::process(const SpanBuffer &dataIn, size_t offset, size_t backOffset)
{
    try
    {
        auto view=dataIn.view();
        size_t processSize=0;
        if (!checkInContainerSize(view.size(),offset,processSize,backOffset))
        {
            return common::Error(common::CommonError::INVALID_SIZE);
        }
        return process(view,processSize,offset);
    }
    catch (common::ErrorException& e)
    {
        return e.error();
    }
    return common::Error();
}

//---------------------------------------------------------------
common::Error DigestBase::process(const SpanBuffers &dataIn, size_t offset, size_t backOffset)
{
    try
    {
        size_t i=0;
        for (auto&& it:dataIn)
        {
            size_t currentOffset=0;
            size_t currentBackOffset=0;
            if (i==0)
            {
                currentOffset=offset;
            }
            if (i==(dataIn.size()-1))
            {
                currentBackOffset=backOffset;
            }

            auto view=it.view();
            size_t processSize=0;
            if (!checkInContainerSize(view.size(),currentOffset,processSize,currentBackOffset))
            {
                return common::Error(common::CommonError::INVALID_SIZE);
            }
            HATN_CHECK_RETURN(process(view,processSize,currentOffset))
            ++i;
        }
    }
    catch (common::ErrorException& e)
    {
        return e.error();
    }
    return common::Error();
}

//---------------------------------------------------------------
void DigestBase::reset() noexcept
{
    m_initialized=false;
    doReset();
}

/*********************** Digest **************************/

//---------------------------------------------------------------
common::Error Digest::finalizeAndCheck(const char *digest, size_t digestSize)
{
    common::ByteArray calcTag;
    HATN_CHECK_RETURN(finalize(calcTag));
    if (digestSize>calcTag.size())
    {
        return makeCryptError(CryptErrorCode::DIGEST_MISMATCH);
    }
    size_t size=(std::min)(calcTag.size(),digestSize);
    if (memcmp(calcTag.data(),digest,size)!=0)
    {
        return makeCryptError(CryptErrorCode::DIGEST_MISMATCH);
    }
    return common::Error();
}

//---------------------------------------------------------------
common::Error Digest::beforeInit() noexcept
{
    if (alg()->type()!=CryptAlgorithm::Type::DIGEST)
    {
        return makeCryptError(CryptErrorCode::INVALID_ALGORITHM);
    }
    return common::Error();
}

//---------------------------------------------------------------
size_t Digest::resultSize() const
{
    if (!isAlgDefined())
    {
        throw common::ErrorException(makeCryptError(CryptErrorCode::INVALID_ALGORITHM));
    }
    return alg()->hashSize();
}

//---------------------------------------------------------------
common::Error Digest::init(const CryptAlgorithm *algorithm) noexcept
{
    if (algorithm==nullptr)
    {
        algorithm=m_alg.ptr;
    }
    m_alg.ptr=algorithm;

    if (!isAlgDefined())
    {
        return makeCryptError(CryptErrorCode::INVALID_ALGORITHM);
    }

    reset();
    HATN_CHECK_RETURN(beforeInit())

    auto ec=doInit();
    setInitialized(!ec);
    return ec;
}

//---------------------------------------------------------------
common::Error Digest::finalizeAndCheck(const SpanBuffer &digest)
{
    try
    {
        auto view=digest.view();
        return finalizeAndCheck(view);
    }
    catch (const common::ErrorException& e)
    {
        return e.error();
    }
    return common::Error();
}

//---------------------------------------------------------------
HATN_CRYPT_NAMESPACE_END
