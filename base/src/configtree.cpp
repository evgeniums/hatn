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
  *      Defines ConfigTree class.
  */

#include <hatn/base/configtree.h>
#include <hatn/base/detail/configtree.ipp>
#include <hatn/base/configtreepath.h>

HATN_BASE_NAMESPACE_BEGIN

//---------------------------------------------------------------

template <typename T, typename T1>
Result<T> doGet(const ConfigTreePath& path, T1* configTreePtr, size_t* depth=nullptr, T1** lastCurrent=nullptr) noexcept
{
    size_t i=0;
    auto current=configTreePtr;
    if (!path.isRoot())
    {
        for (;i<path.count();i++)
        {
            const auto& section=path.at(i);
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
                auto pos=path.numberAt(i);
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
Result<ConfigTree&> buildPath(const ConfigTreePath& path, T* configTreePtr, bool forceMismatchedSections) noexcept
{
    size_t depth=0;
    auto current=configTreePtr;
    doGet<ConfigTree&>(path,configTreePtr,&depth,&current);

    for (auto i=depth;i<path.count();i++)
    {
        const auto& section=path.at(i);
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
            auto tryPos=path.numberAt(i);
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

Result<const ConfigTree&> ConfigTree::getImpl(const ConfigTreePath& path) const noexcept
{
    return doGet<const ConfigTree&>(path,this);
}

//---------------------------------------------------------------

const ConfigTree& ConfigTree::get(const ConfigTreePath& path, Error &ec) const noexcept
{
    auto&& r = getImpl(path);
    HATN_RESULT_EC(r,ec)
    return r.takeValue();
}

//---------------------------------------------------------------

const ConfigTree& ConfigTree::getEx(const ConfigTreePath& path) const
{
    auto&& r = getImpl(path);
    HATN_RESULT_THROW(r)
    return r.takeValue();
}

//---------------------------------------------------------------

Result<ConfigTree&> ConfigTree::getImpl(const ConfigTreePath& path, bool autoCreatePath) noexcept
{
    auto result=doGet<ConfigTree&>(path,this);
    if (!result.isValid()&&autoCreatePath)
    {
        return buildPath(path,this,true);
    }
    return result;
}

//---------------------------------------------------------------

ConfigTree& ConfigTree::get(const ConfigTreePath& path, Error &ec, bool autoCreatePath) noexcept
{
    auto&& r = getImpl(path,autoCreatePath);
    HATN_RESULT_EC(r,ec)
    return r.takeValue();
}

//---------------------------------------------------------------

ConfigTree& ConfigTree::getEx(const ConfigTreePath& path, bool autoCreatePath)
{
    auto&& r = getImpl(path,autoCreatePath);
    HATN_RESULT_THROW(r)
    return r.takeValue();
}

//---------------------------------------------------------------

bool ConfigTree::isSet(const ConfigTreePath& path) const noexcept
{
    auto r=doGet<const ConfigTree&>(path,this);
    if (!r.isValid())
    {
        return false;
    }
    return r->isSet();
}

//---------------------------------------------------------------

void ConfigTree::reset(const ConfigTreePath& path) noexcept
{
    auto r=doGet<ConfigTree&>(path,this);
    if (r.isValid())
    {
        r->reset();
    }
}

//---------------------------------------------------------------

HATN_BASE_NAMESPACE_END
