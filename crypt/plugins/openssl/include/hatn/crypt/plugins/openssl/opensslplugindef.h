/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/plugins/openssl/opensslplugindef.h
  *
  *  Export definitions for Hatn encryption plugin with OpenSSL backend.
  *
  */

/****************************************************************************/

#ifndef HATNOPENSSLPLUGINDEF_H
#define HATNOPENSSLPLUGINDEF_H

#ifdef _WIN32
#ifndef NOCRYPT
#define NOCRYPT
#endif
#include <winsock2.h>
#endif

#ifdef NO_DYNAMIC_HATN_PLUGINS
#include <hatn/crypt/plugins/openssl/config.h>
#endif

// define export symbols fo windows platform
#ifdef _WIN32
#    ifndef HATN_OPENSSL_EXPORT
#        ifdef BUILD_HATN_OPENSSL
#            define HATN_OPENSSL_EXPORT __declspec(dllexport)
#        else
#            define HATN_OPENSSL_EXPORT __declspec(dllimport)
#        endif
#    endif
#else
#    define HATN_OPENSSL_EXPORT
#endif

#define HATN_OPENSSL_NAMESPACE_BEGIN namespace hatn { namespace crypt { namespace openssl {
#define HATN_OPENSSL_NAMESPACE_END }}}

#define HATN_OPENSSL_NAMESPACE hatn::crypt::openssl
#define HATN_OPENSSL_NS openssl
#define HATN_OPENSSL_USING using namespace hatn::crypt::openssl;

HATN_OPENSSL_NAMESPACE_BEGIN

constexpr const int OPENSSL_OK=1;

HATN_OPENSSL_NAMESPACE_END

#endif // HATNOPENSSLPLUGINDEF_H

