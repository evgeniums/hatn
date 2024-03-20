/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file base/configtreepath.h
  *
  * Contains declarations of configuration tree path.
  *
  */

/****************************************************************************/

#ifndef HATNCONFIGTREEPATH_H
#define HATNCONFIGTREEPATH_H

#include <string>
#include <vector>

#include <hatn/common/stdwrappers.h>
#include <hatn/common/result.h>

#include <hatn/base/base.h>

HATN_BASE_NAMESPACE_BEGIN

 /**
 * @brief The ConfigTreePath class represents a path of element in config tree.
 */
class HATN_BASE_EXPORT ConfigTreePath
{
    public:

        static std::string DefaultSeparator; // "."

        /**
         * @brief Constructor from string path.
         * @param path Path string.
         * @param pathSeparator Separator between sections of nested path. Default is ".".
         */
        ConfigTreePath(
            std::string path,
            std::string pathSeparator=DefaultSeparator
        );

        ConfigTreePath()=default;
        ~ConfigTreePath()=default;
        ConfigTreePath(const ConfigTreePath&)=default;
        ConfigTreePath(ConfigTreePath&&)=default;
        ConfigTreePath& operator= (const ConfigTreePath&)=default;
        ConfigTreePath& operator= (ConfigTreePath&&)=default;

        void setSeparator(std::string sep)
        {
            m_separator=std::move(sep);
        }

        std::string separator() const
        {
            return m_separator;
        }

        const std::string& path() const noexcept
        {
            return m_path;
        }

        operator common::lib::string_view() const noexcept
        {
            return m_path;
        }

        operator std::string() const
        {
            return m_path;
        }

        void append(common::lib::string_view path);

        void prepend(common::lib::string_view path);

        size_t count() const noexcept
        {
            return m_parts.size();
        }

        const std::string& back() const
        {
            return m_parts.back();
        }

        const std::string& front() const
        {
            return m_parts.front();
        }

        void dropBack(size_t count=1);

        void dropFront(size_t count=1);

        const std::string& at(size_t index) const
        {
            return m_parts.at(index);
        }

        Result<size_t> numberAt(size_t index) const;

        bool isRoot() const noexcept
        {
            return m_path.empty() || m_path==m_separator;
        }

    private:

        void updateState();

        std::string m_path;
        std::string m_separator;

        std::vector<std::string> m_parts;
};

HATN_BASE_NAMESPACE_END

#endif // HATNCONFIGTREEPATH_H
