/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>

#include <hatn/common/bytearray.h>

#include <hatn/fileencryptor/filedecryptor.h>
#include <hatn/fileencryptor/filedecryptor_capi.h>

#include "filecrypt_helpers.h"

/* --------------------------------------------------------------------------
 * Thread-local error message storage
 * -------------------------------------------------------------------------- */

static thread_local std::string t_decryptor_last_error;

static void set_error(const std::string& msg)
{
    t_decryptor_last_error = msg;
}

/* --------------------------------------------------------------------------
 * Opaque context struct
 * -------------------------------------------------------------------------- */

struct hatn_file_decryptor_ctx_t
{
    hatn::fileencryptor::FileDecryptor decryptor;
};

/* --------------------------------------------------------------------------
 * API implementation
 * -------------------------------------------------------------------------- */

extern "C" {

int hatn_file_decryptor_ctx_create(const char* config_file,
                                   size_t      config_file_len,
                                   const char* section_path,
                                   size_t      section_path_len,
                                   hatn_file_decryptor_ctx* out_ctx)
{
    if (!config_file || !config_file_len || !section_path || !section_path_len || !out_ctx)
    {
        set_error("hatn_file_decryptor_ctx_create: null or empty argument");
        return HATN_FILE_DECRYPTOR_ERR_BADARG;
    }

    try
    {
        auto* ctx = new hatn_file_decryptor_ctx_t{};

        std::string_view cfgPath{config_file, config_file_len};
        std::string_view secPath{section_path, section_path_len};

        auto ec = ctx->decryptor.init(cfgPath, secPath);
        if (ec)
        {
            set_error(ec.message());
            delete ctx;
            return HATN_FILE_DECRYPTOR_ERR_INIT;
        }

        *out_ctx = ctx;
        return HATN_FILE_DECRYPTOR_OK;
    }
    catch (const std::exception& ex)
    {
        set_error(std::string{"hatn_file_decryptor_ctx_create exception: "} + ex.what());
        return HATN_FILE_DECRYPTOR_ERR_INIT;
    }
    catch (...)
    {
        set_error("hatn_file_decryptor_ctx_create: unknown exception");
        return HATN_FILE_DECRYPTOR_ERR_INIT;
    }
}

int hatn_file_decryptor_decrypt(hatn_file_decryptor_ctx    ctx,
                                const char*                ciphertext,
                                size_t                     ciphertext_len,
                                const char*                passphrase,
                                size_t                     passphrase_len,
                                hatn_file_decryptor_bytes* out_plaintext)
{
    if (!ctx || !ciphertext || !ciphertext_len || !passphrase || !passphrase_len || !out_plaintext)
    {
        set_error("hatn_file_decryptor_decrypt: null or empty argument");
        return HATN_FILE_DECRYPTOR_ERR_BADARG;
    }

    try
    {
        std::string_view ct{ciphertext, ciphertext_len};
        std::string_view pp{passphrase, passphrase_len};

        hatn::common::ByteArray result;
        auto ec = ctx->decryptor.decrypt(ct, pp, result);
        if (ec)
        {
            set_error(ec.message());
            return HATN_FILE_DECRYPTOR_ERR_DECRYPT;
        }

        filecrypt_fill_bytes(out_plaintext,
                             &out_plaintext->data,
                             &out_plaintext->length,
                             result.data(),
                             result.size());
        return HATN_FILE_DECRYPTOR_OK;
    }
    catch (const std::exception& ex)
    {
        set_error(std::string{"hatn_file_decryptor_decrypt exception: "} + ex.what());
        return HATN_FILE_DECRYPTOR_ERR_DECRYPT;
    }
    catch (...)
    {
        set_error("hatn_file_decryptor_decrypt: unknown exception");
        return HATN_FILE_DECRYPTOR_ERR_DECRYPT;
    }
}

void hatn_file_decryptor_bytes_free(hatn_file_decryptor_bytes* buf)
{
    if (buf && buf->data)
    {
        std::free(const_cast<char*>(buf->data));
        buf->data   = nullptr;
        buf->length = 0;
    }
}

void hatn_file_decryptor_ctx_destroy(hatn_file_decryptor_ctx ctx)
{
    delete ctx;
}

const char* hatn_file_decryptor_last_error(void)
{
    return t_decryptor_last_error.c_str();
}

} // extern "C"
