/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file db/topic.h
  *
  * Contains declaration of db topic.
  *
  */

/****************************************************************************/

#ifndef HATNDBTOPIC_H
#define HATNDBTOPIC_H

#include <hatn/common/flatmap.h>
#include <hatn/common/stdwrappers.h>
#include <hatn/common/allocatoronstack.h>

#include <hatn/db/db.h>

HATN_DB_NAMESPACE_BEGIN

class Topic
{
    public:

        Topic()=default;
        ~Topic()=default;
        Topic(Topic&& other)=default;
        Topic(const Topic& other)=default;
        Topic& operator=(Topic&&)=default;
        Topic& operator=(const Topic&)=default;

        Topic(const char* topic)
            : m_topic(topic)
        {}

        Topic(lib::string_view topic)
            : m_topic(topic)
        {}

        Topic(const std::string& topic)
            : m_topic(topic)
        {}

        template <size_t PreallocatedSize, typename FallbackAllocatorT>
        Topic(const HATN_COMMON_NAMESPACE::StringOnStackT<PreallocatedSize,FallbackAllocatorT>& topic)
            : m_topic(topic)
        {}

        const lib::string_view& topic() const noexcept
        {
            return m_topic;
        }

        operator lib::string_view() const noexcept
        {
            return m_topic;
        }

        operator std::string() const
        {
            return std::string{m_topic.data(),m_topic.size()};
        }

        bool operator < (Topic other) const noexcept
        {
            return m_topic<other.m_topic;
        }

        bool operator == (Topic other) const noexcept
        {
            return m_topic==other.m_topic;
        }

        bool operator > (Topic other) const noexcept
        {
            return m_topic>other.m_topic;
        }

        bool operator != (Topic other) const noexcept
        {
            return m_topic!=other.m_topic;
        }

        bool operator >= (Topic other) const noexcept
        {
            return m_topic>=other.m_topic;
        }

        bool operator <= (Topic other) const noexcept
        {
            return m_topic<other.m_topic;
        }

    private:

        lib::string_view m_topic;        
};

using Topics=common::FlatSetOnStack<Topic,4>;

HATN_DB_NAMESPACE_END

#endif // HATNDBTOPIC_H
