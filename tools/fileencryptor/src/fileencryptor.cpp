/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

#include <string_view>

#include <hatn/common/memorylockeddata.h>
#include <hatn/common/errorcategory.h>

#include <hatn/crypt/cryptcontainer.h>
#include <hatn/crypt/ciphersuite.h>
#include <hatn/crypt/passwordgenerator.h>

#include <hatn/fileencryptor/fileencryptor.h>

#include "filecryptbase.h"

namespace hatn {
namespace fileencryptor {

//---------------------------------------------------------------

FileEncryptor::FileEncryptor()
    : d(std::make_unique<FileCryptBase>())
{}

FileEncryptor::~FileEncryptor() = default;
FileEncryptor::FileEncryptor(FileEncryptor&&) noexcept = default;
FileEncryptor& FileEncryptor::operator=(FileEncryptor&&) noexcept = default;

//---------------------------------------------------------------

common::Error FileEncryptor::init(std::string_view configFilePath,
                                  std::string_view sectionPath)
{
    return d->init(configFilePath, sectionPath);
}

//---------------------------------------------------------------

common::Result<std::string> FileEncryptor::generatePassphrase()
{
    const auto* suites = d->app->cipherSuites().suites();
    auto* gen = suites ? suites->passwordGenerator() : nullptr;
    if (!gen)
    {
        return commonError(common::CommonError::UNSUPPORTED);
    }
    return gen->generateString();
}

//---------------------------------------------------------------

common::Error FileEncryptor::encrypt(std::string_view plaintext,
                                     std::string_view passphrase,
                                     common::ByteArray& out)
{
    crypt::CryptContainer container{d->app->defaultCipherSuite()};
    container.setCipherSuites(d->app->cipherSuites().suites());
    container.setPassphrase(common::MemoryLockedArray{passphrase});
    return container.pack(plaintext, out);
}

//---------------------------------------------------------------

} // namespace fileencryptor
} // namespace hatn
