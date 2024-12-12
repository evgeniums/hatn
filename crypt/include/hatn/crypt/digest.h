/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/digest.h
 *
 *      Base class for digest/hash processing
 *
 */
/****************************************************************************/

#ifndef HATNCRYPTDIGEST_H
#define HATNCRYPTDIGEST_H

#include <hatn/common/bytearray.h>
#include <hatn/common/spanbuffer.h>
#include <hatn/common/pointerwithinit.h>

#include <hatn/crypt/crypt.h>
#include <hatn/crypt/crypterror.h>
#include <hatn/crypt/cryptalgorithm.h>
#include <hatn/crypt/keycontainer.h>

HATN_CRYPT_NAMESPACE_BEGIN

//! Base class for classes implementing signatures and digests
class HATN_CRYPT_EXPORT DigestBase
{
    public:

        //! Ctor
        DigestBase();

        virtual ~DigestBase()=default;
        DigestBase(const DigestBase&)=delete;
        DigestBase(DigestBase&&) =delete;
        DigestBase& operator=(const DigestBase&)=delete;
        DigestBase& operator=(DigestBase&&) =delete;

        /**
         * @brief Reset digest so that it can be used again with new data
         *
         * @return Operation status
         */
        void reset() noexcept;

        /**
         * @brief Process block of data
         * @param buf Buffer with input data
         * @param size Input size to process
         * @param offset Offset in input buffer for processing
         * @return Operation status
         */
        template <typename ContainerT>
        common::Error process(
            const ContainerT& buf,
            size_t size=0,
            size_t offset=0
        ) noexcept
        {
            if (!m_initialized)
            {
                return cryptError(CryptError::INVALID_DIGEST_STATE);
            }

            if (!checkInContainerSize(buf.size(),offset,size))
            {
                return common::Error(common::CommonError::INVALID_SIZE);
            }
            auto bufIn=buf.data()+offset;

            return doProcess(bufIn,size);
        }

        /**
         * @brief Process last block of data and finalize processing
         * @param result Buffer for output result
         * @param offset Offset in output buffer starting from which to put result
         * @return Operation status
         */
        template <typename ContainerT>
        common::Error finalize(
            ContainerT& result,
            size_t offset=0
        ) noexcept
        {
            if (!m_initialized)
            {
                return cryptError(CryptError::INVALID_DIGEST_STATE);
            }
            m_initialized=false;

            try
            {
                size_t prepareSize=resultSize();
                if (result.size()<(offset+prepareSize))
                {
                    result.resize(offset+prepareSize);
                }
            }
            catch (const common::ErrorException& e)
            {
                return e.error();
            }

            return doFinalize(result.data()+offset);
        }

        /**
        * @brief Process the whole data buffer and finalize result
        * @param dataIn Buffer with input data
        * @param dataOut Buffer for output result
        * @param offsetIn Offset in input buffer for processing
        * @param offsetOut Offset in output buffer starting from which to put result
        * @return Operation status
        **/
        template <typename ContainerInT, typename ContainerOutT>
        common::Error processAndFinalize(
            const ContainerInT& dataIn,
            ContainerOutT& dataOut,
            size_t offsetIn=0,
            size_t sizeIn=0,
            size_t offsetOut=0
        ) noexcept
        {
            if (!checkInContainerSize(dataIn.size(),offsetIn,sizeIn) || !checkOutContainerSize(dataOut.size(),offsetOut))
            {
                return common::Error(common::CommonError::INVALID_SIZE);
            }
            if (!checkEmptyOutContainerResult(sizeIn,dataOut,offsetOut))
            {
                return common::Error();
            }

            HATN_CHECK_RETURN(process(dataIn,sizeIn,offsetIn))

            return finalize(dataOut,offsetOut);
        }

        //! Get size of this digest's hash
        /**
         * @brief Get result size
         * @return Result size
         *
         * @throws common::ErrorException if native alg engine is not set
         */
        virtual size_t resultSize() const =0;

        /**
         * @brief Process data from multiple span buffers
         * @param dataIn Data to process
         * @param offset Offset in the first buffer
         * @param backOffset Offset form the end of the last buffer
         * @return Operation status
         */
        common::Error process(const common::SpanBuffers& dataIn, size_t offset=0,size_t backOffset=0);

