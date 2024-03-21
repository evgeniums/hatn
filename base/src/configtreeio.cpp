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

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/case_conv.hpp>

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

Error ConfigTreeIo::loadFile(
        ConfigTree &target,
        lib::string_view filename,
        const ConfigTreePath &root,
        const std::string& format
    )
{
    common::PlainFile file;
    file.setFilename(filename);
    return loadFile(target,file,root,format);
}

Error ConfigTreeIo::loadFile(
        ConfigTree &target,
        common::File& file,
        const ConfigTreePath &root,
        const std::string& format
    )
{
    common::RunOnScopeExit closeOnExit{
        [&file](){
            file.close();
        },
        false
    };
    if (!file.isOpen())
    {
        HATN_CHECK_RETURN(file.open(common::File::Mode::scan))
        closeOnExit.setEnable(true);
    }

    std::string source;
    HATN_CHECK_RETURN(file.readAll(source))
    auto parseFormat=format.empty()?fileFormat(file.filename()):format;
    return parse(target,source,root,parseFormat);
}

//---------------------------------------------------------------

HATN_BASE_NAMESPACE_END
