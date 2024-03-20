/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file base/configtreepath.сpp
  *
  *  Defines ConfigTreePath class.
  *
  */

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/join.hpp>

#include <hatn/base/baseerror.h>
#include <hatn/base/configtreepath.h>

HATN_BASE_NAMESPACE_BEGIN

std::string ConfigTreePath::DefaultSeparator=".";

//---------------------------------------------------------------

ConfigTreePath::ConfigTreePath(
        std::string path,
        std::string pathSeparator
    ) : m_path(std::move(path)),
        m_separator(std::move(pathSeparator))
{
    updateState();
}

//---------------------------------------------------------------

void ConfigTreePath::append(common::lib::string_view path)
{
    if (!path.empty())
    {
        std::vector<std::string> parts;
        boost::algorithm::split(parts, path, boost::algorithm::is_any_of(m_separator));
        m_parts.insert(std::end(m_parts),std::begin(parts),std::end(parts));
        updateState();
    }
}

//---------------------------------------------------------------

void ConfigTreePath::prepend(common::lib::string_view path)
{
    if (!path.empty())
    {
        std::vector<std::string> parts;
        boost::algorithm::split(parts, path, boost::algorithm::is_any_of(m_separator));
        m_parts.insert(std::begin(m_parts),std::begin(parts),std::end(parts));
        updateState();
    }
}

//---------------------------------------------------------------

void ConfigTreePath::dropBack(size_t count)
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

//---------------------------------------------------------------

void ConfigTreePath::dropFront(size_t count)
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

//---------------------------------------------------------------

Result<size_t> ConfigTreePath::numberAt(size_t index) const
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

//---------------------------------------------------------------

void ConfigTreePath::updateState()
{
    if (m_parts.empty())
    {
        boost::algorithm::split(m_parts, m_path, boost::algorithm::is_any_of(m_separator));
    }
    else
    {
        m_path=boost::algorithm::join(m_parts, m_separator);
    }
}

//---------------------------------------------------------------

HATN_BASE_NAMESPACE_END
