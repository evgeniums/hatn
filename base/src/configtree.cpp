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

                if (pos.value()<0 || pos.value()>=view->size())
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
            auto inserted=map->emplace(section, config_tree::makeTree());
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

            auto subtree=config_tree::makeTree();
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

template <typename T>
Error mergeArrays(ConfigTree* current, ConfigTree &&other, config_tree::ArrayMerge arrayMergeMode)
{
    auto otherArr=other.asArray<T>();
    HATN_CHECK_RESULT(otherArr)
    auto currentArr=current->asArray<T>();
    HATN_CHECK_RESULT(currentArr)
    return currentArr->merge(std::move(otherArr.value()),arrayMergeMode);
}

Error ConfigTree::merge(ConfigTree &&other, const ConfigTreePath &root, config_tree::ArrayMerge arrayMergeMode)
{
    // figure out where to merge to
    auto current=this;
    if (!root.isRoot())
    {
        auto subtree=get(root,true);
        HATN_CHECK_RESULT(subtree)
        current=&subtree.value();
    }

    // maybe single replace is enough
    if (!current->isSet(true))
    {
        *current=std::move(other);
        return OK;
    }

    // merge default value
    if (other.isDefaultSet())
    {
        current->setDefaultValue(std::move(other.defaultValue()));
    }

    // merge value
    if (other.isSet())
    {
        if (!current->isSet(true) || config_tree::isScalar(other.type()) || other.type()!=current->type())
        {
            // override value if it is not set or is of mismatched type
            current->setValue(std::move(other.value()));
        }
        else if (config_tree::isMap(other.type()))
        {
            // merge maps
            auto otherM=other.asMap();
            HATN_CHECK_RESULT(otherM)
            auto currentM=current->asMap();
            HATN_CHECK_RESULT(currentM)
            for (auto&& otherIt: otherM.value())
            {
                auto currentIt=currentM->find(otherIt.first);
                if (currentIt==currentM->end() || !currentIt->second->isSet(true) || config_tree::isScalar(otherIt.second->type()) || otherIt.second->type()!=currentIt->second->type())
                {
                    // insert or override element in current map
                    (*currentM)[otherIt.first]=std::move(otherIt.second);
                }
                else
                {
                    // merge subtree
                    currentIt->second->merge(std::move(*(otherIt.second)),ConfigTreePath(),arrayMergeMode);
                }
            }
        }
        else if (config_tree::isArray(other.type()))
        {
            // merge arrays
            switch (other.type())
            {
                case(Type::ArrayInt):
                {
                    HATN_CHECK_RETURN(mergeArrays<int64_t>(current,std::move(other),arrayMergeMode))
                    break;
                }
                case(Type::ArrayString):
                {
                    HATN_CHECK_RETURN(mergeArrays<std::string>(current,std::move(other),arrayMergeMode))
                    break;
                }
                case(Type::ArrayDouble):
                {
                    HATN_CHECK_RETURN(mergeArrays<double>(current,std::move(other),arrayMergeMode))
                    break;
                }
                case(Type::ArrayBool):
                {
                    HATN_CHECK_RETURN(mergeArrays<bool>(current,std::move(other),arrayMergeMode))
                    break;
                }
                case(Type::ArrayTree):
                {
                    HATN_CHECK_RETURN(mergeArrays<ConfigTree>(current,std::move(other),arrayMergeMode))
                    break;
                }
                default:break;
            }
        }
    }

    // done
    return OK;
}

//---------------------------------------------------------------

HATN_BASE_NAMESPACE_END
