/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/** @file tools/fileencryptor/fileencryptor.h
  *
  * Generic file/data encryptor backed by a hatn App (loads cipher suites from
  * an app config file + config-tree root).
  */

#ifndef HATNFILEENCRYPTOR_H
#define HATNFILEENCRYPTOR_H

#include <memory>
#include <string>
#include <string_view>

#include <hatn/common/error.h>
#include <hatn/common/result.h>
#include <hatn/common/bytearray.h>

namespace hatn {
namespace fileencryptor {

class FileEncryptorImpl;

/**
 * @brief Generic data encryptor driven by hatn CryptContainer.
 *
 * Init once per process with init(configFilePath, appConfigRoot); the hatn App
 * loads cipher suites from the "crypt" section of the config (cipher_suites +
 * default_cipher_suite).  Encrypt arbitrary bytes via encrypt(); each call
 * constructs a fresh CryptContainer so concurrent use is safe once init() has
 * returned.
 */
class FileEncryptor
{
public:
    FileEncryptor();
    ~FileEncryptor();

    FileEncryptor(const FileEncryptor&) = delete;
    FileEncryptor& operator=(const FileEncryptor&) = delete;
    FileEncryptor(FileEncryptor&&) = default;
    FileEncryptor& operator=(FileEncryptor&&) = default;

    /**
     * @brief Load cipher suites from a subsection of an app config file.
     * @param configFilePath Path to the JSONC config file (e.g. the server config).
     * @param sectionPath    Dot-separated path to the fileencryptor subsection
     *                       within the config file (e.g. "fileencryptor").
     *                       The subsection must contain "crypt" (required),
     *                       "password_generator" (optional), and "app" (optional).
     *
     * Extracts the subsection at @p sectionPath, feeds it to a hatn App as its
     * entire config, then calls App::init() to load cipher suites.
     * May be called only once; subsequent calls return an error.
     */
    common::Error init(std::string_view configFilePath,
                       std::string_view sectionPath);

    /**
     * @brief Generate a passphrase using the App's password generator.
     * @return Generated passphrase string, or an error.
     *
     * Requires init() to have succeeded.  Inverted Result convention: a
     * truthy Result means error (same as hatn::crypt::PasswordGenerator).
     */
    common::Result<std::string> generatePassphrase();

    /**
     * @brief Encrypt arbitrary bytes with a passphrase via CryptContainer.
     * @param plaintext  Input bytes.
     * @param passphrase Passphrase; run through PBKDF inside CryptContainer.
     * @param out        Receives the encrypted CryptContainer blob.
     * @return Error on failure.
     *
     * A fresh CryptContainer is constructed per call, so this method is safe
     * to call concurrently once init() has completed.
     */
    common::Error encrypt(std::string_view plaintext,
                          std::string_view passphrase,
                          common::ByteArray& out);

private:
    std::unique_ptr<FileEncryptorImpl> d;
};

} // namespace fileencryptor
} // namespace hatn

#endif // HATNFILEENCRYPTOR_H
