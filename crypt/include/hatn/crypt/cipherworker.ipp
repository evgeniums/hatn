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

#include <hatn/crypt/keycontainer.h>
#include <hatn/crypt/cipherworker.h>

HATN_CRYPT_NAMESPACE_BEGIN

/********************** CipherWorker **********************************/

template <typename ContainerInT, typename ContainerOutT>
common::Error CipherWorker::process(
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

    if (getAlg()->isNone())
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
        size_t prepareSize=sizeIn+getMaxPadding();
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

template <typename ContainerOutT>
common::Error CipherWorker::finalize(
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

    if (getAlg()->isNone())
    {
        sizeOut=offsetOut;
        dataOut.resize(sizeOut);
        return common::Error();
    }

    size_t prepareSize=offsetOut;
    size_t possiblePadding=getMaxPadding();
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

template <typename ContainerInT, typename ContainerOutT>
common::Error CipherWorker::processAndFinalize(
    const ContainerInT& dataIn,
    ContainerOutT& dataOut,
    size_t offsetIn,
    size_t sizeIn,
    size_t offsetOut
)
{
    auto ec=canProcessAndFinalize();
    HATN_CHECK_EC(ec)

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

template <typename ContainerOutT>
common::Error CipherWorker::processAndFinalize(
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

template <typename ContainerOutT>
common::Error CipherWorker::processAndFinalize(
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

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTCIPHERWORKER_IPP
