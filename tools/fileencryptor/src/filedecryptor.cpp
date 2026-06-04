/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

#include <string_view>

#include <hatn/common/memorylockeddata.h>

#include <hatn/crypt/cryptcontainer.h>
#include <hatn/crypt/ciphersuite.h>

#include <hatn/fileencryptor/filedecryptor.h>

#include "filecryptbase.h"

namespace hatn {
namespace fileencryptor {

//---------------------------------------------------------------

FileDecryptor::FileDecryptor()
    : d(std::make_unique<FileCryptBase>())
{}

FileDecryptor::~FileDecryptor() = default;
FileDecryptor::FileDecryptor(FileDecryptor&&) noexcept = default;
FileDecryptor& FileDecryptor::operator=(FileDecryptor&&) noexcept = default;

//---------------------------------------------------------------

common::Error FileDecryptor::init(std::string_view configFilePath,
                                  std::string_view sectionPath)
{
    return d->init(configFilePath, sectionPath);
}

//---------------------------------------------------------------

common::Error FileDecryptor::decrypt(std::string_view ciphertext,
                                     std::string_view passphrase,
                                     common::ByteArray& out)
{
    crypt::CryptContainer container{d->app->defaultCipherSuite()};
    container.setCipherSuites(d->app->cipherSuites().suites());
    container.setPassphrase(common::MemoryLockedArray{passphrase});
    return container.unpack(ciphertext, out);
}

//---------------------------------------------------------------

} // namespace fileencryptor
} // namespace hatn
