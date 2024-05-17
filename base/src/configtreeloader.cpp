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

#include <boost/algorithm/string.hpp>

#include <hatn/common/runonscopeexit.h>
#include <hatn/common/filesystem.h>
#include <hatn/common/translate.h>

#include <hatn/base/baseerror.h>
#include <hatn/base/configtreejson.h>
#include <hatn/base/configtreeloader.h>

HATN_BASE_NAMESPACE_BEGIN

//---------------------------------------------------------------

std::string ConfigTreeLoader::DefaultFormat="jsonc";
std::string ConfigTreeLoader::DefaultIncludeTag="#include";
std::string ConfigTreeLoader::DefaultPathSeparator=".";
std::string ConfigTreeLoader::DefaultRelFilePathPrefix="$rel/";

//---------------------------------------------------------------

ConfigTreeLoader::ConfigTreeLoader(
    ) : m_defaultFormat(DefaultFormat),
    m_includeTag(DefaultIncludeTag),
    m_separator(DefaultPathSeparator),
    m_relFilePathPrefix(DefaultRelFilePathPrefix)
{
    addHandler(std::make_shared<ConfigTreeJson>());
}

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

Result<ConfigTreeInclude> ConfigTreeInclude::fromMap(const config_tree::MapT &map)
{
    auto fileIt=map.find("file");
    if (fileIt==map.end())
    {
        return baseError(BaseError::CONFIG_PARSE_ERROR);
    }
    auto file=fileIt->second->as<std::string>();
    HATN_CHECK_RESULT(file)
    ConfigTreeInclude val{file.takeValue()};

    {
        auto formatIt=map.find("format");
        if (formatIt!=map.end())
        {
            auto format=formatIt->second->as<std::string>();
            HATN_CHECK_RESULT(format)
            val.format=format.takeValue();
        }
    }

    {
        auto extendIt=map.find("extend");
        if (extendIt!=map.end())
        {
            auto extend=extendIt->second->as<bool>();
            HATN_CHECK_RESULT(extend)
            val.extend=extend.value();
        }
    }

    {
        auto mergeIt=map.find("merge");
        if (mergeIt!=map.end())
        {
            auto mergeStr=mergeIt->second->as<std::string>();
            HATN_CHECK_RESULT(mergeStr)
            auto merge=config_tree::arrayMerge(mergeStr.value());
            HATN_CHECK_EC(merge)
            val.merge=merge.value();
        }
    }

    return val;
}

