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

#include <hatn/crypt/mac.h>
#include <hatn/crypt/cryptplugin.h>

namespace hatn {

using namespace common;

namespace crypt {

/*********************** MAC **************************/

//---------------------------------------------------------------
MAC::MAC(const SymmetricKey* key) noexcept : m_key(key)
{
    if (m_key)
    {
        setAlgorithm(m_key->alg());
    }
}

//---------------------------------------------------------------
common::Error MAC::finalizeAndVerify(const char *tag, size_t tagSize)
{
    common::ByteArray calcTag;
    HATN_CHECK_RETURN(finalize(calcTag));
    size_t size=(std::min)(calcTag.size(),tagSize);
    if (alg()->engine()->plugin()->constTimeMemCmp(calcTag.data(),tag,size)!=0)
    {
        return cryptError(CryptError::MAC_FAILED);
    }
    return common::Error();
}

//---------------------------------------------------------------
common::Error MAC::beforeInit() noexcept
{
    if (alg()->type()!=macType())
    {
        return cryptError(CryptError::INVALID_ALGORITHM);
    }
    if (key()==nullptr)
    {
        return cryptError(CryptError::INVALID_MAC_STATE);
    }
    if (!key()->hasRole(SecureKey::Role::MAC))
    {
        return cryptError(CryptError::INVALID_KEY_TYPE);
    }
    return common::Error();
}

//---------------------------------------------------------------
common::Error MAC::finalizeAndVerify(const common::SpanBuffer& tag)
{
    try
    {
        auto view=tag.view();
        return finalizeAndVerify(view);
    }
    catch (const common::ErrorException& e)
    {
        return e.error();
    }
    return common::Error();
}

//---------------------------------------------------------------
HATN_CRYPT_NAMESPACE_END
