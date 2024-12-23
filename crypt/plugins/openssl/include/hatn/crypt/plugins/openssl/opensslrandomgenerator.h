/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslrandomgenerator.h
 *
 * 	Random generator with OpenSSL backend
 *
 */
/****************************************************************************/

#ifndef HATNOPENSSLRANDOMGENERATOR_H
#define HATNOPENSSLRANDOMGENERATOR_H

#include <hatn/crypt/randomgenerator.h>

#include <hatn/crypt/plugins/openssl/opensslplugindef.h>
#include <hatn/crypt/plugins/openssl/opensslerror.h>
#include <hatn/crypt/plugins/openssl/opensslutils.h>

HATN_OPENSSL_NAMESPACE_BEGIN

//! Random generator with OpenSSL backend
class OpenSslRandomGenerator : public RandomGenerator
{
    public:

        virtual common::Error randBytes(char* data,size_t size) const override
        {
            return genRandData(data,size);
        }
};

HATN_OPENSSL_NAMESPACE_END

#endif // HATNOPENSSLRANDOMGENERATOR_H
