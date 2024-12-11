/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/hmac.h
 *
 *      Base class for HMAC (Hash Message Authentication Code) processing
 *
 */
/****************************************************************************/

#ifndef HATNCRYPTHMAC_H
#define HATNCRYPTHMAC_H

#include <hatn/common/bytearray.h>

#include <hatn/crypt/crypt.h>
#include <hatn/crypt/crypterror.h>
#include <hatn/crypt/securekey.h>
#include <hatn/crypt/mac.h>

HATN_CRYPT_NAMESPACE_BEGIN

//! Base class for HMAC (Hash Message Authentication Code) processing
class HMAC : public MAC
{
    public:

        using MAC::MAC;

        /**
         * @brief Calculate HMAC using specified algorithm
         * @param algorithm Algorithm to use
         * @param Key to use
         * @param data Input buffer
         * @param result Result buffer
         * @param offsetIn Offset in input buffer
         * @param sizeIn Size of input data, if zero then sizeof input buffer will be used
         * @param offsetOut Offset in result buffer
         * @return Operation status
         */
        template <typename ContainerInT, typename ContainerOutT>
        static common::Error hmac(
            const CryptAlgorithm* algorithm,
            const MACKey* key,
            const ContainerInT& data,
            ContainerOutT& result,
            size_t offsetIn=0,
            size_t sizeIn=0,
            size_t offsetOut=0
        );

        /**
         * @brief Calculate and check digest
         * @param algorithm Algorithm to use
         * @param Key to use
         * @param data Input buffer
         * @param hash Hash value to compare with
         * @param offsetIn Offset in input buffer
         * @param sizeIn Size of data block in input buffer
         * @param offsetOut Offset in result buffer
         * @return Operation status
         */
        template <typename ContainerInT, typename ContainerHashT>
        static common::Error checkHMAC(
            const CryptAlgorithm* algorithm,
            const MACKey* key,
            const ContainerInT& data,
            const ContainerHashT& hash,
            size_t offsetIn=0,
            size_t sizeIn=0,
            size_t offsetHash=0,
            size_t sizeHash=0
        );

    protected:

        virtual CryptAlgorithm::Type macType() const noexcept override
        {
            return CryptAlgorithm::Type::HMAC;
        }
};

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTHMAC_H
