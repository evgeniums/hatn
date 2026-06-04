/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/** @file tools/fileencryptor/src/filecryptbase.h
  *
  * Internal shared implementation for FileEncryptor and FileDecryptor.
  * Not part of the installed public API.
  */

#pragma once

#include <memory>
#include <string_view>

#include <hatn/common/error.h>

#include <hatn/app/app.h>

namespace hatn {
namespace fileencryptor {

/**
 * @brief Shared pimpl holding the hatn App and cipher suites.
 *
 * Both FileEncryptor and FileDecryptor own a unique_ptr<FileCryptBase>.
 * The init() method is the single canonical implementation of the
 * config-subsection extraction + App initialisation flow.
 */
class FileCryptBase
{
public:
    std::unique_ptr<app::App> app;
    bool initialised{false};

    /**
     * @brief Load cipher suites from a subsection of a JSONC config file.
     *
     * Extracts the node at @p sectionPath from the full config file, serialises
     * it back to JSON, and feeds it to a hatn App (loadConfigString + init).
     * The subsection must contain a "crypt" key; "password_generator", "app"
     * and "db" are optional.
     */
    common::Error init(std::string_view configFilePath,
                       std::string_view sectionPath);
};

} // namespace fileencryptor
} // namespace hatn
