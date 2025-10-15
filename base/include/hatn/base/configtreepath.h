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

        ConfigTreePath(
                const char* path,
                std::string pathSeparator=DefaultSeparator
            ) : ConfigTreePath(std::string(path),std::move(pathSeparator))
        {}

        ConfigTreePath(
                common::lib::string_view path,
                std::string pathSeparator=DefaultSeparator
            ) : ConfigTreePath(std::string(path),std::move(pathSeparator))
        {}

        ConfigTreePath(): ConfigTreePath(std::string())
        {}

        ConfigTreePath(
            std::vector<std::string> parts,
            std::string pathSeparator=DefaultSeparator
        );

        template <typename T>
        ConfigTreePath(
            std::initializer_list<T> parts,
            std::string pathSeparator=DefaultSeparator
            ) : m_separator(std::move(pathSeparator))
        {
            for (auto&& part:parts)
            {
                m_parts.emplace_back(std::move(part));
            }
            updateState();
        }

        ~ConfigTreePath()=default;
        ConfigTreePath(const ConfigTreePath&)=default;
        ConfigTreePath(ConfigTreePath&&)=default;
        ConfigTreePath& operator= (const ConfigTreePath&)=default;
        ConfigTreePath& operator= (ConfigTreePath&&)=default;

        void setSeparator(std::string sep)
        {
            m_separator=std::move(sep);
            updateState();
        }

        std::string separator() const
        {
            return m_separator;
        }

        void setPath(std::string path)
        {
            m_parts.clear();
            m_path=std::move(path);
            updateState();
        }

        const std::string& path() const noexcept
        {
            return m_path;
        }

        const std::string& string() const noexcept
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

        void append(size_t index)
        {
            append(std::to_string(index));
        }

        ConfigTreePath copyAppend(common::lib::string_view path) const
        {
            auto n=*this;
            n.append(path);
            return n;
        }

        ConfigTreePath copyAppend(size_t index) const
        {
            return copyAppend(std::to_string(index));
        }

        void prepend(common::lib::string_view path);

        void prepend(size_t index)
        {
            prepend(std::to_string(index));
        }

        ConfigTreePath copyPrepend(common::lib::string_view path) const
        {
            auto n=*this;
            n.prepend(path);
            return n;
        }

        ConfigTreePath copyPrepend(size_t index) const
        {
            return copyPrepend(std::to_string(index));
        }

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

        ConfigTreePath copyDropBack(size_t count=1) const
        {
            auto n=*this;
            n.dropBack(count);
            return n;
        }

        void dropFront(size_t count=1);

        ConfigTreePath copyDropFront(size_t count=1) const
        {
            auto n=*this;
            n.dropFront(count);
            return n;
        }

        const std::string& at(size_t index) const
        {
            return m_parts.at(index);
        }

        Result<size_t> numberAt(size_t index) const;

        bool isRoot() const noexcept
        {
            return m_path.empty();
        }

        void reset()
        {
            m_parts.clear();
            m_path.clear();
        }

        friend inline bool operator ==(const ConfigTreePath& left,const ConfigTreePath& right) noexcept
        {
            if (left.m_parts.size()!=right.m_parts.size())
            {
                return false;
            }

            for (size_t i=0;i<left.m_parts.size();i++)
            {
                if (left.m_parts.at(i)!=right.m_parts.at(i))
                {
                    return false;
                }
            }

            return true;
        }
        friend inline bool operator <(const ConfigTreePath& left,const ConfigTreePath& right) noexcept
        {
            for (size_t i=0;i<left.m_parts.size();i++)
            {
                if (i==right.m_parts.size())
                {
                    return false;
                }
                if (left.m_parts.at(i)<right.m_parts.at(i))
                {
                    return true;
                }
                else if (left.m_parts.at(i)>right.m_parts.at(i))
                {
                    return false;
                }
            }
            if (left.m_parts.size()==right.m_parts.size())
            {
                return false;
            }
            return true;
        }

    private:

        void updateState();

        std::string m_path;
        std::string m_separator;

        std::vector<std::string> m_parts;
};

HATN_BASE_NAMESPACE_END

#endif // HATNCONFIGTREEPATH_H
