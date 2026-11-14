/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/hmac.h
 *
 *      Base class for MAC (Message Authentication Code) processing
 *
 */
/****************************************************************************/

#ifndef HATNCRYPTMAC_H
#define HATNCRYPTMAC_H

#include <hatn/common/bytearray.h>
#include <hatn/common/spanbuffer.h>

#include <hatn/crypt/crypt.h>
#include <hatn/crypt/crypterror.h>
#include <hatn/crypt/securekey.h>
#include <hatn/crypt/digest.h>

HATN_CRYPT_NAMESPACE_BEGIN

//! Base class for MAC (Message Authentication Code) processing
class HATN_CRYPT_EXPORT MAC : public Digest
{
    public:

        MAC() noexcept :m_key(nullptr)
        {}

        /**
         * @brief Ctor
         * @param key Key to use
         *
         */
        explicit MAC(const SymmetricKey* key) noexcept;

        /**
         * @brief Set MAC key
         * @param key Key
         */
        inline void setKey(const SymmetricKey* key) noexcept
        {
            m_key=key;
        }

        //! Get MAC key
        const SymmetricKey* key() const noexcept
        {
            return m_key;
        }

        /**
         * @brief Set the length of the tag this MAC verifies.
         * @param size Tag length in bytes; 0 (the default) means the whole MAC.
         *
         * The length of a verified tag is decided by the verifier, never by whoever sent the tag: a tag of any
         * other length is refused (see finalizeAndVerify()). A protocol that truncates its tags, for example
         * EncryptMAC with an explicit tag size, sets the size it truncates to.
         */
        inline void setTagSize(size_t size) noexcept
        {
            m_tagSize=size;
        }

        //! Length of the tag this MAC verifies, 0 for the whole MAC.
        size_t tagSize() const noexcept
        {
            return m_tagSize;
        }

        /**
         * @brief Finalize hash calculation and verify MAC
         * @param tag MAC tag buffer
         * @param tagSize MAC tag size
         * @return Operaion status
         *
         * Verification uses safe memory compare operation resistant against timing attacks.
         *
         * The tag must have exactly the verified length: the whole MAC, or the size set with setTagSize().
         * A shorter tag, an empty one included, or a longer one is refused: comparing only the common part of the
         * two would accept every prefix of the right MAC.
         */
        common::Error finalizeAndVerify(const char* tag, size_t tagSize);

        template <typename ContainerT>
        common::Error finalizeAndVerify(const ContainerT& tag)
        {
            return finalizeAndVerify(tag.data(),tag.size());
        }
        common::Error finalizeAndVerify(const common::SpanBuffer& tag);

        /**
         * @brief Calculate and verify MAC
         * @param dataIn Input data to calculate MAC
         * @param tag Tag to compare with
         * @return Operation status
         *
         * BufferT can be either SpanBuffer or SpanBuffers
         * Verification uses safe memory compare operation resistant against timing attacks.
         */
        template <typename BufferT>
        common::Error runVerify(
            const BufferT& dataIn,
            const common::SpanBuffer& tag
        )
        {
            HATN_CHECK_RETURN(init())
            HATN_CHECK_RETURN(process(dataIn))
            return finalizeAndVerify(tag);
        }

        /**
         * @brief Calculate and verify MAC
         * @param dataIn Input data to calculate MAC
         * @param tag Tag to compare with
         * @return Operation status
         *
         * Verification uses safe memory compare operation resistant against timing attacks.
         */
        template <typename BufferT, typename ContainerTagT>
        common::Error runVerify(
            const BufferT& dataIn,
            const ContainerTagT& tag
        )
        {
            HATN_CHECK_RETURN(init())
            HATN_CHECK_RETURN(process(dataIn))
            return finalizeAndVerify(tag);
        }

        /**
         * @brief Calculate and verify MAC
         * @param dataIn Input data to calculate MAC
         * @param tag Tag to compare with
         * @param Size of tag
         * @return Operation status
         *
         * Verification uses safe memory compare operation resistant against timing attacks.
         */
        template <typename BufferT>
        common::Error runVerify(
            const BufferT& dataIn,
            const char* tag,
            size_t tagSize
        )
        {
            HATN_CHECK_RETURN(init())
            HATN_CHECK_RETURN(process(dataIn))
            return finalizeAndVerify(tag,tagSize);
        }

        /**
         * @brief Calculate and produce MAC
         * @param dataIn Input data to calculate MAC
         * @param tag Tag buffer to put MAC result to
         * @param tagOffset Offset in tag buffer
         * @return Operation status
         *
         * BufferT can be either SpanBuffer or SpanBuffers
         *
         */
        template <typename BufferT,typename ContainerTagT>
        common::Error runSign(
            const BufferT& dataIn,
            ContainerTagT& tag,
            size_t tagOffset=0
        )
        {
            return runFinalize(dataIn,tag,tagOffset);
        }

    protected:

        //! Additional initialization step in derived class
        virtual common::Error beforeInit() noexcept override;

        virtual CryptAlgorithm::Type macType() const noexcept
        {
            return CryptAlgorithm::Type::MAC;
        }

    private:

        const SymmetricKey* m_key;
        size_t m_tagSize=0;
};

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTMAC_H
