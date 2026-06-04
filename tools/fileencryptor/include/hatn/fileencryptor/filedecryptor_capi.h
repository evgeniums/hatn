/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/** @file tools/fileencryptor/filedecryptor_capi.h
  *
  * Plain C ABI for the FileDecryptor — used from Go via purego (dlopen).
  * Pattern follows the FileEncryptor C ABI conventions.
  */

#ifndef HATNFILEDECRYPTOR_CAPI_H
#define HATNFILEDECRYPTOR_CAPI_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * Types
 * -------------------------------------------------------------------------- */

/** Byte buffer returned by the library.  Always heap-allocated; caller MUST
 *  release with hatn_file_decryptor_bytes_free(). */
typedef struct
{
    const char* data;
    size_t      length;
} hatn_file_decryptor_bytes;

/** Opaque decryption context (owns the hatn App + cipher suites). */
typedef struct hatn_file_decryptor_ctx_t* hatn_file_decryptor_ctx;

/* --------------------------------------------------------------------------
 * Error codes
 * -------------------------------------------------------------------------- */
enum {
    HATN_FILE_DECRYPTOR_OK           = 0,
    HATN_FILE_DECRYPTOR_ERR_INIT     = 1,   /* App init / config load failed  */
    HATN_FILE_DECRYPTOR_ERR_DECRYPT  = 2,   /* Decryption failed              */
    HATN_FILE_DECRYPTOR_ERR_BADARG   = 3    /* NULL pointer or zero-length    */
};

/* --------------------------------------------------------------------------
 * API
 * -------------------------------------------------------------------------- */

/**
 * @brief Create a decryption context.
 *
 * Loads @p config_file, extracts the subsection at @p section_path, and
 * initialises cipher suites from it via a hatn App.
 *
 * @param config_file       Path to the JSONC config file.
 * @param config_file_len   Length of @p config_file.
 * @param section_path      Dot-separated path to the fileencryptor subsection
 *                          (e.g. "fileencryptor").
 * @param section_path_len  Length of @p section_path.
 * @param out_ctx           Receives the created context on success.
 * @return HATN_FILE_DECRYPTOR_OK or an error code.
 */
int hatn_file_decryptor_ctx_create(const char* config_file,
                                   size_t      config_file_len,
                                   const char* section_path,
                                   size_t      section_path_len,
                                   hatn_file_decryptor_ctx* out_ctx);

/**
 * @brief Decrypt a CryptContainer blob.
 *
 * @param ctx              Context created by hatn_file_decryptor_ctx_create().
 * @param ciphertext       Encrypted bytes (CryptContainer blob).
 * @param ciphertext_len   Length of ciphertext.
 * @param passphrase       Passphrase used during encryption.
 * @param passphrase_len   Length of passphrase.
 * @param out_plaintext    Receives the decrypted bytes (heap-allocated; free with
 *                         hatn_file_decryptor_bytes_free()).
 * @return HATN_FILE_DECRYPTOR_OK or an error code.
 */
int hatn_file_decryptor_decrypt(hatn_file_decryptor_ctx    ctx,
                                const char*                ciphertext,
                                size_t                     ciphertext_len,
                                const char*                passphrase,
                                size_t                     passphrase_len,
                                hatn_file_decryptor_bytes* out_plaintext);

/**
 * @brief Free a byte buffer returned by the library.
 * @param buf  Buffer to free (data pointer freed; struct zeroed).
 */
void hatn_file_decryptor_bytes_free(hatn_file_decryptor_bytes* buf);

/**
 * @brief Destroy a decryption context and release all resources.
 * @param ctx  Context to destroy (may be NULL).
 */
void hatn_file_decryptor_ctx_destroy(hatn_file_decryptor_ctx ctx);

/**
 * @brief Return the last error message for the calling thread.
 * @return Pointer to a static thread-local string.  Never NULL.
 */
const char* hatn_file_decryptor_last_error(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* HATNFILEDECRYPTOR_CAPI_H */
