/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslx509certificate.cpp
 * 	X.509 certificate parser
 */
/****************************************************************************/

#include <iostream>

#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include <hatn/crypt/plugins/openssl/opensslerror.h>
#include <hatn/crypt/plugins/openssl/opensslutils.h>
#include <hatn/crypt/plugins/openssl/opensslprivatekey.h>
#include <hatn/crypt/plugins/openssl/opensslpublickey.h>

#include <hatn/crypt/plugins/openssl/opensslx509chain.h>
#include <hatn/crypt/plugins/openssl/opensslx509certificatestore.h>
#include <hatn/crypt/plugins/openssl/opensslx509.h>

//! \note Do not move this header upper, otherwise there are some conflicts of types on Windows platform
#include <hatn/common/makeshared.h>

#ifdef _WIN32
#define timegm _mkgmtime
#endif

HATN_OPENSSL_NAMESPACE_BEGIN
HATN_COMMON_USING

/******************* OpenSslX509 ********************/

//---------------------------------------------------------------
Error OpenSslX509::parse(
        Native& native,
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
    HATN_CHECK_RETURN(createMemBio(bio,data,size))

    if (format==ContainerFormat::DER)
    {
        native.handler=::d2i_X509_bio(bio,NULL);
    }
    else
    {
        native.handler=::PEM_read_bio_X509(bio,NULL,passwdCallback,passwdCallbackUserdata);
    }
    if (native.isNull())
    {
        BIO_free(bio);
        return makeLastSslError();
    }
    BIO_free(bio);
    return Error();
}

