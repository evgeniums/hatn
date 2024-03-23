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

#include <hatn/common/runonscopeexit.h>
#include <hatn/common/filesystem.h>

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

    auto formatIt=map.find("format");
    if (formatIt!=map.end())
    {
        auto format=formatIt->second->as<std::string>();
        HATN_CHECK_RESULT(format)
        val.format=format.takeValue();
    }

    auto extendIt=map.find("extend");
    if (extendIt!=map.end())
    {
        auto extend=extendIt->second->as<bool>();
        HATN_CHECK_RESULT(extend)
        val.extend=extend.value();
    }

    auto mergeIt=map.find("merge");
    if (mergeIt!=map.end())
    {
        auto mergeStr=extendIt->second->as<std::string>();
        HATN_CHECK_RESULT(mergeStr)
        auto merge=config_tree::arrayMerge(mergeStr.value());
        HATN_CHECK_EC(merge)
        val.merge=merge.value();
    }

    return val;
}

namespace {

Error parseIncludes(const lib::string_view& filename, const ConfigTree &value, const ConfigTreePath& path, std::vector<ConfigTreeInclude>& includes)
{
    auto makeError=[&filename](const ConfigTreePath& path)
    {
        auto msg=fmt::format("invalid format of include at path {} in file {}", path.path(), filename);
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
                return makeError(p);
            }

            auto include=ConfigTreeInclude::fromMap(m.value());
            if (include)
            {
                return makeError(p);
            }
            includes.emplace_back(include.takeValue());
        }
    }
    else if (value.type()==config_tree::Type::Map)
    {
        auto m=value.asMap();
        if (m)
        {
            return makeError(path);
        }

        auto include=ConfigTreeInclude::fromMap(m.value());
        if (include)
        {
            return makeError(path);
        }
        includes.emplace_back(include.takeValue());
    }
    else
    {
        return makeError(path);
    }

    return OK;
}

Error loadNext(const ConfigTreeLoader& loader, ConfigTree &current, const ConfigTreeInclude& descriptor, std::set<std::string> chain, const lib::filesystem::path& parentPath)
{
    // find handler for format
    auto format=ConfigTreeIo::fileFormat(descriptor.format);
    std::shared_ptr<ConfigTreeIo> handler{loader.handler(format)};
    if (!handler)
    {
        auto msg=fmt::format("unsupported config format \"{}\" of file {}", format, descriptor.file);
        return Error{BaseError::CONFIG_PARSE_ERROR,std::make_shared<ConfigTreeParseError>(msg)};
    }

    // find include file using include dirs if required
    auto filename=descriptor.file;
    auto fileNotFound=[&filename]()
    {
        auto msg=fmt::format("file not found {}", filename);
        return Error{BaseError::CONFIG_PARSE_ERROR,std::make_shared<ConfigTreeParseError>(msg)};
    };
    auto currentPath=lib::filesystem::path(filename);
    if (currentPath.is_absolute())
    {
        if (!lib::filesystem::exists(currentPath))
        {
            return fileNotFound();
        }
    }
    else
    {
        bool found=false;
        lib::filesystem::path newPath{parentPath};
        newPath/=currentPath;
        if (!lib::filesystem::exists(newPath))
        {
            currentPath=std::move(newPath);
            found=true;
        }
        else
        {
            for (auto&& it:loader.includeDirs())
            {
                lib::filesystem::path newPath{it};
                newPath/=currentPath;
                if (!lib::filesystem::exists(newPath))
                {
                    currentPath=std::move(newPath);
                    found=true;
                    break;
                }
            }
        }
        if (!found)
        {
            return fileNotFound();
        }
    }
    filename=currentPath.string();

    // check for cycles
    if (chain.find(filename)!=chain.end())
    {
        auto msg=fmt::format("include cycle detected for file {}", filename);
        return Error{BaseError::CONFIG_PARSE_ERROR,std::make_shared<ConfigTreeParseError>(msg)};
    }
    chain.insert(filename);
    common::RunOnScopeExit onExit{
        [&chain,&filename]()
        {
            chain.erase(filename);
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
        current.reset(loader.includeTag());
    }

    // process top includes
    ConfigTree merged;
    auto currentMergeMode=config_tree::ArrayMerge::Merge;
    for (auto&& it:topIncludes)
    {
        if (!it.file.empty())
        {
            ConfigTree next;
            HATN_CHECK_RETURN(loadNext(loader,next,it,chain,currentPath));
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
            HATN_CHECK_RETURN(loadNext(loader,next,it1,chain,currentPath));
            HATN_CHECK_RETURN(current.merge(std::move(next),it.first,it1.merge))
        }
    }

    // done
    return OK;
}

} // anonymous namsepace

Error ConfigTreeLoader::loadFromFile(ConfigTree &target, std::string filename, const ConfigTreePath &root, const std::string &format) const
{
    ConfigTree next;
    ConfigTreeInclude descriptor{std::move(filename)};
    descriptor.format=format;
    std::set<std::string> chain;
    HATN_CHECK_RETURN(loadNext(*this,next,descriptor,chain,lib::filesystem::current_path()))
    return target.merge(std::move(next),root);
}

//---------------------------------------------------------------

HATN_BASE_NAMESPACE_END
