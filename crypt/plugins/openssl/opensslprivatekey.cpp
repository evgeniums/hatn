/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslprivatekey.cpp
 *
 * 	Private key wrapper with OpenSSL backend
 *
 */
/****************************************************************************/

#include <hatn/common/logger.h>

#include <hatn/dataunit/visitors.h>
#include <hatn/dataunit/ipp/syntax.ipp>

#include <hatn/crypt/encrypthmac.h>

#include <hatn/crypt/plugins/openssl/opensslerror.h>
#include <hatn/crypt/plugins/openssl/opensslutils.h>

#include <hatn/crypt/plugins/openssl/opensslprivatekey.h>
#include <hatn/crypt/plugins/openssl/opensslprivatekey.ipp>

namespace hatn {

using namespace common;

namespace crypt {
namespace openssl {

/******************* OpenSslPrivateKey ********************/

#ifndef _WIN32
template class OpenSslPKey<PrivateKey>;
#endif

//---------------------------------------------------------------
} // namespace openssl
HATN_CRYPT_NAMESPACE_END
