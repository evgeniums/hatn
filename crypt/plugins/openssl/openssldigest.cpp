/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/openssldigest.cpp
 *
 * 	Digest/hash implementation with OpenSSL backend
 *
 */
/****************************************************************************/

#include <openssl/evp.h>
#include <openssl/objects.h>

#include <hatn/crypt/plugins/openssl/opensslutils.h>
#include <hatn/crypt/plugins/openssl/openssldigest.h>

HATN_OPENSSL_NAMESPACE_BEGIN

/******************* OpenSslDigest ********************/

static void eachObj(const OBJ_NAME *obj, void *arg)
{
    auto result=reinterpret_cast<std::vector<std::string>*>(arg);
    result->push_back(obj->name);
}

//---------------------------------------------------------------
std::vector<std::string> OpenSslDigest::listDigests()
{
    std::vector<std::string> result;
    ::OBJ_NAME_do_all_sorted(OBJ_NAME_TYPE_MD_METH, eachObj, &result);
    return result;
}

//---------------------------------------------------------------

HATN_OPENSSL_NAMESPACE_END
