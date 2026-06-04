/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/**
 * hatn-file-encryptor: CLI tool to encrypt a file using hatn CryptContainer.
 *
 * Usage:
 *   hatn-file-encryptor --config <file> --config-root <root>
 *                       --plain <file> --target <file>
 *                       [--passphrase <str>]
 *                       [--passphrase-file <file>]
 *
 * Options:
 *   --config          Path to the app JSONC config file (required).
 *   --config-root     Top-level config key, e.g. "asta" (required).
 *   --plain           Input plaintext file (required).
 *   --target          Output file for the encrypted CryptContainer blob (required).
 *   --passphrase      Passphrase to use; if omitted one is generated.
 *   --passphrase-file If given, the passphrase is also written to this file (mode 0600).
 *
 * The passphrase is always printed to stdout.
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <cerrno>
#include <stdexcept>

#include <sys/stat.h>

#include <hatn/common/bytearray.h>

#include <hatn/fileencryptor/fileencryptor.h>

//---------------------------------------------------------------

static void usage(const char* argv0)
{
    std::cerr
        << "Usage: " << argv0 << "\n"
        << "  --config <file>          App config file (JSONC)\n"
        << "  --config-root <root>     Config tree root (e.g. asta)\n"
        << "  --plain <file>           Input plaintext file\n"
        << "  --target <file>          Output encrypted file\n"
        << "  [--passphrase <str>]     Passphrase (auto-generated if omitted)\n"
        << "  [--passphrase-file <f>]  Also write passphrase to this file\n";
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

static void writeFile(const std::string& path, const char* data, size_t len, bool secure)
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
    out.close();
    if (secure)
    {
        ::chmod(path.c_str(), 0600);
    }
}

//---------------------------------------------------------------

int main(int argc, char* argv[])
{
    std::string configFile;
    std::string configRoot;
    std::string plainFile;
    std::string targetFile;
    std::string passphrase;
    std::string passphraseFile;

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

        if      (arg == "--config")          configFile     = next();
        else if (arg == "--config-root")     configRoot     = next();
        else if (arg == "--plain")           plainFile      = next();
        else if (arg == "--target")          targetFile     = next();
        else if (arg == "--passphrase")      passphrase     = next();
        else if (arg == "--passphrase-file") passphraseFile = next();
        else if (arg == "--help" || arg == "-h") { usage(argv[0]); return 0; }
        else { std::cerr << "Unknown option: " << arg << "\n"; usage(argv[0]); return 1; }
    }

    if (configFile.empty() || configRoot.empty() || plainFile.empty() || targetFile.empty())
    {
        std::cerr << "Error: --config, --config-root, --plain and --target are required.\n";
        usage(argv[0]);
        return 1;
    }

    try
    {
        // Initialise encryptor from app config
        hatn::fileencryptor::FileEncryptor enc;
        auto ec = enc.init(configFile, configRoot);
        if (ec)
        {
            std::cerr << "Encryptor init failed: " << ec.message() << "\n";
            return 1;
        }

        // Resolve passphrase
        if (passphrase.empty())
        {
            auto result = enc.generatePassphrase();
            if (result)
            {
                std::cerr << "Passphrase generation failed: " << result.error().message() << "\n";
                return 1;
            }
            passphrase = result.value();
        }

        // Read plaintext
        const auto plain = readFile(plainFile);

        // Encrypt
        hatn::common::ByteArray ciphertext;
        ec = enc.encrypt(plain, passphrase, ciphertext);
        if (ec)
        {
            std::cerr << "Encryption failed: " << ec.message() << "\n";
            return 1;
        }

        // Write output
        writeFile(targetFile, ciphertext.data(), ciphertext.size(), false);

        // Deliver passphrase
        std::cout << "Encryption passphrase: " << passphrase << "\n";

        if (!passphraseFile.empty())
        {
            writeFile(passphraseFile, passphrase.data(), passphrase.size(), true);
        }
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