//---------------------------------------------------------------
Error OpenSslX509::serialize(const Native& native, ByteArray &content, ContainerFormat format)
{
    content.clear();
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
        ret=::i2d_X509_bio(bio,native.handler);
    }
    else
    {
        ret=::PEM_write_bio_X509(bio,native.handler);
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
bool OpenSslX509::isNativeNull() const
{
    return !isValid();
}

//---------------------------------------------------------------
inline Error OpenSslX509::checkValidOrParse() const
{
    if (!isValid())
    {
        return const_cast<OpenSslX509*>(this)->parse();
    }
    return Error();
}

//---------------------------------------------------------------
Error OpenSslX509::checkPrivateKey(const PrivateKey &key) const noexcept
{
    auto pkey=dynamic_cast<const OpenSslPrivateKey*>(&key);
    if (!pkey)
    {
        return cryptError(CryptError::INVALID_KEY_TYPE);
    }

    ::ERR_clear_error();

    HATN_CHECK_RETURN(checkValidOrParse())

    if (X509_check_private_key(nativeHandler().handler,pkey->nativeHandler().handler)!=1)
    {
        return makeLastSslError(CryptError::KEYS_MISMATCH);
    }

    return Error();
}

//---------------------------------------------------------------
Error OpenSslX509::checkIssuedBy(const X509Certificate &ca) const noexcept
{
    auto nCA=dynamic_cast<const OpenSslX509*>(&ca);
    if (!nCA)
    {
        return Error(CommonError::INVALID_ARGUMENT);
    }

    ::ERR_clear_error();

    HATN_CHECK_RETURN(checkValidOrParse())

    auto ret=::X509_check_issued(nCA->nativeHandler().handler,nativeHandler().handler);
    if (ret!=X509_V_OK)
    {
        return makeSslError(ret);
    }

    return Error();
}

//---------------------------------------------------------------
template <typename GetterTraits, typename ParameterT=int>
typename GetterTraits::type getField(const OpenSslX509* crt, Error *error, ParameterT param=ParameterT())
{
    Error ecTmp;
    Error* ecPtr=&ecTmp;
    if (error!=nullptr)
    {
        ecPtr=error;
    }
    auto& ec=*ecPtr;
    ::ERR_clear_error();

    try
    {
        ec=crt->checkValidOrParse();
        if (ec)
        {
            throw ErrorException(ec);
        }
        return GetterTraits::get(crt->nativeHandler().handler,param);
    }
    catch (const ErrorException&)
    {
        if (error==nullptr)
        {
            throw;
        }
    }

    return typename GetterTraits::type();
}

namespace
{
struct VersionTraits
{
    using type=size_t;
    static size_t get(const X509* crt,int)
    {
        auto version=::X509_get_version(crt);
        if (version<=0)
        {
            throw ErrorException(makeLastSslError(CryptError::GENERAL_FAIL));
        }
        return static_cast<size_t>(version);
    }
};

struct SerialTraits
{
    using type=FixedByteArray20;
    static type get(const X509* crt,int)
    {
        auto ret=::X509_get0_serialNumber(crt);
        if (ret==nullptr)
        {
            throw ErrorException(makeLastSslError(CryptError::GENERAL_FAIL));
        }
        auto bn=ASN1_INTEGER_to_BN(ret,NULL);
        if (bn==nullptr)
        {
            throw ErrorException(makeLastSslError(CryptError::GENERAL_FAIL));
        }

        type result;
        result.resize(20);
        bn2Container(bn,result);
        BN_free(bn);

        return result;
    }
};

struct NotBefore
{
    static const ASN1_TIME* get(const X509* crt)
    {
        return ::X509_get0_notBefore(crt);
    }
};
struct NotAfter
{
    static const ASN1_TIME* get(const X509* crt)
    {
        return ::X509_get0_notAfter(crt);
    }
};

template <typename TimeField>
struct TimeTraits
{
    using type=X509Certificate::TimePoint;
    static type get(const X509* crt,int)
    {
        const ASN1_TIME* time=TimeField::get(crt);
        if (time==nullptr)
        {
            throw ErrorException(makeLastSslError(CryptError::GENERAL_FAIL));
        }

        std::tm timeinfo;
        if (::ASN1_TIME_to_tm(time,&timeinfo)!=1)
        {
            throw ErrorException(makeLastSslError(CryptError::GENERAL_FAIL));
        }
        return std::chrono::system_clock::from_time_t(timegm(&timeinfo));
    }
};

struct PublicKeyTraits
{
    using type=common::SharedPtr<PublicKey>;
    static type get(const X509* crt,int)
    {
        auto key=::X509_get_pubkey(const_cast<X509*>(crt));
        if (key==nullptr)
        {
            throw ErrorException(makeLastSslError(CryptError::GENERAL_FAIL));
        }
        auto result=makeShared<OpenSslPublicKey>();
        result->setNativeHandler(key);
        return result;
    }
};

}

//---------------------------------------------------------------
size_t OpenSslX509::version(Error *error) const
{
    return getField<VersionTraits>(this,error);
}

//---------------------------------------------------------------
FixedByteArray20 OpenSslX509::serial(Error *error) const
{
    return getField<SerialTraits>(this,error);
}

//---------------------------------------------------------------
X509Certificate::TimePoint OpenSslX509::validNotBefore(Error *error) const
{
    return getField<TimeTraits<NotBefore>>(this,error);
}

//---------------------------------------------------------------
X509Certificate::TimePoint OpenSslX509::validNotAfter(Error *error) const
{
    return getField<TimeTraits<NotAfter>>(this,error);
}

//---------------------------------------------------------------
common::SharedPtr<PublicKey> OpenSslX509::publicKey(Error *error) const
{
    return getField<PublicKeyTraits>(this,error);
}

namespace
{
constexpr int StandardNameFieldMapping(X509Certificate::BasicNameField field)
{
    int NID=-1;
    switch (field)
    {
        case (X509Certificate::BasicNameField::CommonName):
        {
            NID=NID_commonName;
        }
            break;
        case (X509Certificate::BasicNameField::Country):
        {
            NID=NID_countryName;
        }
            break;
        case (X509Certificate::BasicNameField::Organization):
        {
            NID=NID_organizationName;
        }
            break;
        case (X509Certificate::BasicNameField::Locality):
        {
            NID=NID_localityName;
        }
            break;
        case (X509Certificate::BasicNameField::OrganizationUnit):
        {
            NID=NID_organizationalUnitName;
        }
            break;
        case (X509Certificate::BasicNameField::StateOrProvince):
        {
            NID=NID_stateOrProvinceName;
        }
            break;
        case (X509Certificate::BasicNameField::EmailAddress):
        {
            NID=NID_pkcs9_emailAddress;
        }
            break;

    }
    return NID;
}

struct SubjectName
{
    static X509_NAME* get(const X509* crt)
    {
        return ::X509_get_subject_name(crt);
    }
};
struct IssuerName
{
    static X509_NAME* get(const X509* crt)
    {
        return ::X509_get_issuer_name(crt);
    }
};

template <typename NameGetter>
struct NameTraits
{
    using type=std::string;
    static type get(const X509* crt,X509Certificate::BasicNameField field)
    {
        auto name=NameGetter::get(crt);
        if (name==nullptr)
        {
            throw ErrorException(makeLastSslError(CryptError::GENERAL_FAIL));
        }

        auto index=::X509_NAME_get_index_by_NID(name,StandardNameFieldMapping(field),-1);
        if (index<0)
        {
            throw ErrorException(makeLastSslError(CryptError::GENERAL_FAIL));
        }

        auto entry=::X509_NAME_get_entry(name,index);
        if (entry==nullptr)
        {
            throw ErrorException(makeLastSslError(CryptError::GENERAL_FAIL));
        }

        auto asn1Str=::X509_NAME_ENTRY_get_data(entry);
        if (asn1Str==nullptr)
        {
            throw ErrorException(makeLastSslError(CryptError::GENERAL_FAIL));
        }

        unsigned char* cStr=nullptr;
        if (::ASN1_STRING_to_UTF8(&cStr,asn1Str)<=0)
        {
            throw ErrorException(makeLastSslError(CryptError::GENERAL_FAIL));
        }

        std::string result(reinterpret_cast<char*>(cStr));
        OPENSSL_free(cStr);

        return result;
    }
};

constexpr int AltNameFieldMapping(X509Certificate::AltNameType type)
{
    int NID=-1;
    switch (type)
    {
        case (X509Certificate::AltNameType::DNS):
        {
            NID=GEN_DNS;
        }
            break;
        case (X509Certificate::AltNameType::Email):
        {
            NID=GEN_EMAIL;
        }
            break;
        case (X509Certificate::AltNameType::URI):
        {
            NID=GEN_URI;
        }
            break;
        case (X509Certificate::AltNameType::IP):
        {
            NID=GEN_IPADD;
        }
            break;
    }
    return NID;
}

struct AltNameTraits
{
    using type=std::vector<std::string>;
    static type get(const X509* crt,const std::pair<X509Certificate::AltNameType,int>& field)
    {
        std::vector<std::string> result;

        STACK_OF(GENERAL_NAME)* sanNames=reinterpret_cast<STACK_OF(GENERAL_NAME)*>(::X509_get_ext_d2i(crt,field.second, NULL, NULL));
        if (sanNames==nullptr)
        {
            throw ErrorException(makeLastSslError(CryptError::GENERAL_FAIL));
        }

        auto nativeType=AltNameFieldMapping(field.first);
        auto namesCount=sk_GENERAL_NAME_num(sanNames);
        for (int i=0;i<namesCount;i++)
        {
            const GENERAL_NAME *currentName = sk_GENERAL_NAME_value(sanNames,i);
            if (nativeType==currentName->type)
            {
                if (field.first!=X509Certificate::AltNameType::IP)
                {
                    // DNS, URI and email are regarded as UTF strings
                    auto* asn1Str=currentName->d.dNSName;
                    unsigned char* cStr=nullptr;
                    if (::ASN1_STRING_to_UTF8(&cStr,asn1Str)<=0)
                    {
                        sk_GENERAL_NAME_pop_free(sanNames, GENERAL_NAME_free);
                        throw ErrorException(makeLastSslError(CryptError::GENERAL_FAIL));
                    }
                    result.emplace_back(reinterpret_cast<char*>(cStr));
                    OPENSSL_free(cStr);
                }
                else
                {
                    if (currentName->d.ip->length==4)
                    {
                        // IPv4
                        std::string ip=fmt::format("{}.{}.{}.{}",currentName->d.ip->data[0],
                                                                 currentName->d.ip->data[1],
                                                                 currentName->d.ip->data[2],
                                                                 currentName->d.ip->data[3]
                                                  );
                        result.push_back(std::move(ip));
                    }
                    else if (currentName->d.ip->length==16)
                    {
                        // IPv6
                        std::string ip=fmt::format("{:02x}{:02x}:{:02x}{:02x}:{:02x}{:02x}:{:02x}{:02x}:{:02x}{:02x}:{:02x}{:02x}:{:02x}{:02x}:{:02x}{:02x}",
                                                                    currentName->d.ip->data[0],
                                                                    currentName->d.ip->data[1],
                                                                    currentName->d.ip->data[2],
                                                                    currentName->d.ip->data[3],
                                                                    currentName->d.ip->data[4],
                                                                    currentName->d.ip->data[5],
                                                                    currentName->d.ip->data[6],
                                                                    currentName->d.ip->data[7],
                                                                    currentName->d.ip->data[8],
                                                                    currentName->d.ip->data[9],
                                                                    currentName->d.ip->data[10],
                                                                    currentName->d.ip->data[11],
                                                                    currentName->d.ip->data[12],
                                                                    currentName->d.ip->data[13],
                                                                    currentName->d.ip->data[14],
                                                                    currentName->d.ip->data[15]
                                                  );
                        result.push_back(std::move(ip));
                    }
                }
            }
        }

        ::sk_GENERAL_NAME_pop_free(sanNames, GENERAL_NAME_free);

        return result;
    }
};

}

//---------------------------------------------------------------
std::string OpenSslX509::subjectNameField(X509Certificate::BasicNameField field, Error *error) const
{
    return getField<NameTraits<SubjectName>>(this,error,field);
}

//---------------------------------------------------------------
std::string OpenSslX509::issuerNameField(X509Certificate::BasicNameField field, Error *error) const
{
    return getField<NameTraits<IssuerName>>(this,error,field);
}

//---------------------------------------------------------------
std::vector<std::string> OpenSslX509::subjectAltNames(X509Certificate::AltNameType type, Error *error) const
{
    return getField<AltNameTraits>(this,error,std::make_pair(type,NID_subject_alt_name));
}

//---------------------------------------------------------------
std::vector<std::string> OpenSslX509::issuerAltNames(X509Certificate::AltNameType type, Error *error) const
{
    return getField<AltNameTraits>(this,error,std::make_pair(type,NID_issuer_alt_name));
}

namespace
{

struct VerifyStruct
{
    std::function<void (Error&&)> handler;
    bool processed;
};

//---------------------------------------------------------------
int verifyCb(int preverifyOk, X509_STORE_CTX *x509Ctx)
{
    if (preverifyOk==1)
    {
        return preverifyOk;
    }
    if (x509Ctx)
    {
        auto verifyObj=reinterpret_cast<VerifyStruct*>(::X509_STORE_CTX_get_app_data(x509Ctx));
        if (!verifyObj->processed)
        {
            auto code=::X509_STORE_CTX_get_error(x509Ctx);
            auto nativeCrt=::X509_STORE_CTX_get_current_cert(x509Ctx);
            auto cert=makeShared<OpenSslX509>();
            ::X509_up_ref(nativeCrt);
            cert->setNativeHandler(nativeCrt);

            verifyObj->processed=true;
            verifyObj->handler(makeSslError(code,std::move(cert)));
        }
    }
    return preverifyOk;
}

}

//---------------------------------------------------------------
Error OpenSslX509::verify(const X509Certificate &otherCert) const noexcept
{
    auto rootCrt=dynamic_cast<const OpenSslX509*>(&otherCert);
    if (rootCrt==nullptr)
    {
        return cryptError(CryptError::INVALID_OBJECT_TYPE);
    }

    NativeHandler<X509_STORE,detail::X509StoreTraits> trustedStore(::X509_STORE_new());
    if (trustedStore.isNull())
    {
        return makeLastSslError(CryptError::GENERAL_FAIL);
    }
    NativeHandler<X509_STORE_CTX,detail::X509StoreCtxTraits> ctx(::X509_STORE_CTX_new());
    if (ctx.isNull())
    {
        return makeLastSslError(CryptError::GENERAL_FAIL);
    }

    if (::X509_STORE_add_cert(trustedStore.handler,rootCrt->nativeHandler().handler)!=1)
    {
        return makeLastSslError(CryptError::X509_STORE_FAILED);
    }

    if (::X509_STORE_CTX_init(ctx.handler,trustedStore.handler,nativeHandler().handler,NULL)!=1)
    {
        return makeLastSslError(CryptError::X509_STORE_FAILED);
    }
    auto params=::X509_STORE_CTX_get0_param(ctx.handler);
    if (::X509_VERIFY_PARAM_set_flags(params,X509_V_FLAG_PARTIAL_CHAIN)!=1)
    {
        return makeLastSslError(CryptError::X509_STORE_FAILED);
    }

    Error ec;
    auto cb=[&ec](Error&& e)
    {
        ec=std::move(e);
    };
    VerifyStruct verifyObj{cb,false};
    ::X509_STORE_CTX_set_app_data(ctx.handler,&verifyObj);
    ::X509_STORE_CTX_set_verify_cb(ctx.handler,verifyCb);
    if (::X509_verify_cert(ctx.handler)!=1)
    {
        return ec;
    }

    return Error();
}

//---------------------------------------------------------------
Error OpenSslX509::verify(
        const X509CertificateStore& store,
        const X509CertificateChain* chain
    ) const noexcept
{
    auto nativeStore=dynamic_cast<const OpenSslX509CertificateStore*>(&store);
    if (nativeStore==nullptr)
    {
        return cryptError(CryptError::INVALID_OBJECT_TYPE);
    }
    const OpenSslX509Chain* nativeChain=nullptr;
    X509CrtStack* sk=NULL;
    if (chain!=nullptr)
    {
        nativeChain=dynamic_cast<const OpenSslX509Chain*>(chain);
        if (nativeChain==nullptr)
        {
            return cryptError(CryptError::INVALID_OBJECT_TYPE);
        }
        sk=nativeChain->nativeHandler().handler;
    }

    NativeHandler<X509_STORE_CTX,detail::X509StoreCtxTraits> ctx(::X509_STORE_CTX_new());
    if (ctx.isNull())
    {
        return makeLastSslError(CryptError::GENERAL_FAIL);
    }

    if (::X509_STORE_CTX_init(ctx.handler,nativeStore->nativeHandler().handler,nativeHandler().handler,sk)!=1)
    {
        return makeLastSslError(CryptError::X509_STORE_FAILED);
    }

    Error ec;
    auto cb=[&ec](Error&& e)
    {
        ec=std::move(e);
    };
    VerifyStruct verifyObj{cb,false};
    ::X509_STORE_CTX_set_app_data(ctx.handler,&verifyObj);
    ::X509_STORE_CTX_set_verify_cb(ctx.handler,verifyCb);
    if (::X509_verify_cert(ctx.handler)!=1)
    {
        return ec;
    }

    return Error();
}

//---------------------------------------------------------------

HATN_OPENSSL_NAMESPACE_END