        /**
         * @brief Process data from single span buffer
         * @param dataIn Data to process
         * @param offset Offset in the buffer
         * @param backOffset Offset form the end of the buffer
         * @return Operation status
         */
        common::Error process(const common::SpanBuffer& dataIn, size_t offset=0,size_t backOffset=0);

    protected:

        /**
         * @brief Actually process data in derived class
         * @param buf Input buffer
         * @param size Size of input data
         * @return Operation status
         */
        virtual common::Error doProcess(
            const char* buf,
            size_t size
        ) noexcept = 0;

        /**
         * @brief Actually finalize processing and put result to buffer
         * @param buf Output buffer
         * @return Operation status
         */
        virtual common::Error doFinalize(
            char* buf
        ) noexcept = 0;

        /**
         * @brief Init digest
         * @param nativeAlg Digest algoritnm, if null then use previuosly set algorithm
         * @return Operation status
         */
        virtual common::Error doInit() noexcept =0;

        /**
         * @brief Reset digest so that it can be used again with new data
         *
         * @return Operation status
         */
        virtual void doReset() noexcept =0;

        void setInitialized(bool enable) noexcept
        {
            m_initialized=enable;
        }

        bool isInitialized() const noexcept
        {
            return m_initialized;
        }

    private:

        bool m_initialized;
};

//! Base class for digest/hash processing
class HATN_CRYPT_EXPORT Digest : public DigestBase
{
    public:

        using DigestBase::DigestBase;

        /**
         * @brief Initialize digest for reuse
         * @param ref Other digest object to get native algorithm from
         * @return Operation status
         */
        common::Error init(const CryptAlgorithm* algorithm=nullptr) noexcept;

        /**
        * @brief Init, process the whole data buffer and finalize result
        * @param dataIn Buffer with input data
        * @param dataOut Buffer for output result
        * @param offsetIn Offset in input buffer for processing
        * @param offsetOut Offset in output buffer starting from which to put result
        * @return Operation status
        **/
        template <typename ContainerInT, typename ContainerOutT>
        common::Error run(
            const ContainerInT& dataIn,
            ContainerOutT& dataOut,
            size_t offsetIn=0,
            size_t sizeIn=0,
            size_t offsetOut=0
        ) noexcept
        {
            HATN_CHECK_RETURN(init());
            return processAndFinalize(dataIn,dataOut,offsetIn,sizeIn,offsetOut);
        }

        //! Check if native cryptiraphic algorithm is defined
        inline bool isAlgDefined() const noexcept
        {
            return !m_alg.isNull() && m_alg.ptr->isValid();
        }

        /**
         * @brief Get cryptoraphic algorithm
         * @return Pointer to cryptographic algorithm
         */
        inline const CryptAlgorithm* alg() const noexcept
        {
            return m_alg.ptr;
        }

        /**
         * @brief Get algorithm of digest
         * @return Digest
         */
        virtual const CryptAlgorithm* digestAlg() const noexcept
        {
            return alg();
        }

        //! Get size of this digest's hash
        /**
         * @brief Get hash size
         * @return Hash size
         *
         * @throws common::ErrorException if native alg engine is not set
         */
        inline size_t hashSize() const
        {
            return resultSize();
        }

        /**
         * @brief Calculate digest using specified algorithm
         * @param algorithm Algorithm to use
         * @param data Input buffer
         * @param result Result buffer
         * @param offsetIn Offset in input buffer
         * @param sizeIn Size of data block in input buffer
         * @param offsetOut Offset in result buffer
         * @return Operation status
         */
        template <typename ContainerInT, typename ContainerOutT>
        static common::Error digest(
            const CryptAlgorithm* algorithm,
            const ContainerInT& data,
            ContainerOutT& result,
            size_t offsetIn=0,
            size_t sizeIn=0,
            size_t offsetOut=0
        );

