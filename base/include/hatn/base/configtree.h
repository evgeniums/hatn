/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file base/configtree.h
  *
  * Contains declarations of configuration tree class.
  *
  */

/****************************************************************************/

#ifndef HATNCONFIGTREE_H
#define HATNCONFIGTREE_H

#include <hatn/common/error.h>

#include <hatn/base/base.h>
#include <hatn/base/baseerror.h>
#include <hatn/base/configtreevalue.h>
#include <hatn/base/configtreepath.h>
#include <hatn/base/detail/configtreevalue.ipp>

HATN_BASE_NAMESPACE_BEGIN

 /**
 * @brief The ConfigTree class represents a tree of values that can be accessed with literal nested path.
 */
class HATN_BASE_EXPORT ConfigTree : public ConfigTreeValue
{
    public:

        ConfigTree()=default;
        ConfigTree(ConfigTree&&)=default;
        ConfigTree& operator =(ConfigTree&&)=default;
        ~ConfigTree()=default;

        ConfigTree(const ConfigTree&)=delete;
        ConfigTree& operator =(const ConfigTree&)=delete;

        using ConfigTreeValue::set;
        using ConfigTreeValue::setDefault;
        using ConfigTreeValue::toArray;
        using ConfigTreeValue::toMap;
        using ConfigTreeValue::reset;
        using ConfigTreeValue::isSet;

        template <typename T>
        Result<ConfigTree&> set(const ConfigTreePath& path, T&& value, bool autoCreatePath=true) noexcept
        {
            auto r=getImpl(path,autoCreatePath);
            HATN_CHECK_RESULT(r)
            r->set(std::forward<T>(value));
            return r;
        }

        template <typename T>
        ConfigTree& set(const ConfigTreePath& path, T&& value, Error &ec, bool autoCreatePath=true) noexcept
        {
            auto&& r=getImpl(path,autoCreatePath);
            HATN_RESULT_EC(r,ec)
            r->set(std::forward<T>(value));
            return r.takeValue();
        }

        template <typename T>
        ConfigTree& setEx(const ConfigTreePath& path, T&& value, bool autoCreatePath=true)
        {
            auto&& r=getImpl(path,autoCreatePath);
            HATN_RESULT_THROW(r)
            r->set(std::forward<T>(value));
            return r.takeValue();
        }

        template <typename T>
        Result<ConfigTree&> setDefault(const ConfigTreePath& path, T&& value, bool autoCreatePath=true) noexcept
        {
            auto r=getImpl(path,autoCreatePath);
            HATN_CHECK_RESULT(r)
            r->setDefault(std::forward<T>(value));
            return r;
        }

        template <typename T>
        ConfigTree& setDefault(const ConfigTreePath& path, T&& value, Error &ec, bool autoCreatePath=true) noexcept
        {
            auto&& r=getImpl(path,autoCreatePath);
            HATN_RESULT_EC(r,ec)
            r->setDefault(std::forward<T>(value));
            return r.takeValue();
        }

        template <typename T>
        ConfigTree& setDefaultEx(const ConfigTreePath& path, T&& value, bool autoCreatePath=true)
        {
            auto&& r=getImpl(path,autoCreatePath);
            HATN_RESULT_THROW(r)
            r->setDefault(std::forward<T>(value));
            return r.takeValue();
        }

        Result<const ConfigTree&> get(const ConfigTreePath& path) const noexcept
        {
            return getImpl(path);
        }
        const ConfigTree& get(const ConfigTreePath& path, Error &ec) const noexcept;
        const ConfigTree& getEx(const ConfigTreePath& path) const;

        Result<ConfigTree&> get(const ConfigTreePath& path, bool autoCreatePath=false) noexcept
        {
            return getImpl(path,autoCreatePath);
        }
        ConfigTree& get(const ConfigTreePath& path, Error &ec, bool autoCreatePath=false) noexcept;
        ConfigTree& getEx(const ConfigTreePath& path, bool autoCreatePath=false);

        bool isSet(const ConfigTreePath& path) const noexcept;

        template <typename T>
        Result<ArrayView<T>> toArray(const ConfigTreePath& path)
        {
            auto r=get(path,true);
            HATN_CHECK_RESULT(r)
            return r->toArray<T>();
        }

        Result<config_tree::MapT&> toMap(const ConfigTreePath& path)
        {
            auto r=get(path,true);
            HATN_CHECK_RESULT(r)
            return r->toMap();
        }

        void reset(const ConfigTreePath& path) noexcept;

        // Error merge(const ConfigTree& other, const ConfigTreePath& root=ConfigTreePath());

    private:

        Result<const ConfigTree&> getImpl(const ConfigTreePath& path) const noexcept;
        Result<ConfigTree&> getImpl(const ConfigTreePath& path, bool autoCreatePath=false) noexcept;
};

HATN_BASE_NAMESPACE_END

#endif // HATNCONFIGTREE_H
