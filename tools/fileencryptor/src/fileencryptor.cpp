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

#include <hatn/app/app.h>
#include <hatn/app/appname.h>

#include <hatn/fileencryptor/fileencryptor.h>

namespace hatn {
namespace fileencryptor {

//---------------------------------------------------------------

class FileEncryptorImpl
{
public:
    std::unique_ptr<app::App> app;
    bool initialised{false};
};

//---------------------------------------------------------------

FileEncryptor::FileEncryptor()
    : d(std::make_unique<FileEncryptorImpl>())
{}

FileEncryptor::~FileEncryptor() = default;

//---------------------------------------------------------------

common::Error FileEncryptor::init(std::string_view configFilePath,
                                  std::string_view appConfigRoot)
{
    if (d->initialised)
    {
        return commonError(common::CommonError::INVALID_ARGUMENT);
    }

    d->app = std::make_unique<app::App>(app::AppName{"hatn-file-encryptor", "hatn File Encryptor"});
    d->app->setAppConfigRoot(std::string{appConfigRoot});

    auto ec = d->app->loadConfigFile(std::string{configFilePath});
    HATN_CHECK_EC(ec)

    ec = d->app->init();
    HATN_CHECK_EC(ec)

    d->initialised = true;
    return OK;
}

//---------------------------------------------------------------

common::Result<std::string> FileEncryptor::generatePassphrase()
{
    auto* gen = d->app->cipherSuites().passwordGenerator();
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
    container.setCipherSuites(&d->app->cipherSuites());
    container.setPassphrase(common::MemoryLockedArray{passphrase});
    return container.pack(plaintext, out);
}

//---------------------------------------------------------------

} // namespace fileencryptor
} // namespace hatn
