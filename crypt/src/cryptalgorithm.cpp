/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/cryptalgoritm.cpp
 *
 * 	General descriptor of cryptigraphic algorithm
 *
 */
/****************************************************************************/

#include <hatn/crypt/cryptplugin.h>
#include <hatn/crypt/cryptalgorithm.h>
#include <hatn/crypt/crypterror.h>

namespace hatn {

using namespace common;

namespace crypt {

/*********************** CryptAlgorithm ***********************/

//---------------------------------------------------------------
common::SharedPtr<MACKey> CryptAlgorithm::createMACKey() const
{
    return common::SharedPtr<MACKey>();
}

//---------------------------------------------------------------
common::SharedPtr<SymmetricKey> CryptAlgorithm::createSymmetricKey() const
{
    return common::SharedPtr<SymmetricKey>();
}

//---------------------------------------------------------------
common::SharedPtr<PrivateKey> CryptAlgorithm::createPrivateKey() const
{
    return common::SharedPtr<PrivateKey>();
}

//---------------------------------------------------------------
common::SharedPtr<SignatureVerify> CryptAlgorithm::createSignatureVerify() const
{
    return common::SharedPtr<SignatureVerify>();
}

//---------------------------------------------------------------
common::SharedPtr<SignatureSign> CryptAlgorithm::createSignatureSign() const
{
    return common::SharedPtr<SignatureSign>();
}

//---------------------------------------------------------------
common::SharedPtr<PublicKey> CryptAlgorithm::createPublicKey() const
{

    if (
            engine() && engine()->plugin()
            &&
            (
                isType(Type::SIGNATURE)
                ||
                isType(Type::ECDH)
                ||
                isType(Type::DH)
                ||
                isType(Type::AENCRYPTION)
            )
       )
    {
        return engine()->plugin()->createPublicKey(this);
    }
    return common::SharedPtr<PublicKey>();
}

//---------------------------------------------------------------
HATN_CRYPT_NAMESPACE_END
