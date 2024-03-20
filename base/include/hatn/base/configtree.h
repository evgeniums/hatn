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
#include <hatn/base/detail/configtreevalue.ipp>

HATN_BASE_NAMESPACE_BEGIN

 /**
 * @brief The ConfigTree class represents a tree of values that can be accessed with literal nested path.
 */
class HATN_BASE_EXPORT ConfigTree : public ConfigTreeValue
{
    public:

        /**
         * @brief Constructor.
         * @param pathSeparator Separator between sections of nested path. Default is ".".
         */
        ConfigTree(const std::string& pathSeparator="."):m_pathSeparator(pathSeparator)
        {}

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

        std::string pathSeparator() const
        {
            return m_pathSeparator;
        }

        template <typename T>
        Result<ConfigTree&> set(common::lib::string_view path, T&& value, bool autoCreatePath=true) noexcept
        {
            auto r=getImpl(path,autoCreatePath);
            HATN_CHECK_RESULT(r)
            r->set(std::forward<T>(value));
            return r;
        }

        template <typename T>
        ConfigTree& set(common::lib::string_view path, T&& value, Error &ec, bool autoCreatePath=true) noexcept
        {
            auto&& r=getImpl(path,autoCreatePath);
            HATN_RESULT_EC(r,ec)
            r->set(std::forward<T>(value));
            return r.takeValue();
        }

        template <typename T>
        ConfigTree& setEx(common::lib::string_view path, T&& value, bool autoCreatePath=true)
        {
            auto&& r=getImpl(path,autoCreatePath);
            HATN_RESULT_THROW(r)
            r->set(std::forward<T>(value));
            return r.takeValue();
        }

        template <typename T>
        Result<ConfigTree&> setDefault(common::lib::string_view path, T&& value, bool autoCreatePath=true) noexcept
        {
            auto r=getImpl(path,autoCreatePath);
            HATN_CHECK_RESULT(r)
            r->setDefault(std::forward<T>(value));
            return r;
        }

        template <typename T>
        ConfigTree& setDefault(common::lib::string_view path, T&& value, Error &ec, bool autoCreatePath=true) noexcept
        {
            auto&& r=getImpl(path,autoCreatePath);
            HATN_RESULT_EC(r,ec)
            r->setDefault(std::forward<T>(value));
            return r.takeValue();
        }

        template <typename T>
        ConfigTree& setDefaultEx(common::lib::string_view path, T&& value, bool autoCreatePath=true)
        {
            auto&& r=getImpl(path,autoCreatePath);
            HATN_RESULT_THROW(r)
            r->setDefault(std::forward<T>(value));
            return r.takeValue();
        }

        Result<const ConfigTree&> get(common::lib::string_view path) const noexcept
        {
            return getImpl(path);
        }
        const ConfigTree& get(common::lib::string_view path, Error &ec) const noexcept;
        const ConfigTree& getEx(common::lib::string_view path) const;

        Result<ConfigTree&> get(common::lib::string_view path, bool autoCreatePath=false) noexcept
        {
            return getImpl(path,autoCreatePath);
        }
        ConfigTree& get(common::lib::string_view path, Error &ec, bool autoCreatePath=false) noexcept;
        ConfigTree& getEx(common::lib::string_view path, bool autoCreatePath=false);

        bool isSet(common::lib::string_view path) const noexcept;

        template <typename T>
        auto toArray(common::lib::string_view path) -> decltype(auto)
        {
            auto r=get(path,true);
            return r->toArray<T>();
        }

        config_tree::MapT& toMap(common::lib::string_view path);

        void reset(common::lib::string_view path) noexcept;

    private:

        Result<const ConfigTree&> getImpl(const common::lib::string_view& path) const noexcept;
        Result<ConfigTree&> getImpl(const common::lib::string_view& path, bool autoCreatePath=false) noexcept;

        std::string m_pathSeparator;
};

HATN_BASE_NAMESPACE_END

#endif // HATNCONFIGTREE_H
