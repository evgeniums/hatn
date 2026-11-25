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

#include <hatn/dataunit/objectid.h>

#include <hatn/db/db.h>

// By default Topic keeps its own copy of the string it is built from, so a Topic never
// outlives its characters. Define HATN_DB_TOPIC_NO_COPY to get the old non-owning view.
#ifndef HATN_DB_TOPIC_NO_COPY
#define HATN_DB_TOPIC_COPY
#endif

HATN_DB_NAMESPACE_BEGIN

using TopicHolder=std::shared_ptr<HATN_DATAUNIT_NAMESPACE::ObjectId::String>;

class Topic
{
    public:

        Topic()=default;
        ~Topic()=default;

        Topic(Topic&& other) : m_holder(std::move(other.m_holder)),
                               m_topic(other.topic())
        {
            if (m_holder)
            {
                m_topic=*m_holder;
            }
        }

        Topic(const Topic& other) : m_holder(other.m_holder),
                                    m_topic(other.topic())
        {
            if (m_holder)
            {
                m_topic=*m_holder;
            }
        }

        Topic& operator=(Topic&& other)
        {
            if (this==&other)
            {
                return *this;
            }

            m_holder=std::move(other.m_holder);
            if (m_holder)
            {
                m_topic=*m_holder;
            }
            else
            {
                m_topic=other.m_topic;
            }

            return *this;
        }

        Topic& operator=(const Topic& other)
        {
            if (this==&other)
            {
                return *this;
            }

            m_holder=other.m_holder;
            if (m_holder)
            {
                m_topic=*m_holder;
            }
            else
            {
                m_topic=other.m_topic;
            }

            return *this;
        }

        Topic(const du::ObjectId& topic)
            : m_holder(std::make_shared<HATN_DATAUNIT_NAMESPACE::ObjectId::String>(topic.asString())),
              m_topic(m_holder->data(),m_holder->size())
        {}

#ifdef HATN_DB_TOPIC_COPY

        Topic(const char* topic)
        {
            lib::string_view view{topic};
            load(view.data(),view.size());
        }

        Topic(lib::string_view topic)
        {
            load(topic.data(),topic.size());
        }

        Topic(const std::string& topic)
        {
            load(topic.data(),topic.size());
        }

        template <size_t PreallocatedSize, typename FallbackAllocatorT>
        Topic(const HATN_COMMON_NAMESPACE::StringOnStackT<PreallocatedSize,FallbackAllocatorT>& topic)
        {
            load(topic.data(),topic.size());
        }

#else

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

#endif

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

        bool operator < (const Topic& other) const noexcept
        {
            return m_topic<other.m_topic;
        }

        bool operator < (const lib::string_view& other) const noexcept
        {
            return m_topic<other;
        }

        bool operator == (const Topic& other) const noexcept
        {
            return m_topic==other.m_topic;
        }

        bool operator == (const lib::string_view& other) const noexcept
        {
            return m_topic==other;
        }

        bool operator > (const Topic& other) const noexcept
        {
            return m_topic>other.m_topic;
        }

        bool operator > (const lib::string_view& other) const noexcept
        {
            return m_topic>other;
        }

        bool operator != (const Topic& other) const noexcept
        {
            return m_topic!=other.m_topic;
        }

        bool operator != (const lib::string_view& other) const noexcept
        {
            return m_topic!=other;
        }

        bool operator >= (const Topic& other) const noexcept
        {
            return m_topic>=other.m_topic;
        }

        bool operator >= (const lib::string_view& other) const noexcept
        {
            return m_topic>=other;
        }

        bool operator <= (const Topic& other) const noexcept
        {
            return m_topic<=other.m_topic;
        }

        bool operator <= (const lib::string_view& other) const noexcept
        {
            return m_topic<=other;
        }

        void load(lib::string_view topic)
        {
            m_holder=std::make_shared<HATN_DATAUNIT_NAMESPACE::ObjectId::String>(topic.data(),topic.size());
            m_topic=*m_holder;
        }

        void load(const char* data, size_t size)
        {
            m_holder=std::make_shared<HATN_DATAUNIT_NAMESPACE::ObjectId::String>(data,size);
            m_topic=*m_holder;
        }

        bool isWithHolder() const
        {
            return static_cast<bool>(m_holder);
        }

    private:

        TopicHolder m_holder;
        lib::string_view m_topic;
};

using Topics=common::FlatSetOnStack<Topic,4>;

HATN_DB_NAMESPACE_END

#endif // HATNDBTOPIC_H
