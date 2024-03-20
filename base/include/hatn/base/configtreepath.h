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

#include <vector>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/join.hpp>

#include <hatn/common/error.h>

#include <hatn/base/base.h>
#include <hatn/base/baseerror.h>

HATN_BASE_NAMESPACE_BEGIN

 /**
 * @brief The ConfigTreePath class represents a path of element in config tree.
 */
class HATN_BASE_EXPORT ConfigTreePath
{
    public:

        /**
         * @brief Constructor from string path.
         * @param path Path string.
         * @param pathSeparator Separator between sections of nested path. Default is ".".
         */
        ConfigTreePath(
                const std::string& path,
                std::string pathSeparator="."
            ):ConfigTreePath(common::lib::string_view(path),std::move(pathSeparator))
        {}

        /**
         * @brief Constructor from const char* path.
         * @param path Path string.
         * @param pathSeparator Separator between sections of nested path. Default is ".".
         */
        ConfigTreePath(
                const char* path,
                std::string pathSeparator="."
            ):ConfigTreePath(common::lib::string_view(path),std::move(pathSeparator))
        {}

        /**
         * @brief Constructor from string view.
         * @param path Path string.
         * @param pathSeparator Separator between sections of nested path. Default is ".".
         */
        ConfigTreePath(
                const common::lib::string_view& path,
                std::string pathSeparator="."
            ) : m_path(path),
                m_pathSeparator(std::move(pathSeparator)),
                m_prepared(false)
        {}

        /**
         * @brief Constructor from string view.
         * @param pathSeparator Separator between sections of nested path. Default is ".".
         */
        ConfigTreePath(
                common::lib::string_view&& path,
                std::string pathSeparator="."
            ) : m_path(std::move(path)),
                m_pathSeparator(std::move(pathSeparator)),
                m_prepared(false)
        {}

        common::lib::string_view pathView() const noexcept
        {
            return m_path;
        }

        operator common::lib::string_view() const noexcept
        {
            return pathView();
        }

        void prepare() const
        {
            if (!m_prepared)
            {
                auto self=const_cast<ConfigTreePath*>(this);
                self->updateState();
                self->m_prepared=true;
            }
        }

        void append(common::lib::string_view path)
        {
            if (!path.empty())
            {
                std::vector<std::string> parts;
                boost::algorithm::split(parts, path, boost::algorithm::is_any_of(m_pathSeparator));
                m_parts.insert(std::end(m_parts),std::begin(parts),std::end(parts));
                updateState();
            }
        }

        void prepend(common::lib::string_view path)
        {
            if (!path.empty())
            {
                std::vector<std::string> parts;
                boost::algorithm::split(parts, path, boost::algorithm::is_any_of(m_pathSeparator));
                m_parts.insert(std::begin(m_parts),std::begin(parts),std::end(parts));
                updateState();
            }
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

        void dropBack(size_t count=1)
        {
            auto num=count;
            if (num>m_parts.size())
            {
                num=m_parts.size();
            }
            if (num==0)
            {
                return;
            }

            m_parts.erase(std::end(m_parts)-num,std::end(m_parts));
            updateState();
        }

        void dropFront(size_t count=1)
        {
            auto num=count;
            if (num>m_parts.size())
            {
                num=m_parts.size();
            }
            if (num==0)
            {
                return;
            }

            m_parts.erase(std::begin(m_parts),std::begin(m_parts)+num);
            updateState();
        }

        const std::string& at(size_t index) const
        {
            return m_parts.at(index);
        }

        Result<size_t> numberAt(size_t index) const
        {
            const std::string& key=m_parts.at(index);
            try
            {
                return static_cast<size_t>(std::stoi(key));
            }
            catch (...)
            {
            }
            return baseErrorResult(BaseError::STRING_NOT_NUMBER);
        }

        bool isRoot() const noexcept
        {
            return m_path.empty() || m_path==m_pathSeparator;
        }

    private:

        void updateState()
        {
            if (m_parts.empty())
            {
                m_storagePath=m_path;
                boost::algorithm::split(m_parts, m_path, boost::algorithm::is_any_of(m_pathSeparator));
            }
            else
            {
                m_storagePath=boost::algorithm::join(m_parts, m_pathSeparator);
            }
            m_path=m_storagePath;
        }

        common::lib::string_view m_path;
        std::string m_pathSeparator;

        std::vector<std::string> m_parts;
        std::string m_storagePath;

        bool m_prepared;
};

HATN_BASE_NAMESPACE_END

#endif // HATNCONFIGTREEPATH_H
