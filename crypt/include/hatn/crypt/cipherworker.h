/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/cipherworker.h
 *
 *      Base classes for implementation of encryption/decryption workers
 *
 */
/****************************************************************************/

#ifndef HATNCRYPTCIPHERWORKER_H
#define HATNCRYPTCIPHERWORKER_H

#include <type_traits>

#include <hatn/common/bytearray.h>
#include <hatn/common/spanbuffer.h>

#include <hatn/crypt/crypt.h>
#include <hatn/crypt/crypterror.h>
#include <hatn/crypt/cryptalgorithm.h>

HATN_CRYPT_NAMESPACE_BEGIN

/********************** CipherWorker **********************************/

//! Base class for data encryptors and decryptors
class CipherWorker
{
    public:

        CipherWorker() : m_initialized(false)
        {}

        //! Dtor
        virtual ~CipherWorker()=default;

        CipherWorker(const CipherWorker&)=delete;
        CipherWorker(CipherWorker&&) =default;
        CipherWorker& operator=(const CipherWorker&)=delete;
        CipherWorker& operator=(CipherWorker&&) =default;

        //! Reset worker so that it can be used again with new data
        inline void reset() noexcept
        {
            m_initialized=false;
            doReset();
        }

        /**
         * @brief Process block of data
         * @param dataIn Buffer with input data
         * @param dataOut Buffer for output result
         * @param sizeOut Resulting size
         * @param sizeIn Input size to process
         * @param offsetIn Offset in input buffer for processing
         * @param offsetOut Offset in output buffer starting from which to put result
         * @param lastBlock Finalize processing
         * @param noResize Do not resize output container, take it as is
         * @return Operation status
         */
        template <typename ContainerInT, typename ContainerOutT>
        common::Error process(
            const ContainerInT& dataIn,
            ContainerOutT& dataOut,
            size_t& sizeOut,
            size_t sizeIn=0,
            size_t offsetIn=0,
            size_t offsetOut=0,
            bool lastBlock=false,
            bool noResize=false
        );

        /**
         * @brief Process last block of data and finalize processing
         * @param dataOut Buffer for output result
         * @param sizeOut Resulting size
         * @param offsetOut Offset in output buffer starting from which to put result
         * @return Operation status
         */
        template <typename ContainerOutT>
        common::Error finalize(
            ContainerOutT& dataOut,
            size_t& sizeOut,
            size_t offsetOut=0
        );

        /**
        * @brief Process the whole data buffer and finalize result
        * @param dataIn Buffer with input data
        * @param dataOut Buffer for output result
        * @param offsetIn Offset in input buffer for processing
        * @param offsetOut Offset in output buffer starting from which to put result
        * @param aeadOffset Offset in input buffer up to which do not encrypt data but only authentificate in AEAD mode
        * @return Operation status
        **/
        template <typename ContainerInT, typename ContainerOutT>
        common::Error processAndFinalize(
            const ContainerInT& dataIn,
            ContainerOutT& dataOut,
            size_t offsetIn=0,
            size_t sizeIn=0,
            size_t offsetOut=0
        );

        template <typename ContainerOutT>
        common::Error processAndFinalize(
            const common::SpanBuffer& dataIn,
            ContainerOutT& dataOut,
            size_t offsetOut=0
        );
        template <typename ContainerOutT>
        common::Error processAndFinalize(
            const common::SpanBuffers& dataIn,
            ContainerOutT& dataOut,
            size_t offsetOut=0
        );

        virtual const CryptAlgorithm* getAlg() const=0;

        virtual size_t getMaxPadding() const noexcept
        {
            return 0;
        }

    protected:

        virtual Error canProcessAndFinalize() const noexcept
        {
            return OK;
        }

        /**
         * @brief Actually process data in derived class
         * @param bufIn Input buffer
         * @param sizeIn Size of input data
         * @param bufOut Output buffer
         * @param sizeOut Resulting size
         * @param lastBlock Finalize processing
         * @return Operation status
         */
        virtual common::Error doProcess(
            const char* bufIn,
            size_t sizeIn,
            char* bufOut,
            size_t& sizeOut,
            bool lastBlock
        ) = 0;

        //! Reset worker so that it can be used again with new data
        virtual void doReset() noexcept =0;

        bool m_initialized;
};

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTCIPHERWORKER_H
