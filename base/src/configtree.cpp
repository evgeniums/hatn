/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file base/configtree.сpp
  *
  *      Hatn DataUnit meta definitions.
  *
  */

#include <hatn/base/configtree.h>
#include <hatn/base/detail/configtree.ipp>
#include <hatn/base/configtreepath.h>

HATN_BASE_NAMESPACE_BEGIN

//---------------------------------------------------------------

template <typename T, typename T1>
Result<T> doGet(ConfigTreePath& key, T1* configTreePtr, size_t* depth=nullptr, T1** lastCurrent=nullptr) noexcept
{
    size_t i=0;
    auto current=configTreePtr;
    if (!key.isRoot())
    {
        key.prepare();
        for (;i<key.count();i++)
        {
            const auto& section=key.at(i);
            if (section.empty())
            {
                continue;
            }

            if (depth!=nullptr)
            {
                *depth=i;
            }
            if (lastCurrent!=nullptr)
            {
                *lastCurrent=current;
            }

            auto map=current->asMap();
            if (map.isValid())
            {
                auto it1=map->find(section);
                if (it1==map->end())
                {
                    return baseErrorResult(BaseError::VALUE_NOT_SET);
                }

                current=it1->second.get();
                continue;
            }

            if (current->type()==config_tree::Type::ArrayTree)
            {
                auto pos=key.numberAt(i);
                HATN_CHECK_RESULT(pos)

                auto view=current->template asArray<ConfigTree>();
                HATN_CHECK_RESULT(view)

                if (pos<0 || pos>=view->size())
                {
                    return ErrorResult{makeSystemError(std::errc::result_out_of_range)};
                }

                current=view->at(pos.value()).get();
                continue;
            }

            // return invalid result
            return map;
        }
    }

    if (depth!=nullptr)
    {
        *depth=i;
    }
    if (lastCurrent!=nullptr)
    {
        *lastCurrent=current;
    }
    return *current;
}

//---------------------------------------------------------------

template <typename T>
Result<ConfigTree&> buildPath(ConfigTreePath& key, T* configTreePtr, bool forceMismatchedSections) noexcept
{
    size_t depth=0;
    auto current=configTreePtr;
    doGet<ConfigTree&>(key,configTreePtr,&depth,&current);

    for (auto i=depth;i<key.count();i++)
    {
        const auto& section=key.at(i);
        if (section.empty())
        {
            continue;
        }

        if (!current->isSet() || (current->type()!=config_tree::Type::Map && current->type()!=config_tree::Type::ArrayTree && forceMismatchedSections))
        {
            current->toMap();
        }

        size_t pos=-1;
        if (current->type()==config_tree::Type::ArrayTree)
        {
            auto tryPos=key.numberAt(i);
            if (!tryPos.isValid())
            {
                if (forceMismatchedSections)
                {
                    current->toMap();
                }
                else
                {
                    return tryPos;
                }
            }
            else
            {
                pos=tryPos.value();
            }
        }

        auto map=current->asMap();
        if (map.isValid())
        {
            auto inserted=map->emplace(section, std::make_shared<ConfigTree>());
            current=inserted.first->second.get();
            continue;
        }

        if (current->type()==config_tree::Type::ArrayTree)
        {
            auto view=current->template asArray<ConfigTree>();
            HATN_CHECK_RESULT(view)

            if (pos<0 || pos>=view->size())
            {
                return ErrorResult{makeSystemError(std::errc::result_out_of_range)};
            }

            auto subtree=std::make_shared<ConfigTree>();
            current=subtree.get();
            view->set(pos,std::move(subtree));
            continue;
        }

        // return invalid result
        return map;
    }

    return *current;
}

//---------------------------------------------------------------

Result<const ConfigTree&> ConfigTree::getImpl(const common::lib::string_view& path) const noexcept
{
    ConfigTreePath key(path,pathSeparator());
    return doGet<const ConfigTree&>(key,this);
}

//---------------------------------------------------------------

const ConfigTree& ConfigTree::get(common::lib::string_view path, Error &ec) const noexcept
{
    auto&& r = getImpl(path);
    HATN_RESULT_EC(r,ec)
    return r.takeValue();
}

//---------------------------------------------------------------

const ConfigTree& ConfigTree::getEx(common::lib::string_view path) const
{
    auto&& r = getImpl(path);
    HATN_RESULT_THROW(r)
    return r.takeValue();
}

//---------------------------------------------------------------

Result<ConfigTree&> ConfigTree::getImpl(const common::lib::string_view& path, bool autoCreatePath) noexcept
{
    ConfigTreePath key(path,pathSeparator());
    auto result=doGet<ConfigTree&>(key,this);
    if (!result.isValid()&&autoCreatePath)
    {
        return buildPath(key,this,true);
    }
    return result;
}

//---------------------------------------------------------------

ConfigTree& ConfigTree::get(common::lib::string_view path, Error &ec, bool autoCreatePath) noexcept
{
    auto&& r = getImpl(path,autoCreatePath);
    HATN_RESULT_EC(r,ec)
    return r.takeValue();
}

//---------------------------------------------------------------

ConfigTree& ConfigTree::getEx(common::lib::string_view path, bool autoCreatePath)
{
    auto&& r = getImpl(path,autoCreatePath);
    HATN_RESULT_THROW(r)
    return r.takeValue();
}

//---------------------------------------------------------------

bool ConfigTree::isSet(common::lib::string_view path) const noexcept
{
    ConfigTreePath key(path,pathSeparator());
    auto r=doGet<const ConfigTree&>(key,this);
    if (!r.isValid())
    {
        return false;
    }
    return r->isSet();
}

//---------------------------------------------------------------

void ConfigTree::reset(common::lib::string_view path) noexcept
{
    ConfigTreePath key(path,pathSeparator());
    auto r=doGet<ConfigTree&>(key,this);
    if (r.isValid())
    {
        r->reset();
    }
}

//---------------------------------------------------------------

config_tree::MapT& ConfigTree::toMap(common::lib::string_view path)
{
    auto r=getImpl(path,true);
    return r->toMap();
}

//---------------------------------------------------------------

HATN_BASE_NAMESPACE_END
