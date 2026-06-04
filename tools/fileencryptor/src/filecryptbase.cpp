/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

#include <string_view>

#include <hatn/common/errorcategory.h>

#include <hatn/base/configtree.h>
#include <hatn/base/configtreepath.h>
#include <hatn/base/configtreeloader.h>

#include <hatn/app/appname.h>

#include "filecryptbase.h"

namespace hatn {
namespace fileencryptor {

//---------------------------------------------------------------

common::Error FileCryptBase::init(std::string_view configFilePath,
                                  std::string_view sectionPath)
{
    if (initialised)
    {
        return commonError(common::CommonError::INVALID_ARGUMENT);
    }

    // Load the full config file, extract the subsection at sectionPath,
    // serialize it back to JSON, then feed only that to the App.
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

    app = std::make_unique<app::App>(app::AppName{"fileencryptor", "hatn File Encryptor"});

    auto ec = app->loadConfigString(jsonResult.value());
    HATN_CHECK_EC(ec)

    ec = app->init();
    HATN_CHECK_EC(ec)

    initialised = true;
    return OK;
}

//---------------------------------------------------------------

} // namespace fileencryptor
} // namespace hatn
