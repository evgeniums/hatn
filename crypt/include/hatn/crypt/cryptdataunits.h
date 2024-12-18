/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file crypt/cryptdataunits.h
 *
 *      Definition of DataUnt objects for crypt module
 *
 */
/****************************************************************************/

#ifndef HATNCRYPTDATAUNITS_H
#define HATNCRYPTDATAUNITS_H

#include <hatn/dataunit/syntax.h>
#include <hatn/crypt/crypt.h>

HATN_CRYPT_NAMESPACE_BEGIN

//! @todo Support data compression

HDU_UNIT(cipher_suite,
    HDU_FIELD(id,TYPE_STRING,1)
    HDU_FIELD(cipher,TYPE_STRING,2)
    HDU_FIELD(digest,TYPE_STRING,3)
    HDU_FIELD(aead,TYPE_STRING,4)
    HDU_FIELD(mac,TYPE_STRING,5)
    HDU_FIELD(hkdf,TYPE_STRING,6)
    HDU_FIELD(pbkdf,TYPE_STRING,7)
    HDU_FIELD(signature,TYPE_STRING,8)
    HDU_FIELD(dh,TYPE_STRING,9)
    HDU_FIELD(ecdh,TYPE_STRING,10)
)

constexpr const uint32_t MaxContainerChunkSize=0x40000u;
constexpr const uint32_t MaxContainerFirstChunkSize=0x1000u;

HDU_UNIT(container_descriptor,
    HDU_DEFAULT_FIELD(chunk_max_size,TYPE_UINT32,1,MaxContainerChunkSize)
    HDU_DEFAULT_FIELD(first_chunk_max_size,TYPE_UINT32,2,MaxContainerFirstChunkSize)
    HDU_FIELD(cipher_suite_id,TYPE_STRING,3)
    HDU_FIELD(cipher_suite,cipher_suite::TYPE,4)
    HDU_ENUM(KdfType,HKDF=0,PBKDF=1)
    HDU_DEFAULT_FIELD(kdf_type,HDU_TYPE_ENUM(KdfType),5,KdfType::PBKDF)
    HDU_FIELD(salt,TYPE_BYTES,6)
)

HDU_UNIT(file_stamp,
    HDU_FIELD(digest,TYPE_BYTES,1)
    HDU_FIELD(mac,TYPE_BYTES,2)
)

HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPTDATAUNITS_H
