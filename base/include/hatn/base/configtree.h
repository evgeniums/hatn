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

#include <functional>

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
        using ConfigTreeValue::resetDefault;
        using ConfigTreeValue::isSet;
        using ConfigTreeValue::isDefaultSet;

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

        bool isSet(const ConfigTreePath& path, bool orDefault=false) const noexcept;
        bool isSet(const char* path, bool orDefault=false) const noexcept
        {
            return isSet(ConfigTreePath(path),orDefault);
        }

        bool isDefaultSet(const ConfigTreePath& path) const noexcept;
        bool isDefaultSet(const char* path) const noexcept
        {
            return isDefaultSet(ConfigTreePath(path));
        }

        template <typename T>
        Result<ArrayView<T>> toArray(const ConfigTreePath& path)
        {
            auto r=get(path,true);
            HATN_CHECK_RESULT(r)
            return r->toArray<T>();
        }

        template <typename T>
        Result<ConstArrayView<T>> toArray(const ConfigTreePath& path) const
        {
            auto r=get(path);
            HATN_CHECK_RESULT(r)
            return r->asArray<T>();
        }

        Result<config_tree::MapT&> toMap(const ConfigTreePath& path)
        {
            auto r=get(path,true);
            HATN_CHECK_RESULT(r)
            return r->toMap();
        }

        void reset(const ConfigTreePath& path) noexcept;
        void resetDefault(const ConfigTreePath& path) noexcept;

        void remove(const ConfigTreePath& path) noexcept;

        /**
         * @brief Merge other tree moving its content to this tree if applicable.
         * @param other Other tree to move into this.
         * @param root Root path to attach the other tree to.
         * @return Operation status.
         */
        Error merge(ConfigTree&& other, const ConfigTreePath& root=ConfigTreePath(), config_tree::ArrayMerge arrayMergeMode=config_tree::ArrayMerge::Merge);

        Error each(const std::function<Error (const ConfigTreePath&,const ConfigTree&)>& handler, const ConfigTreePath& root=ConfigTreePath()) const;
        Error each(const std::function<Error (const ConfigTreePath&,ConfigTree&)>& handler, const ConfigTreePath& root=ConfigTreePath());

        Result<std::vector<std::string>> allKeys(const ConfigTreePath& root=ConfigTreePath()) const;

        Error copy(const ConfigTree& source, const ConfigTreePath& sourceRoot=ConfigTreePath(), const ConfigTreePath& targetRoot=ConfigTreePath());

    private:

        Result<const ConfigTree&> getImpl(const ConfigTreePath& path) const noexcept;
        Result<ConfigTree&> getImpl(const ConfigTreePath& path, bool autoCreatePath=false) noexcept;
};

HATN_BASE_NAMESPACE_END

#endif // HATNCONFIGTREE_H
