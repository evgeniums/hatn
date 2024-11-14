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

#include <hatn/db/db.h>

HATN_DB_NAMESPACE_BEGIN

class Topic
{
    public:

        Topic()=default;

        Topic(const lib::string_view& topic)
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

        operator < (const Topic& other) const noexcept
        {
            return m_topic<other.m_topic;
        }

        operator == (const Topic& other) const noexcept
        {
            return m_topic==other.m_topic;
        }

        operator > (const Topic& other) const noexcept
        {
            return m_topic>other.m_topic;
        }

        operator != (const Topic& other) const noexcept
        {
            return m_topic!=other.m_topic;
        }

        operator >= (const Topic& other) const noexcept
        {
            return m_topic>=other.m_topic;
        }

        operator <= (const Topic& other) const noexcept
        {
            return m_topic<other.m_topic;
        }

    private:

        lib::string_view m_topic;        
};

//! @todo Use container on stack
using Topics=common::pmr::FlatSet<Topic>;

HATN_DB_NAMESPACE_END

#endif // HATNDBTOPIC_H
