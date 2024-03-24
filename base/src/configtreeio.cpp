/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file base/configtreeio.сpp
  *
  *  Containse implementation of config tree input/output operations.
  *
  */

#include <vector>

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include <hatn/common/errorstack.h>
#include <hatn/common/translate.h>
#include <hatn/common/plainfile.h>
#include <hatn/common/runonscopeexit.h>
#include <hatn/common/filesystem.h>

#include <hatn/base/baseerror.h>
#include <hatn/base/configtreeio.h>

HATN_BASE_NAMESPACE_BEGIN

//---------------------------------------------------------------

ConfigTreeIo::~ConfigTreeIo()=default;

//---------------------------------------------------------------

std::string ConfigTreeIo::fileFormat(lib::string_view filename)
{
    auto format=lib::filesystem::path(filename).extension().string();
    boost::algorithm::trim_left_if(format,boost::algorithm::is_any_of("*."));
    boost::algorithm::to_lower(format);
    return format;
}

//---------------------------------------------------------------

Error ConfigTreeIo::loadFromFile(
        ConfigTree &target,
        lib::string_view filename,
        const ConfigTreePath &root,
        const std::string& format
    ) const
{
    common::PlainFile file;
    file.setFilename(filename);
    return loadFromFile(target,file,root,format);
}

Error ConfigTreeIo::loadFromFile(
        ConfigTree &target,
        common::File& file,
        const ConfigTreePath &root,
        const std::string& format
    ) const
{
    //! @todo More verbose error.

    common::RunOnScopeExit closeOnExit{
        [&file](){
            file.close();
        },
        false
    };
    if (!file.isOpen())
    {
        auto ec=file.open(common::File::Mode::scan);
        if (ec)
        {
            auto err=std::make_shared<common::ErrorStack>(std::move(ec),fmt::format(_TR("failed to open file {}","base"), file.filename()));
            return baseError(BaseError::CONFIG_LOAD_ERROR,std::move(err));
        }
        closeOnExit.setEnable(true);
    }

    std::vector<char> source;
    auto ec=file.readAll(source);
    if (ec)
    {
        auto err=std::make_shared<common::ErrorStack>(std::move(ec),fmt::format(_TR("failed to read file {}","base"), file.filename()));
        return baseError(BaseError::CONFIG_LOAD_ERROR,std::move(err));
    }
    auto parseFormat=format.empty()?fileFormat(file.filename()):format;
    ec=parse(target,lib::toStringView(source),root,parseFormat);
    if (ec)
    {
        auto err=std::make_shared<common::ErrorStack>(std::move(ec),file.filename());
        return baseError(BaseError::CONFIG_LOAD_ERROR,std::move(err));
    }
    return OK;
}

//---------------------------------------------------------------

Error ConfigTreeIo::saveToFile(
        const ConfigTree &source,
        lib::string_view filename,
        const ConfigTreePath &root,
        const std::string& format
    ) const
{
    common::PlainFile file;
    file.setFilename(filename);
    return saveToFile(source,file,root,format);
}

Error ConfigTreeIo::saveToFile(
    const ConfigTree &source,
    common::File& file,
    const ConfigTreePath &root,
    const std::string& format
    ) const
{
    common::RunOnScopeExit closeOnExit{
        [&file](){
            file.close();
        },
        false
    };
    if (!file.isOpen())
    {
        auto ec=file.open(common::File::Mode::write);
        if (ec)
        {
            auto err=std::make_shared<common::ErrorStack>(std::move(ec),fmt::format(_TR("failed to open file {}","base"), file.filename()));
            return baseError(BaseError::CONFIG_SAVE_ERROR,std::move(err));
        }
        closeOnExit.setEnable(true);
    }

    auto serializeFormat=format.empty()?fileFormat(file.filename()):format;
    auto r=serialize(source,root,serializeFormat);
    if (r)
    {
        auto err=std::make_shared<common::ErrorStack>(std::move(r.takeError()),fmt::format(_TR("failed to serialize configuration tree for {}","base"), file.filename()));
        return baseError(BaseError::CONFIG_SAVE_ERROR,std::move(err));
    }

    Error ec;
    file.write(r.value().data(),r.value().size(),ec);
    if (ec)
    {
        auto err=std::make_shared<common::ErrorStack>(std::move(ec),fmt::format(_TR("failed to write to file {}","base"), file.filename()));
        return baseError(BaseError::CONFIG_SAVE_ERROR,std::move(err));
    }
    return OK;
}

//---------------------------------------------------------------

HATN_BASE_NAMESPACE_END
