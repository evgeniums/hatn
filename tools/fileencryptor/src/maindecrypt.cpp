/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/**
 * hatn-file-decryptor: CLI tool to decrypt a CryptContainer blob produced by
 * hatn-file-encryptor.
 *
 * Cipher suites are read from the "fileencryptor" subsection of the config
 * file (or another subsection specified with --section-path).
 *
 * Usage:
 *   hatn-file-decryptor --config <file> --encrypted <file> --target <file>
 *                       --passphrase <str>
 *                       [--section-path <path>]
 *
 * Options:
 *   --config          Path to the JSONC config file (required).
 *   --encrypted       Input encrypted file (CryptContainer blob) (required).
 *   --target          Output file for the decrypted plaintext (required).
 *   --passphrase      Passphrase used during encryption (required).
 *   [--section-path]  Subsection path in config (default: "fileencryptor").
 */

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <cerrno>
#include <stdexcept>

#include <hatn/common/bytearray.h>

#include <hatn/fileencryptor/filedecryptor.h>

//---------------------------------------------------------------

static void usage(const char* argv0)
{
    std::cerr
        << "Usage: " << argv0 << "\n"
        << "  --config <file>           Config file (JSONC)\n"
        << "  --encrypted <file>        Input encrypted file\n"
        << "  --target <file>           Output decrypted file\n"
        << "  --passphrase <str>        Passphrase used during encryption\n"
        << "  [--section-path <path>]   Subsection path in config (default: fileencryptor)\n";
}

static std::string readFile(const std::string& path)
{
    std::ifstream in{path, std::ios::binary};
    if (!in)
    {
        throw std::runtime_error{"cannot open input file: " + path + " (" + std::strerror(errno) + ")"};
    }
    return std::string{std::istreambuf_iterator<char>{in}, {}};
}

static void writeFile(const std::string& path, const char* data, size_t len)
{
    std::ofstream out{path, std::ios::binary | std::ios::trunc};
    if (!out)
    {
        throw std::runtime_error{"cannot open output file: " + path + " (" + std::strerror(errno) + ")"};
    }
    out.write(data, static_cast<std::streamsize>(len));
    if (!out)
    {
        throw std::runtime_error{"failed to write file: " + path};
    }
}

//---------------------------------------------------------------

int main(int argc, char* argv[])
{
    std::string configFile;
    std::string sectionPath  = "fileencryptor";
    std::string encryptedFile;
    std::string targetFile;
    std::string passphrase;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg{argv[i]};
        auto next = [&]() -> std::string {
            if (i + 1 >= argc)
            {
                throw std::runtime_error{"option " + arg + " requires an argument"};
            }
            return argv[++i];
        };

        if      (arg == "--config")        configFile    = next();
        else if (arg == "--section-path")  sectionPath   = next();
        else if (arg == "--encrypted")     encryptedFile = next();
        else if (arg == "--target")        targetFile    = next();
        else if (arg == "--passphrase")    passphrase    = next();
        else if (arg == "--help" || arg == "-h") { usage(argv[0]); return 0; }
        else { std::cerr << "Unknown option: " << arg << "\n"; usage(argv[0]); return 1; }
    }

    if (configFile.empty() || encryptedFile.empty() || targetFile.empty() || passphrase.empty())
    {
        std::cerr << "Error: --config, --encrypted, --target and --passphrase are required.\n";
        usage(argv[0]);
        return 1;
    }

    try
    {
        hatn::fileencryptor::FileDecryptor dec;
        auto ec = dec.init(configFile, sectionPath);
        if (ec)
        {
            std::cerr << "Decryptor init failed: " << ec.message() << "\n";
            return 1;
        }

        const auto ciphertext = readFile(encryptedFile);

        hatn::common::ByteArray plaintext;
        ec = dec.decrypt(ciphertext, passphrase, plaintext);
        if (ec)
        {
            std::cerr << "Decryption failed: " << ec.message() << "\n";
            return 1;
        }

        writeFile(targetFile, plaintext.data(), plaintext.size());
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
