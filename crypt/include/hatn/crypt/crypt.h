/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/crypt.h
  *
  *  Hatn Crypt Library.
  *
  */

/****************************************************************************/

#ifndef HATNCRYPT_H
#define HATNCRYPT_H

#include <hatn/crypt/config.h>

#include <hatn/common/visibilitymacros.h>

#ifndef HATN_CRYPT_EXPORT
#   ifdef BUILD_HATN_CRYPT
#       define HATN_CRYPT_EXPORT HATN_VISIBILITY_EXPORT
#   else
#       define HATN_CRYPT_EXPORT HATN_VISIBILITY_IMPORT
#   endif
#endif

#define HATN_CRYPT_NAMESPACE_BEGIN namespace hatn { namespace crypt {
#define HATN_CRYPT_NAMESPACE_END }}

#define HATN_CRYPT_NAMESPACE hatn::crypt
#define HATN_CRYPT_NS crypt
#define HATN_CRYPT_USING using namespace hatn::crypt;

HATN_CRYPT_NAMESPACE_BEGIN
HATN_CRYPT_NAMESPACE_END

#endif // HATNCRYPT_H
