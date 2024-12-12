/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslprivatekey.ipp
 *
 * 	Private key wrapper with OpenSSL backend
 *
 */
/****************************************************************************/

#include <hatn/crypt/keyprotector.h>

#include <hatn/crypt/plugins/openssl/opensslerror.h>
#include <hatn/crypt/plugins/openssl/opensslutils.h>

#include <hatn/crypt/plugins/openssl/opensslmac.h>
#include <hatn/crypt/plugins/openssl/opensslprivatekey.h>

HATN_OPENSSL_NAMESPACE_BEGIN

/******************* OpenSslPKey ********************/

//---------------------------------------------------------------
template <typename BaseT>
Error OpenSslPKey<BaseT>::parseContent(
        typename BaseTmplT::Native& native,
        const char *data,
        size_t size,
        ContainerFormat format,
        pem_password_cb *passwdCallback,
        void *passwdCallbackUserdata
    ) noexcept
{
    native.reset();

    if (size==0)
    {
        return Error();
    }

    BIO *bio;
    HATN_CHECK_RETURN(createMemBio(bio,data,size));

    switch (format)
    {
        case (ContainerFormat::PEM):
        {
            native.handler=::PEM_read_bio_PrivateKey(bio,NULL,passwdCallback,passwdCallbackUserdata);
        }
            break;

        case (ContainerFormat::DER):
        {
            native.handler=::d2i_PrivateKey_bio(bio,NULL);
        }
            break;

        default:
        {
            break;
        }
    }

    if (native.isNull())
    {
        BIO_free(bio);
        return makeLastSslError(CryptError::INVALID_CONTENT_FORMAT);
    }
    BIO_free(bio);
    return Error();
}

//---------------------------------------------------------------
template <typename KeyT>
static int protectorPasswordCb(char *buf, int size, int rwflag, void *userdata)
{
    std::ignore=rwflag;

    KeyT* key=reinterpret_cast<KeyT*>(userdata);

    size=static_cast<int>(key->protector()->passphrase()->rawSize());
    memcpy(buf,key->protector()->passphrase()->rawData(),size);

    return size;
}

//---------------------------------------------------------------
template <typename BaseT>
Error OpenSslPKey<BaseT>::serialize(
        const typename BaseTmplT::Native &native,
        common::MemoryLockedArray &content,
        const EVP_CIPHER *enc,
        ContainerFormat format,
        const char *passPhrase,
        size_t psLength,
        pem_password_cb *passwdCallback,
        void *passwdCallbackUserdata
    )
{
    if (native.isNull())
    {
        return Error();
    }

    ::ERR_clear_error();
    BIO *bio = ::BIO_new(::BIO_s_mem());
    if (bio==nullptr)
    {
        return makeLastSslError();
    }

    int ret=0;
    if (format==ContainerFormat::DER)
    {
        ret=::i2d_PrivateKey_bio(bio,native.handler);
    }
    else
    {
        ret=::PEM_write_bio_PrivateKey(bio,native.handler,enc,
                                       reinterpret_cast<unsigned char*>(const_cast<char*>(passPhrase)),
                                       static_cast<int>(psLength),
                                       passwdCallback,passwdCallbackUserdata);
    }
    if (ret!=1)
    {
        BIO_free(bio);
        return makeLastSslError();
    }

    readMemBio(bio,content);
    return Error();
}

//---------------------------------------------------------------
template <typename BaseT>
Error OpenSslPKey<BaseT>::doImportFromBuf(const char *buf, size_t size, ContainerFormat format, bool keepContent)
{
    if (format==ContainerFormat::PEM || format==ContainerFormat::DER)
    {
        pem_password_cb* cb=nullptr;
        void* udata=nullptr;
        if (
                format==ContainerFormat::PEM
                && this->protector()!=nullptr
                && this->protector()->isPassphraseValid()
            )
        {
            cb=protectorPasswordCb<OpenSslPKey<BaseT>>;
            udata=this;
        }
        HATN_CHECK_RETURN(parseContent(this->nativeHandler(),buf,size,format,cb,udata));

        if (keepContent)
        {
            this->loadContent(buf,size);
            this->setFormat(format);
        }
    }
    else if (format==ContainerFormat::RAW_PLAIN || format==ContainerFormat::RAW_ENCRYPTED)
    {
        ENGINE* engine=nullptr;
        if (this->alg()->engine())
        {
            engine=this->alg()->engine()->template nativeHandler<ENGINE>();
        }

        common::MemoryLockedArray unpacked;

        if (format==ContainerFormat::RAW_ENCRYPTED)
        {
            if (this->protector()==nullptr)
            {
                return cryptError(CryptError::INVALID_KEY_PROTECTION);
            }

            HATN_CHECK_RETURN(this->protector()->unpack(common::ConstDataBuf(buf,size),unpacked));
            buf=unpacked.data();
            size=unpacked.size();
        }

        if (nativeType()==EVP_PKEY_CMAC)
        {
            //! @todo Check if this format is still actual
#if 0
            this->nativeHandler().handler=::EVP_PKEY_new_CMAC_key(engine,
                                                                 reinterpret_cast<const unsigned char*>(buf),
                                                                 size,EVP_aes_128_cbc());
            // keep CMAC key in content because there is no way to get raw data later with EVP_PKEY_get_raw_private_key
            this->loadContent(buf,size);
            this->setFormat(ContainerFormat::RAW_PLAIN);
            keepContent=false;
#endif
            return cryptError(CryptError::UNSUPPORTED_KEY_FORMAT);
        }
        else
        {
            this->nativeHandler().handler=::EVP_PKEY_new_raw_private_key(nativeType(),engine,
                                                                 reinterpret_cast<const unsigned char*>(buf),
                                                                 size);
        }

        if (this->nativeHandler().isNull())
        {
            return makeLastSslError(CryptError::KEY_INITIALIZATION_FAILED);
        }

        if (keepContent)
        {
            this->loadContent(buf,size);
            this->setFormat(format);
        }
    }
    else
    {
        return cryptError(CryptError::INVALID_CONTENT_FORMAT);
    }
    return common::Error();
}

