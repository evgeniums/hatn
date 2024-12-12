/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file crypt/plugins/openssl/opensslutils.h
  *
  *   Utils for OpenSSL plugin
  *
  */

/****************************************************************************/

#ifndef HATNOPENSSLUTILS_H
#define HATNOPENSSLUTILS_H

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>

#include <hatn/common/utils.h>

#include <openssl/bio.h>
#include <openssl/rand.h>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <hatn/crypt/plugins/openssl/opensslerror.h>

HATN_OPENSSL_NAMESPACE_BEGIN

inline Error createMemBio(BIO*& bio, const char* data, size_t size) noexcept
{
    ::ERR_clear_error();

    bio = ::BIO_new(::BIO_s_mem());
    if (bio == NULL)
    {
        return makeLastSslError();
    }
    int ret=BIO_write(bio,data,static_cast<int>(size));
    if (ret == 0)
    {
        ::BIO_free(bio);
        return makeLastSslError();
    }

    return Error();
}

template <typename ContainerT> inline void readMemBio(BIO* bio, ContainerT& container, bool freeBio=true)
{
    char *data = nullptr;
    long size = ::BIO_get_mem_data(bio,&data);
    if (data && (size > 0))
    {
        container.load(data,size);
    }
    if (freeBio)
    {
        ::BIO_free(bio);
    }
}

inline Error genRandData(char* data, size_t size) noexcept
{
    if (size>0)
    {
        ::ERR_clear_error();
        if (::RAND_bytes(reinterpret_cast<unsigned char*>(data),static_cast<int>(size))!=1)
        {
            return makeLastSslError(CryptError::GENERAL_FAIL);
        }
    }
    return Error();
}

template <typename ContainerT, typename SizeT> inline Error genRandData(ContainerT& container, SizeT size) noexcept
{
    container.resize(size);
    return genRandData(container.data(),static_cast<size_t>(size));
}

template <typename ContainerT> void bn2Container(const BIGNUM* bn,ContainerT& container)
{
    container.resize(static_cast<size_t>(BN_num_bytes(bn)));
    ::BN_bn2bin(bn,reinterpret_cast<unsigned char*>(container.data()));
}

template <typename ContainerT> void container2Bn(const ContainerT& container,BIGNUM** bn)
{
    *bn=::BN_bin2bn(reinterpret_cast<unsigned const char*>(container.data()),container.size(),NULL);
}

inline void splitAlgName(const char* name, std::vector<std::string>& parts)
{
    std::string nameStr(name);
    common::Utils::trimSplit(parts,nameStr,'/');
}

HATN_OPENSSL_NAMESPACE_END

#endif // HATNOPENSSLPLUGIN_H
