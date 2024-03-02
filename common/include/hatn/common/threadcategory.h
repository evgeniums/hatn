/*
    Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/
/****************************************************************************/
/** @file common/threadcategory.h
 *
 *     Thread category.
 *
 */
/****************************************************************************/

#ifndef HATNTHREADCATEGORY_H
#define HATNTHREADCATEGORY_H

#include <string>

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

//! Thread category
class HATN_COMMON_EXPORT ThreadCategory
{
    public:

        //! Ctor
        ThreadCategory(
            std::string family="generic"
        ) noexcept : m_family(std::move(family))
        {}

        //! Get family
        inline std::string family() const noexcept
        {
            return m_family;
        }

        static ThreadCategory generic;

    private:

        std::string m_family;
};

//! Thread category with priority
class ThreadCategoryAndPriority
{
    public:

        //! Ctor
        ThreadCategoryAndPriority(const ThreadCategory& category=ThreadCategory::generic, int priority=0) noexcept
            : m_category(&category),
              m_priority(priority)
        {}

        inline int priority() const noexcept
        {
            return m_priority;
        }
        inline void setPriority(int priority) noexcept
        {
            m_priority=priority;
        }

        inline const ThreadCategory& category() const noexcept
        {
            return *m_category;
        }

    private:

        const ThreadCategory* m_category;
        int m_priority;
};

HATN_COMMON_NAMESPACE_END

namespace std
{
    template<> struct less<::hatn::common::ThreadCategory>
    {
       bool operator() (const ::hatn::common::ThreadCategory& lhs, const ::hatn::common::ThreadCategory& rhs) const noexcept
       {
           return lhs.family() < rhs.family();
       }
    };
}

#endif // HATNTHREADCATEGORY_H