        /**
         * @brief Calculate and check digest
         * @param algorithm Algorithm to use
         * @param data Input buffer
         * @param hash Hash value to compare with
         * @param offsetIn Offset in input buffer
         * @param sizeIn Size of data block in input buffer
         * @param offsetOut Offset in result buffer
         * @return Operation status
         */
        template <typename ContainerInT, typename ContainerHashT>
        static common::Error check(
            const CryptAlgorithm* algorithm,
            const ContainerInT& data,
            const ContainerHashT& hash,
            size_t offsetIn=0,
            size_t sizeIn=0,
            size_t offsetHash=0,
            size_t sizeHash=0
        );

        void setAlgorithm(const CryptAlgorithm* algorithm) noexcept
        {
            m_alg.ptr=algorithm;
        }

        void setAlg(const CryptAlgorithm* algorithm) noexcept
        {
            setAlgorithm(algorithm);
        }

        virtual void setDigestAlg(const CryptAlgorithm* algorithm) noexcept
        {
            setAlgorithm(algorithm);
        }

        //! Get size of this digest's hash
        /**
         * @brief Get result size
         * @return Result size
         *
         * @throws common::ErrorException if native alg engine is not set
         */
        virtual size_t resultSize() const;

        /**
         * @brief Calculate and produce hash
         * @param dataIn Input data to calculate digest
         * @param result Result buffers
         * @param resultOffset Offset in result buffer
         * @return Operation status
         *
         * BufferT can be either SpanBuffer or SpanBuffers
         *
         */
        template <typename BufferT,typename ContainerTagT>
        common::Error runFinalize(
            const BufferT& dataIn,
            ContainerTagT& result,
            size_t resultOffset=0
        )
        {
            HATN_CHECK_RETURN(init())
            HATN_CHECK_RETURN(process(dataIn))
            return finalize(result,resultOffset);
        }

        /**
         * @brief Finalize hash calculation and check hash
         * @param digest Digest buffer
         * @param digestSize Digest size. Can be less than actual hash size produced by Digest
         * @return Operation status
         *
         * @attention Do not use it for MAC. Checking is not time constant.
         *
         */
        common::Error finalizeAndCheck(const char* digest, size_t digestSize);

        template <typename ContainerT>
        common::Error finalizeAndCheck(const ContainerT& digest)
        {
            return finalizeAndCheck(digest.data(),digest.size());
        }
        common::Error finalizeAndCheck(const common::SpanBuffer& digest);

        /**
         * @brief Calculate and check digest
         * @param dataIn Input data to calculate digest
         * @param digest Digest to compare with
         * @return Operation status
         *
         * BufferT can be either SpanBuffer or SpanBuffers
         *
         * @attention Do not use it for MAC. Checking is not time constant.
         */
        template <typename BufferT>
        common::Error runCheck(
            const BufferT& dataIn,
            const common::SpanBuffer& digest
        )
        {
            HATN_CHECK_RETURN(init())
            HATN_CHECK_RETURN(process(dataIn))
            return finalizeAndCheck(digest);
        }

        /**
         * @brief Calculate and check digest
         * @param dataIn Input data to calculate digest
         * @param digest Digest to compare with
         * @return Operation status
         *
         * @attention Do not use it for MAC. Checking is not time constant.
         *
         */
        template <typename BufferT, typename ContainerDigestT>
        common::Error runCheck(
            const BufferT& dataIn,
            const ContainerDigestT& digest
        )
        {
            HATN_CHECK_RETURN(init())
            HATN_CHECK_RETURN(process(dataIn))
            return finalizeAndCheck(digest);
        }

        /**
         * @brief Calculate and check digest
         * @param dataIn Input data to calculate digest
         * @param digest Digest to compare with
         * @param Size of digest
         * @return Operation status
         *
         * @attention Do not use it for MAC. Checking is not time constant.
         */
        template <typename BufferT>
        common::Error runCheck(
            const BufferT& dataIn,
            const char* digest,
            size_t digestSize
        )
        {
            HATN_CHECK_RETURN(init())
            HATN_CHECK_RETURN(process(dataIn))
            return finalizeAndCheck(digest,digestSize);
        }

    protected:

        //! Additional initialization step in derived class
        virtual common::Error beforeInit() noexcept;

    private:

        common::ConstPointerWithInit<CryptAlgorithm> m_alg;
};

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTDIGEST_H