//---------------------------------------------------------------
template <typename BaseT>
Error OpenSslPKey<BaseT>::doExportToBuf(common::MemoryLockedArray &buf, ContainerFormat format, bool unprotected) const
{
    if (format==ContainerFormat::PEM || format==ContainerFormat::DER)
    {
        const char *passPhrase=nullptr;
        size_t psLength=0;
        const EVP_CIPHER* cipher=nullptr;

        if (!unprotected)
        {
            // only PEM format can be encrypted, DER can not
            if (format!=ContainerFormat::PEM)
            {
                return cryptError(CryptError::INVALID_CONTENT_FORMAT);
            }

            // check for PEM encryption parameters
            if (
                    this->protector()==nullptr
                    || !this->protector()->isPassphraseValid()
                )
            {
                return cryptError(CryptError::INVALID_KEY_PROTECTION);
            }

            auto cipherSuite=this->protector()->cryptContainer().cipherSuite();
            if (cipherSuite==nullptr)
            {
                return cryptError(CryptError::INVALID_KEY_PROTECTION);
            }
            const CryptAlgorithm* cipherAlg=nullptr;
            HATN_CHECK_RETURN(cipherSuite->cipherAlgorithm(cipherAlg))
            cipher=cipherAlg->template nativeHandler<EVP_CIPHER>();
            if (cipher==nullptr)
            {
                return cryptError(CryptError::INVALID_KEY_PROTECTION);
            }
            passPhrase=this->protector()->passphrase()->rawData();
            psLength=this->protector()->passphrase()->rawSize();
        }
        return serialize(this->nativeHandler(),
                         buf,
                         cipher,
                         format,
                         passPhrase,
                         psLength
                         );

    }
    else if (format==ContainerFormat::RAW_PLAIN || format==ContainerFormat::RAW_ENCRYPTED)
    {
        common::MemoryLockedArray* exportBuf=&buf;
        common::MemoryLockedArray tmpBuf;
        if (format==ContainerFormat::RAW_ENCRYPTED)
        {
            if (this->protector()==nullptr)
            {
                return cryptError(CryptError::INVALID_KEY_PROTECTION);
            }
            exportBuf=&tmpBuf;
        }

        if (nativeType()==EVP_PKEY_CMAC)
        {
            // CMAC is a special case, it can be exported only from content()
            if (this->format()==ContainerFormat::RAW_PLAIN)
            {
                // proceed with normal exporting
                exportBuf->load(this->content().data(),this->content().size());
            }
            else if (this->format()==ContainerFormat::RAW_ENCRYPTED && format==ContainerFormat::RAW_ENCRYPTED)
            {
                // export encrypted as is
                buf.load(this->content().data(),this->content().size());
                return Error();
            }
        }
        else
        {
            size_t len=0;
            if (::EVP_PKEY_get_raw_private_key(this->nativeHandler().handler,NULL,&len)!=1)
            {
                return makeLastSslError(CryptError::EXPORT_KEY_FAILED);
            }
            if (len>0)
            {
                exportBuf->resize(len);
                if (::EVP_PKEY_get_raw_private_key(this->nativeHandler().handler,reinterpret_cast<unsigned char*>(exportBuf->data()),&len)!=1)
                {
                    return makeLastSslError(CryptError::EXPORT_KEY_FAILED);
                }
            }
            else
            {
                exportBuf->clear();
            }
        }

        if (format==ContainerFormat::RAW_ENCRYPTED)
        {
            HATN_CHECK_RETURN(this->protector()->pack(*exportBuf,buf));
        }

        return Error();
    }
    return cryptError(CryptError::INVALID_CONTENT_FORMAT);
}

//---------------------------------------------------------------
template <typename BaseT>
Error OpenSslPKey<BaseT>::parseContent(
    pem_password_cb* passwdCallback,
    void* passwdCallbackUserdata
) noexcept
{
    return parseContent(this->nativeHandler(),this->content().data(),this->content().size(),this->format(),passwdCallback,passwdCallbackUserdata);
}

//---------------------------------------------------------------
template <typename BaseT>
Error OpenSslPKey<BaseT>::doGenerate()
{
    HATN_CHECK_RETURN(BaseTmplT::doGenerate())
    HATN_CHECK_RETURN(this->importFromBuf(this->content(),ContainerFormat::RAW_PLAIN))
    this->content().clear();
    return Error();
}

//---------------------------------------------------------------

HATN_OPENSSL_NAMESPACE_END
