/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/** @file tools/fileencryptor/filedecryptor.h
  *
  * Generic file/data decryptor backed by a hatn App (loads cipher suites from
  * an app config file + section path).  Symmetric counterpart of FileEncryptor.
  */

#ifndef HATNFILEDECRYPTOR_H
#define HATNFILEDECRYPTOR_H

#include <memory>
#include <string>
#include <string_view>

#include <hatn/common/error.h>
#include <hatn/common/bytearray.h>

namespace hatn {
namespace fileencryptor {

class FileCryptBase;

/**
 * @brief Generic data decryptor driven by hatn CryptContainer.
 *
 * Init once per process with init(configFilePath, sectionPath); the hatn App
 * loads cipher suites from the "crypt" sub-key of the extracted config section.
 * Decrypt CryptContainer blobs (produced by FileEncryptor) via decrypt(); each
 * call constructs a fresh CryptContainer so concurrent use is safe once init()
 * has returned.
 */
class FileDecryptor
{
public:
    FileDecryptor();
    ~FileDecryptor();

    FileDecryptor(const FileDecryptor&) = delete;
    FileDecryptor& operator=(const FileDecryptor&) = delete;

    // Declared here, defined in .cpp where FileCryptBase is complete.
    FileDecryptor(FileDecryptor&&) noexcept;
    FileDecryptor& operator=(FileDecryptor&&) noexcept;

    /**
     * @brief Load cipher suites from a subsection of an app config file.
     * @param configFilePath Path to the JSONC config file (e.g. the server config).
     * @param sectionPath    Dot-separated path to the fileencryptor subsection
     *                       within the config file (e.g. "fileencryptor").
     *                       The subsection must contain "crypt" (required),
     *                       "password_generator" (optional), and "app" (optional).
     *
     * Identical behaviour to FileEncryptor::init().
     * May be called only once; subsequent calls return an error.
     */
    common::Error init(std::string_view configFilePath,
                       std::string_view sectionPath);

    /**
     * @brief Decrypt a CryptContainer blob.
     * @param ciphertext  Encrypted bytes produced by FileEncryptor::encrypt().
     * @param passphrase  Passphrase used during encryption.
     * @param out         Receives the decrypted plaintext.
     * @return Error on failure (wrong passphrase, corrupt data, unknown suite).
     *
     * A fresh CryptContainer is constructed per call; safe for concurrent use
     * once init() has completed.
     */
    common::Error decrypt(std::string_view ciphertext,
                          std::string_view passphrase,
                          common::ByteArray& out);

private:
    std::unique_ptr<FileCryptBase> d;
};

} // namespace fileencryptor
} // namespace hatn

#endif // HATNFILEDECRYPTOR_H