namespace {

Error parseIncludes(const lib::string_view& filename, const ConfigTree &value, const ConfigTreePath& path, std::vector<ConfigTreeInclude>& includes)
{
    auto makeError=[&filename](const ConfigTreePath& path, const Error& origin)
    {
        auto msg=fmt::format(_TR("invalid format of include: {} at path \"{}\" in file \"{}\"","base"), origin.message(), path.path(), filename);
        return Error{BaseError::CONFIG_PARSE_ERROR,std::make_shared<ConfigTreeParseError>(msg)};
    };

    if (value.type()==config_tree::Type::String)
    {
        auto file=value.as<std::string>();
        HATN_CHECK_RESULT(file)
        includes.emplace_back(file.takeValue());
    }
    else if (value.type()==config_tree::Type::ArrayString)
    {
        auto arr=value.asArray<std::string>();
        HATN_CHECK_RESULT(arr)
        for (size_t i=0;i<arr->size();i++)
        {
            includes.emplace_back(arr->at(i));
        }
    }
    else if (value.type()==config_tree::Type::ArrayTree)
    {
        auto arr=value.asArray<ConfigTree>();
        HATN_CHECK_RESULT(arr)
        for (size_t i=0;i<arr->size();i++)
        {
            auto p=path.copyAppend(i);
            const auto& m=arr->at(i)->asMap();
            if (m)
            {
                return makeError(p,m.error());
            }

            auto include=ConfigTreeInclude::fromMap(m.value());
            if (include)
            {
                return makeError(p,include.error());
            }
            includes.emplace_back(include.takeValue());
        }
    }
    else if (value.type()==config_tree::Type::Map)
    {
        auto m=value.asMap();
        if (m)
        {
            return makeError(path,m.error());
        }

        auto include=ConfigTreeInclude::fromMap(m.value());
        if (include)
        {
            return makeError(path,include.error());
        }
        includes.emplace_back(include.takeValue());
    }
    else
    {
        return makeError(path, baseError(BaseError::INVALID_TYPE));
    }

    return OK;
}

Error loadNext(const ConfigTreeLoader& loader, ConfigTree &current, const ConfigTreeInclude& descriptor, std::vector<std::string> chain)
{
    // find handler for format
    auto format=ConfigTreeIo::fileFormat(descriptor.format);
    std::shared_ptr<ConfigTreeIo> handler{loader.handler(format)};
    if (!handler)
    {
        auto msg=fmt::format(_TR("unsupported config format \"{}\" of file {}","base"), format, descriptor.file);
        return Error{BaseError::CONFIG_PARSE_ERROR,std::make_shared<ConfigTreeParseError>(msg)};
    }

    // find include file using dirs of parent files and then include dirs
    auto filename=descriptor.file;
    auto fileNotFound=[&filename,&chain]()
    {
        std::string msg;
        if (chain.empty())
        {
            msg=fmt::format(_TR("file not found: {}","base"), filename);
        }
        else
        {
            msg=fmt::format(_TR("include file not found: {}","base"), filename);
        }
        return Error{BaseError::CONFIG_PARSE_ERROR,std::make_shared<ConfigTreeParseError>(msg)};
    };
    auto currentPath=lib::filesystem::path(filename).lexically_normal();
    if (!lib::filesystem::exists(currentPath))
    {
        if (currentPath.is_absolute())
        {
            return fileNotFound();
        }
        else
        {
            bool found=false;

            std::vector<std::string> includeDirs;
            std::set<std::string> uniqueIncludeDirs;
            if (chain.empty())
            {
                includeDirs.push_back(lib::filesystem::current_path().string());
                uniqueIncludeDirs.insert(lib::filesystem::current_path().string());
            }
            else
            {
                for (auto it=chain.crbegin();it!=chain.crend();++it)
                {
                    lib::filesystem::path p{*it};
                    auto dir=p.parent_path().string();
                    if (uniqueIncludeDirs.find(dir)==uniqueIncludeDirs.end())
                    {
                        includeDirs.push_back(dir);
                        uniqueIncludeDirs.insert(dir);
                    }
                }
                for (auto it=loader.includeDirs().begin();it!=loader.includeDirs().end();++it)
                {
                    if (uniqueIncludeDirs.find(*it)==uniqueIncludeDirs.end())
                    {
                        includeDirs.push_back(*it);
                        uniqueIncludeDirs.insert(*it);
                    }
                }
            }

            for (auto&& it:includeDirs)
            {
                lib::filesystem::path newPath{it};
                newPath/=currentPath;
                if (lib::filesystem::exists(newPath))
                {
                    currentPath=newPath.lexically_normal();
                    found=true;
                    break;
                }
            }
            if (!found)
            {
                return fileNotFound();
            }
        }
    }
    filename=currentPath.string();

    // check for cycles
    if (std::find(chain.begin(), chain.end(), filename)!=chain.end())
    {
        auto msg=fmt::format(_TR("include cycle detected for file {}","base"), filename);
        return Error{BaseError::CONFIG_PARSE_ERROR,std::make_shared<ConfigTreeParseError>(msg)};
    }
    chain.push_back(filename);
    common::RunOnScopeExit onExit{
        [&chain]()
        {
            chain.pop_back();
        }
    };
    std::ignore=onExit;

    // load config
    HATN_CHECK_RETURN(handler->loadFromFile(current,filename,ConfigTreePath(),format))

    // find includes in the config
    std::vector<ConfigTreeInclude> topIncludes;
    std::map<ConfigTreePath,std::vector<ConfigTreeInclude>> nestedIncludes;
    auto keysHandler=[&loader,&topIncludes,&nestedIncludes,&filename,&current](const ConfigTreePath& path, const ConfigTree& value)
    {
        if (path.back()==loader.includeTag())
        {
            if (path.count()>1)
            {
                std::vector<ConfigTreeInclude> includes;
                HATN_CHECK_RETURN(parseIncludes(filename,value,path,includes))
                nestedIncludes[path.copyDropBack()]=std::move(includes);
                // remove include tag from config
                current.reset(path);
                return Error();
            }
            return parseIncludes(filename,value,path,topIncludes);
        }
        return Error();
    };
    ConfigTreePath root;
    root.setSeparator(loader.pathSeparator());
    HATN_CHECK_RETURN(current.each(keysHandler,root))

    // remove top includes from config
    if (!topIncludes.empty())
    {
        current.remove(loader.includeTag());
    }

    // process top includes
    ConfigTree merged;
    auto currentMergeMode=config_tree::ArrayMerge::Merge;
    for (auto&& it:topIncludes)
    {
        if (!it.file.empty())
        {
            ConfigTree next;
            HATN_CHECK_RETURN(loadNext(loader,next,it,chain));
            if (!it.extend)
            {
                HATN_CHECK_RETURN(merged.merge(std::move(next),ConfigTreePath(),it.merge))
            }
            else
            {
                HATN_CHECK_RETURN(current.merge(std::move(next),ConfigTreePath(),it.merge))
            }
        }
        else
        {
            currentMergeMode=it.merge;
        }
    }
    if (merged.isSet(true))
    {
        HATN_CHECK_RETURN(merged.merge(std::move(current),ConfigTreePath(),currentMergeMode))
        current=std::move(merged);
    }

    // process nested includes
    for (auto&& it:nestedIncludes)
    {
        for (auto&& it1:it.second)
        {
            ConfigTree next;
            HATN_CHECK_RETURN(loadNext(loader,next,it1,chain));
            HATN_CHECK_RETURN(current.merge(std::move(next),it.first,it1.merge))
        }
    }

    // done
    return OK;
}

} // anonymous namsepace

