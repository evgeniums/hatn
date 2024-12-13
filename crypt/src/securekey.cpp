/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/securekey.cpp
 *
 * 	Base classes for secure keys
 *
 */
/****************************************************************************/

#include <hatn/crypt/keyprotector.h>
#include <hatn/crypt/cryptplugin.h>
#include <hatn/crypt/securekey.h>

HATN_CRYPT_NAMESPACE_BEGIN

/*********************** SecureKey ***********************/

//---------------------------------------------------------------
common::Error SecureKey::doExportToBuf(common::MemoryLockedArray& buf,
                         ContainerFormat format,
                         bool unprotected
                    ) const
{
    if (!unprotected)
    {
        if (format!=ContainerFormat::RAW_ENCRYPTED)
        {
            return cryptError(CryptError::INVALID_CONTENT_FORMAT);
        }
        if (!isContentProtected())
        {
            if (protector()==nullptr)
            {
                return cryptError(CryptError::INVALID_KEY_PROTECTION);
            }
            return protector()->pack(content(),buf);
        }
        else
        {
            buf.load(content());
        }
    }
    else
    {
        if (format!=ContainerFormat::RAW_PLAIN)
        {
            return cryptError(CryptError::INVALID_CONTENT_FORMAT);
        }
        if (isContentProtected())
        {
            if (protector()==nullptr)
            {
                return cryptError(CryptError::INVALID_KEY_PROTECTION);
            }
            return protector()->unpack(content(),buf);
        }
        else
        {
            buf.load(content());
        }
    }
    return common::Error();
}

//---------------------------------------------------------------
common::Error SecureKey::doImportFromBuf(const char *buf, size_t size, ContainerFormat format, bool keepContent)
{
    std::ignore=keepContent;
    if (format==ContainerFormat::RAW_ENCRYPTED)
    {
        if (protector()==nullptr)
        {
            return cryptError(CryptError::INVALID_KEY_PROTECTION);
        }

        HATN_CHECK_RETURN(protector()->unpack(common::ConstDataBuf(buf,size),content()));
    }
    else if (format==ContainerFormat::RAW_PLAIN)
    {
        setFormat(format);
        setContentProtected(false);

        content().load(buf,size);
    }
    else
    {
        return cryptError(CryptError::INVALID_CONTENT_FORMAT);
    }
    return common::Error();
}

//---------------------------------------------------------------
common::Error SecureKey::importFromFile(const char* filename, ContainerFormat bufFormat, bool keepContent)
{
    common::ByteArray tmpBuf;
    HATN_CHECK_RETURN(tmpBuf.loadFromFile(filename))
    return importFromBuf(tmpBuf,bufFormat,keepContent);
}

//---------------------------------------------------------------
common::Error SecureKey::exportToFile(const char* filename, ContainerFormat format, bool unprotected) const
{
    common::MemoryLockedArray tmpBuf;
    HATN_CHECK_RETURN(exportToBuf(tmpBuf,format,unprotected));
    return tmpBuf.saveToFile(filename);
}

/*********************** SymmetricKey ***********************/

//---------------------------------------------------------------
common::Error SymmetricKey::doGenerate()
{
    content().clear();
    setContentProtected(false);
    setFormat(ContainerFormat::RAW_PLAIN);

    return alg()->engine()->plugin()->randContainer(content(),alg()->keySize());
}

//---------------------------------------------------------------
HATN_CRYPT_NAMESPACE_END
