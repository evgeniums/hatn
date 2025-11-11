/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslecdh.h
 *
 * 	Elliptic curves Diffie-Hellmann processing
 *
 */
/****************************************************************************/

#ifndef HATNOPENSSLECDH_H
#define HATNOPENSSLECDH_H

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>

#include <openssl/dh.h>

#include <hatn/common/nativehandler.h>

#include <hatn/crypt/securekey.h>
#include <hatn/crypt/publickey.h>

#include <hatn/crypt/ecdh.h>

#include <hatn/crypt/plugins/openssl/opensslec.h>
#include <hatn/crypt/plugins/openssl/openssldh.h>

HATN_OPENSSL_NAMESPACE_BEGIN

#ifdef _WIN32
    template class HATN_OPENSSL_EXPORT OpenSslDHT<ECDH>;
#endif

class HATN_OPENSSL_EXPORT OpenSslECDH : public OpenSslDHT<ECDH>
{
    public:

        using OpenSslDHT<ECDH>::OpenSslDHT;

        static common::Error findNativeAlgorithm(
            std::shared_ptr<CryptAlgorithm> &alg,
            const char *name,
            CryptEngine* engine
            ) noexcept
        {
            std::vector<std::string> parts;
            splitAlgName(name,parts);

            if (parts.size()<1)
            {
                return cryptError(CryptError::INVALID_ALGORITHM);
            }

            alg=std::make_shared<ECAlg>(engine,CryptAlgorithm::Type::ECDH,name,parts);
            return Error();
        }
};

HATN_OPENSSL_NAMESPACE_END

#endif // HATNOPENSSLECDH_H
