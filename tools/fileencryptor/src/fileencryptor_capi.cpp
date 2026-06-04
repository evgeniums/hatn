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

#include <hatn/fileencryptor/fileencryptor.h>
#include <hatn/fileencryptor/fileencryptor_capi.h>

/* --------------------------------------------------------------------------
 * Thread-local error message storage
 * -------------------------------------------------------------------------- */

static thread_local std::string t_last_error;

static void set_error(const std::string& msg)
{
    t_last_error = msg;
}

/* --------------------------------------------------------------------------
 * Opaque context struct
 * -------------------------------------------------------------------------- */

struct hatn_file_encryptor_ctx_t
{
    hatn::fileencryptor::FileEncryptor encryptor;
};

/* --------------------------------------------------------------------------
 * Helpers
 * -------------------------------------------------------------------------- */

/** Copy a std::string / ByteArray into a malloc-allocated hatn_file_encryptor_bytes. */
static void fill_bytes(hatn_file_encryptor_bytes* out, const char* data, size_t len)
{
    char* buf = static_cast<char*>(std::malloc(len));
    if (buf)
    {
        std::memcpy(buf, data, len);
    }
    out->data   = buf;
    out->length = buf ? len : 0;
}

/* --------------------------------------------------------------------------
 * API implementation
 * -------------------------------------------------------------------------- */

extern "C" {

int hatn_file_encryptor_ctx_create(const char* config_file,
                                   size_t      config_file_len,
                                   const char* config_root,
                                   size_t      config_root_len,
                                   hatn_file_encryptor_ctx* out_ctx)
{
    if (!config_file || !config_file_len || !config_root || !config_root_len || !out_ctx)
    {
        set_error("hatn_file_encryptor_ctx_create: null or empty argument");
        return HATN_FILE_ENCRYPTOR_ERR_BADARG;
    }

    try
    {
        auto* ctx = new hatn_file_encryptor_ctx_t{};

        std::string_view cfgPath{config_file, config_file_len};
        std::string_view cfgRoot{config_root, config_root_len};

        auto ec = ctx->encryptor.init(cfgPath, cfgRoot);
        if (ec)
        {
            set_error(ec.message());
            delete ctx;
            return HATN_FILE_ENCRYPTOR_ERR_INIT;
        }

        *out_ctx = ctx;
        return HATN_FILE_ENCRYPTOR_OK;
    }
    catch (const std::exception& ex)
    {
        set_error(std::string{"hatn_file_encryptor_ctx_create exception: "} + ex.what());
        return HATN_FILE_ENCRYPTOR_ERR_INIT;
    }
    catch (...)
    {
        set_error("hatn_file_encryptor_ctx_create: unknown exception");
        return HATN_FILE_ENCRYPTOR_ERR_INIT;
    }
}

int hatn_file_encryptor_encrypt(hatn_file_encryptor_ctx    ctx,
                                const char*                plaintext,
                                size_t                     plaintext_len,
                                const char*                passphrase,
                                size_t                     passphrase_len,
                                hatn_file_encryptor_bytes* out_ciphertext)
{
    if (!ctx || !plaintext || !passphrase || !passphrase_len || !out_ciphertext)
    {
        set_error("hatn_file_encryptor_encrypt: null or empty argument");
        return HATN_FILE_ENCRYPTOR_ERR_BADARG;
    }

    try
    {
        std::string_view pt{plaintext, plaintext_len};
        std::string_view pp{passphrase, passphrase_len};

        hatn::common::ByteArray result;
        auto ec = ctx->encryptor.encrypt(pt, pp, result);
        if (ec)
        {
            set_error(ec.message());
            return HATN_FILE_ENCRYPTOR_ERR_ENCRYPT;
        }

        fill_bytes(out_ciphertext, result.data(), result.size());
        return HATN_FILE_ENCRYPTOR_OK;
    }
    catch (const std::exception& ex)
    {
        set_error(std::string{"hatn_file_encryptor_encrypt exception: "} + ex.what());
        return HATN_FILE_ENCRYPTOR_ERR_ENCRYPT;
    }
    catch (...)
    {
        set_error("hatn_file_encryptor_encrypt: unknown exception");
        return HATN_FILE_ENCRYPTOR_ERR_ENCRYPT;
    }
}

void hatn_file_encryptor_bytes_free(hatn_file_encryptor_bytes* buf)
{
    if (buf && buf->data)
    {
        std::free(const_cast<char*>(buf->data));
        buf->data   = nullptr;
        buf->length = 0;
    }
}

void hatn_file_encryptor_ctx_destroy(hatn_file_encryptor_ctx ctx)
{
    delete ctx;
}

const char* hatn_file_encryptor_last_error(void)
{
    return t_last_error.c_str();
}

} // extern "C"
