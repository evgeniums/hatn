/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslmackey.cpp
 *
 * 	MAC key wrapper with OpenSSL backend
 *
 */
/****************************************************************************/

#include <hatn/crypt/plugins/openssl/opensslerror.h>
#include <hatn/crypt/plugins/openssl/opensslutils.h>
#include <hatn/crypt/plugins/openssl/opensslmackey.h>
#include <hatn/crypt/plugins/openssl/opensslprivatekey.h>
#include <hatn/crypt/plugins/openssl/opensslprivatekey.ipp>

HATN_OPENSSL_NAMESPACE_BEGIN

//---------------------------------------------------------------

#ifndef _WIN32
template class OpenSslPKey<MACKey>;
#endif

//---------------------------------------------------------------

HATN_OPENSSL_NAMESPACE_END
