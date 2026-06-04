/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

#include <string_view>

#include <hatn/common/memorylockeddata.h>
#include <hatn/common/errorcategory.h>

#include <hatn/base/configtree.h>
#include <hatn/base/configtreepath.h>
#include <hatn/base/configtreeloader.h>

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
                                  std::string_view sectionPath)
{
    if (d->initialised)
    {
        return commonError(common::CommonError::INVALID_ARGUMENT);
    }

    // Load the full server config, extract the subsection at sectionPath,
    // serialize it back to JSON, then feed only that to the App.
    // This lets the server config keep all FileEncryptor config nested under
    // one key (e.g. "fileencryptor") without affecting any other service.

    base::ConfigTreeLoader loader;

    auto fullTree = loader.createFromFile(std::string{configFilePath});
    if (fullTree)
    {
        return fullTree.takeError();
    }

    auto sectionResult = fullTree.value().get(base::ConfigTreePath{std::string{sectionPath}});
    if (sectionResult)
    {
        return sectionResult.takeError();
    }
    const auto& section = sectionResult.value();

    auto ioHandler = loader.handler();
    if (!ioHandler)
    {
        return commonError(common::CommonError::UNSUPPORTED);
    }
    auto jsonResult = ioHandler->serialize(section);
    if (jsonResult)
    {
        return jsonResult.takeError();
    }

    d->app = std::make_unique<app::App>(app::AppName{"fileencryptor", "hatn File Encryptor"});
    // Use hatn's built-in "app" root for the app subsection within the extracted JSON.
    // No setAppConfigRoot call needed — "app" is the default.

    auto ec = d->app->loadConfigString(jsonResult.value());
    HATN_CHECK_EC(ec)

    ec = d->app->init();
    HATN_CHECK_EC(ec)

    d->initialised = true;
    return OK;
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
