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

HATN_BASE_NAMESPACE_BEGIN

//---------------------------------------------------------------

template <typename T, typename T1>
Result<T> doGet(common::lib::string_view path, T1* configTreePtr) noexcept
{
    auto current=configTreePtr;
    if (path.length()!=0)
    {
        std::vector<std::string> sections;
        boost::algorithm::split(sections, path, boost::algorithm::is_any_of(configTreePtr->pathSeparator()));

        for (auto&& it: sections)
        {
            auto child=current->asMap();
            HATN_CHECK_RESULT(child)

            auto it1=child->find(it);
            if (it1==child->end())
            {
                return baseErrorResult(BaseError::VALUE_NOT_SET);
            }

            current=it1->second.get();
        }
    }

    return *current;
}

//---------------------------------------------------------------

template <typename T>
Result<ConfigTree&> createPath(common::lib::string_view path, T* configTreePtr, bool forceMismatchedSections) noexcept
{
    auto current=configTreePtr;
    if (path.length()!=0)
    {
        std::vector<std::string> sections;
        boost::algorithm::split(sections, path, boost::algorithm::is_any_of(configTreePtr->pathSeparator()));

        for (auto&& it: sections)
        {
            if (!current->isSet())
            {
                current->toMap();
            }
            else if (current->type()!=config_tree::Type::Map)
            {
                if (forceMismatchedSections)
                {
                    current->toMap();
                }
            }

            auto child=current->asMap();
            HATN_CHECK_RESULT(child)

            auto it1=child->find(it);
            if (it1!=child->end())
            {
                current=it1->second.get();
            }
            else
            {
                auto inserted=child->emplace(it, std::make_shared<ConfigTree>());
                current=inserted.first->second.get();
            }
        }
    }

    return *current;
}

//---------------------------------------------------------------

Result<const ConfigTree&> ConfigTree::get(common::lib::string_view path) const noexcept
{
    return doGet<const ConfigTree&>(path,this);
}

//---------------------------------------------------------------

Result<ConfigTree&> ConfigTree::get(common::lib::string_view path, bool autoCreatePath) noexcept
{
    auto result=doGet<ConfigTree&>(path,this);
    if (!result.isValid()&&autoCreatePath)
    {
        return createPath(path,this,true);
    }
    return result;
}

//---------------------------------------------------------------

bool ConfigTree::isSet(common::lib::string_view path) const noexcept
{
    auto result=doGet<const ConfigTree&>(path,this);
    if (!result.isValid())
    {
        return false;
    }
    return result->isSet();
}

//---------------------------------------------------------------

HATN_BASE_NAMESPACE_END
