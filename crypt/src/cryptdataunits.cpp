/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file crypt/cryptdataunits.cpp
  *
  *   Definition of DataUnt objects for crypt module
  *
  */

/****************************************************************************/

#include <hatn/dataunit/ipp/syntax.ipp>
#include <hatn/dataunit/ipp/wirebuf.ipp>

#include <hatn/crypt/crypt.h>
#include <hatn/crypt/cryptdataunits.h>

HATN_CRYPT_NAMESPACE_BEGIN

HATN_CRYPT_NAMESPACE_END

HDU_INSTANTIATE(HATN_CRYPT_NAMESPACE::cipher_suite,HATN_CRYPT_EXPORT)
HDU_INSTANTIATE(HATN_CRYPT_NAMESPACE::container_descriptor,HATN_CRYPT_EXPORT)
HDU_INSTANTIATE(HATN_CRYPT_NAMESPACE::file_stamp,HATN_CRYPT_EXPORT)
