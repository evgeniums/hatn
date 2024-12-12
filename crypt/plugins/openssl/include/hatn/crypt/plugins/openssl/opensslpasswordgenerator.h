/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslpasswordgenerator.h
 *
 * 	Password generator with OpenSSL backend
 *
 */
/****************************************************************************/

#ifndef HATNOPENSSLPASSWORDGENERATOR_H
#define HATNOPENSSLPASSWORDGENERATOR_H

#include <hatn/crypt/passwordgenerator.h>

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>
#include <hatn/crypt/plugins/openssl/opensslerror.h>
#include <hatn/crypt/plugins/openssl/opensslutils.h>

namespace hatn {
using namespace common;
namespace crypt {
namespace openssl {

//! Password generator with OpenSSL backend
class OpenSslPasswordGenerator : public PasswordGenerator
{
    public:

        virtual common::Error randBytes(char* data,size_t size) override
        {
            return genRandData(data,size);
        }
};

} // namespace openssl
HATN_CRYPT_NAMESPACE_END

#endif // HATNOPENSSLPASSWORDGENERATOR_H
