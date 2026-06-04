/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/** @file tools/fileencryptor/fileencryptor_capi.h
  *
  * Plain C ABI for libhatnfileencryptor — used from Go via purego (dlopen).
  * Pattern follows whitembridge bridgestring.h conventions.
  */

#ifndef HATNFILEENCRYPTOR_CAPI_H
#define HATNFILEENCRYPTOR_CAPI_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * Types
 * -------------------------------------------------------------------------- */

/** Byte buffer returned by the library.  Always heap-allocated by the
 *  library; caller MUST release with hatn_file_encryptor_bytes_free(). */
typedef struct
{
    const char* data;
    size_t      length;
} hatn_file_encryptor_bytes;

/** Opaque encryption context (owns the hatn App + cipher suites). */
typedef struct hatn_file_encryptor_ctx_t* hatn_file_encryptor_ctx;

/* --------------------------------------------------------------------------
 * Error codes
 * -------------------------------------------------------------------------- */
enum {
    HATN_FILE_ENCRYPTOR_OK           = 0,
    HATN_FILE_ENCRYPTOR_ERR_INIT     = 1,   /* App init / config load failed */
    HATN_FILE_ENCRYPTOR_ERR_ENCRYPT  = 2,   /* Encryption failed             */
    HATN_FILE_ENCRYPTOR_ERR_BADARG   = 3    /* NULL pointer or zero-length    */
};

/* --------------------------------------------------------------------------
 * API
 * -------------------------------------------------------------------------- */

/**
 * @brief Create an encryption context.
 *
 * Loads the hatn app config from @p config_file, sets @p config_root as the
 * top-level config key, and calls App::init() to load cipher suites.
 *
 * @param config_file      Path to the JSONC app config (null-terminated prefix
 *                         of length @p config_file_len, need not be NUL-terminated).
 * @param config_file_len  Length of @p config_file.
 * @param config_root      App config-tree root (e.g. "asta").
 * @param config_root_len  Length of @p config_root.
 * @param out_ctx          Receives the created context on success.
 * @return HATN_FILE_ENCRYPTOR_OK or an error code.
 */
int hatn_file_encryptor_ctx_create(const char* config_file,
                                   size_t      config_file_len,
                                   const char* config_root,
                                   size_t      config_root_len,
                                   hatn_file_encryptor_ctx* out_ctx);

/**
 * @brief Encrypt plaintext bytes.
 *
 * Encrypts @p plaintext_len bytes at @p plaintext using @p passphrase and the
 * default cipher suite from the context.  The output buffer is heap-allocated
 * by the library and MUST be released by the caller via
 * hatn_file_encryptor_bytes_free().
 *
 * @param ctx              Context created by hatn_file_encryptor_ctx_create().
 * @param plaintext        Input bytes.
 * @param plaintext_len    Length of input.
 * @param passphrase       Passphrase bytes (run through PBKDF inside CryptContainer).
 * @param passphrase_len   Length of passphrase.
 * @param out_ciphertext   Receives the encrypted CryptContainer blob.
 * @return HATN_FILE_ENCRYPTOR_OK or an error code.
 */
int hatn_file_encryptor_encrypt(hatn_file_encryptor_ctx     ctx,
                                const char*                 plaintext,
                                size_t                      plaintext_len,
                                const char*                 passphrase,
                                size_t                      passphrase_len,
                                hatn_file_encryptor_bytes*  out_ciphertext);

/**
 * @brief Free a byte buffer returned by the library.
 * @param buf  Pointer to the buffer struct (data pointer is freed; struct zeroed).
 */
void hatn_file_encryptor_bytes_free(hatn_file_encryptor_bytes* buf);

/**
 * @brief Destroy an encryption context and release all resources.
 * @param ctx  Context to destroy (may be NULL).
 */
void hatn_file_encryptor_ctx_destroy(hatn_file_encryptor_ctx ctx);

/**
 * @brief Return the last error message for the calling thread.
 * @return Pointer to a static thread-local string; valid until the next
 *         library call on this thread.  Never NULL.
 */
const char* hatn_file_encryptor_last_error(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* HATNFILEENCRYPTOR_CAPI_H */