Error ConfigTreeLoader::loadFromFile(ConfigTree &target, const std::string& filename, const ConfigTreePath &root, const std::string &format) const
{
    // load config tree
    ConfigTree next;
    ConfigTreeInclude descriptor{filename};
    descriptor.format=format;
    std::vector<std::string> chain;
    HATN_CHECK_RETURN(loadNext(*this,next,descriptor,chain))
    HATN_CHECK_RETURN(target.merge(std::move(next),root))

    // process relative paths in string parameters
    if (!m_relFilePathPrefix.empty())
    {
        lib::filesystem::path filePath{filename};
        auto rootFilePath=lib::filesystem::canonical(filePath.parent_path());
        auto substitute=rootFilePath.string();
        if (boost::algorithm::ends_with(m_relFilePathPrefix,"/"))
        {
            substitute+=lib::filesystem::path::preferred_separator;
        }

        auto handler=[this,&substitute](const ConfigTreePath&, ConfigTree& value)
        {
            if (value.type(true)==config_tree::Type::String)
            {
                auto str=value.asString();
                if (str.isValid())
                {
                    if (boost::algorithm::starts_with(str.value(),m_relFilePathPrefix))
                    {
                        value.set(boost::algorithm::replace_first_copy(str.value(),m_relFilePathPrefix,substitute));
                    }
                }
            }
            else if (value.type(true)==config_tree::Type::ArrayString)
            {
                auto arr=value.asArray<std::string>();
                if (arr.isValid())
                {
                    for (size_t i=0;i<arr->size();i++)
                    {
                        if (boost::algorithm::starts_with(arr->at(i),m_relFilePathPrefix))
                        {
                            boost::algorithm::replace_first(arr->at(i),m_relFilePathPrefix,substitute);
                        }
                    }
                }
            }

            return Error();
        };
        std::ignore=target.each(handler);
    }
    return OK;
}

//---------------------------------------------------------------

Result<ConfigTree> ConfigTreeLoader::createFromFile(const std::string& filename, const std::string &format) const
{
    ConfigTree t;
    HATN_CHECK_RETURN(loadFromFile(t,filename,ConfigTreePath(),format))
    return t;
}

//---------------------------------------------------------------

HATN_BASE_NAMESPACE_END
