/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/** @file tools/fileencryptor/src/filecrypt_helpers.h
  *
  * Internal helpers shared by the encryptor and decryptor C ABI implementations.
  * Not part of the installed public API.
  */

#pragma once

#include <cstdlib>
#include <cstring>

/** Copy @p len bytes from @p data into a malloc-allocated output buffer. */
static inline void filecrypt_fill_bytes(void* out_struct,
                                        const char** out_data,
                                        std::size_t* out_length,
                                        const char*  data,
                                        std::size_t  len)
{
    (void)out_struct;
    char* buf = static_cast<char*>(std::malloc(len));
    if (buf && len)
    {
        std::memcpy(buf, data, len);
    }
    *out_data   = buf;
    *out_length = buf ? len : 0;
}
