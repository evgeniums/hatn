/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file base/configtreeloader.сpp
  *
  *  Containse implementation of config tree loader.
  *
  */

#include <hatn/base/baseerror.h>
#include <hatn/base/configtreeloader.h>

HATN_BASE_NAMESPACE_BEGIN

//---------------------------------------------------------------

std::string ConfigTreeLoader::DefaultFormat="jsonc";
std::string ConfigTreeLoader::DefaultIncludeTag="#include";
std::string ConfigTreeLoader::DefaultPathSeparator=".";
std::string ConfigTreeLoader::DefaultRelFilePathPrefix="$rel/";

//---------------------------------------------------------------

void ConfigTreeLoader::addHandler(std::shared_ptr<ConfigTreeIo> handler)
{
    for (auto&& it: handler->formats())
    {
        m_handlers[it]=handler;
    }
}

//---------------------------------------------------------------

std::shared_ptr<ConfigTreeIo> ConfigTreeLoader::handler(const std::string& format) const noexcept
{
    auto find=[this](const std::string& format)
    {
        auto it=m_handlers.find(format);
        if (it==m_handlers.end())
        {
            return std::shared_ptr<ConfigTreeIo>();
        }
        return it->second;
    };

    if (format.empty())
    {
        return find(m_defaultFormat);
    }

    return find(format);
}

//---------------------------------------------------------------

Error ConfigTreeLoader::saveToFile(const ConfigTree &source, lib::string_view filename, const ConfigTreePath &root, const std::string &format) const
{
    std::shared_ptr<ConfigTreeIo> handler{this->handler(format)};
    if (!handler)
    {
        return baseError(BaseError::UNSUPPORTED_CONFIG_FORMAT);
    }
    return handler->saveToFile(source,filename,root,format);
}

//---------------------------------------------------------------

Error ConfigTreeLoader::loadFromFile(ConfigTree &target, lib::string_view filename, const ConfigTreePath &root, const std::string &format) const
{
    std::shared_ptr<ConfigTreeIo> handler{this->handler(format)};
    if (!handler)
    {
        return baseError(BaseError::UNSUPPORTED_CONFIG_FORMAT);
    }
    return CommonError::NOT_IMPLEMENTED;
}

//---------------------------------------------------------------

HATN_BASE_NAMESPACE_END
