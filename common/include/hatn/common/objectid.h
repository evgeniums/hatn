/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/objectid.h
  *
  *     Base classes for objects with ID.
  *
  */

/****************************************************************************/

#ifndef HATNOBJECTID_H
#define HATNOBJECTID_H

#include <functional>

#include <hatn/common/common.h>
#include <hatn/common/fixedbytearray.h>
#include <hatn/common/withthread.h>

HATN_COMMON_NAMESPACE_BEGIN

using STR_ID_CONTENT=FixedByteArray20;

/**
 * @brief The StringId class to be used for object identification.
 *
 * It can either contain the object's ID or point to ID contained by other (e.g. parent) object.
 */
template <typename StrIdContent=STR_ID_CONTENT>
class StringId
{
    public:

        /**
         * @brief Default ctor is pointing to "unknown"
         */
        StringId() noexcept : m_ptr(&StrIdContent::defaultUnknown())
        {}

        /**
         * @brief Copy ctor
         * @param other Other ID
         *
         * @attention Only pointer is copied
         *
         */
        StringId(const StringId& other) noexcept : m_ptr(other.m_ptr)
        {
        }

        /**
         * @brief Move ctor
         * @param other Other ID
         */
        StringId(StringId&& other) noexcept : m_ptr(other.m_ptr)
        {
            if (!other.m_val.isEmpty())
            {
                m_val=std::move(other.m_val);
                m_ptr=&m_val;
            }
        }

        /**
         * @brief Ctor from content of ID value
         * @param otherVal Content of ID value
         *
         * Intentionally not explicit.
         */
        StringId(const StrIdContent& otherVal) noexcept :m_ptr(&otherVal)
        {}

        /**
         * @brief Move ctor from content of ID value
         * @param otherVal Content of ID value
         *
         */
        StringId(StrIdContent&& val) noexcept :m_val(std::move(val)),
                                       m_ptr(&m_val)
        {}

        /**
         * @brief Ctor from C-string
         * @param val C-string value of ID
         *
         * Intentionally not explicit.
         */
        StringId(const char* val) noexcept :
                            m_val(val),
                            m_ptr(&m_val)
        {}

        ~StringId()=default;

        /**
         * @brief Move assignment perator
         * @param other Other ID
         */
        inline StringId& operator =(StringId&& other) noexcept
        {
            if (this!=&other)
            {
                if (!other.m_val.isEmpty())
                {
                    m_val=std::move(other.m_val);
                    m_ptr=&m_val;
                }
                else
                {
                    m_ptr=other.m_ptr;
                }
            }
            return *this;
        }

        /**
         * @brief Copy assignment operator
         * @param other Other ID
         *
         * @attention Only pointer is copied
         *
         */
        inline StringId& operator =(const StringId& other)
        {
            if (this!=&other)
            {
                m_ptr=other.m_ptr;
            }
            return *this;
        }

        /**
         * @brief Get C-string of ID
         * @return C-string representation of ID
         */
        inline const char* c_str() const noexcept
        {
            return m_ptr->c_str();
        }

        /**
         * @brief Get content
         * @return Content of ID
         */
        inline const StrIdContent& get() const noexcept
        {
            return *m_ptr;
        }

        /**
         * @brief Set ID from C-string
         * @param val C-string value of ID
         */
        inline void set(const char* val) noexcept
        {
            m_val.load(val);
            m_ptr=&m_val;
        }

    private:

        StrIdContent m_val;
        const StrIdContent* m_ptr;
};
using STR_ID_TYPE=StringId<STR_ID_CONTENT>;
using STR_ID_SHARED_TYPE=StringId<STR_ID_CONTENT>;

class Thread;

//! Base class for objects with ID
class WithID
{
    public:

        //! Ctor
        WithID(
            STR_ID_TYPE id=STR_ID_TYPE()
        ) noexcept : m_id(std::move(id))
        {}

        //! Set object ID
        inline void setID(STR_ID_TYPE id) noexcept
        {
            m_id=std::move(id);
        }
        inline const STR_ID_TYPE& id() const noexcept
        {
            return m_id;
        }

        const char* idStr() const noexcept
        {
            return m_id.c_str();
        }

    private:

        STR_ID_TYPE m_id;
};

//! Base class for objects with ID and thread
class WithIDThread : public WithID, public WithThread
{
    public:

        //! Ctor
        WithIDThread(
            Thread* thread,
            STR_ID_TYPE id=STR_ID_TYPE()
        ) noexcept : WithID(std::move(id)),
                     WithThread(thread)
        {}
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END

#endif // HATNOBJECTID_H
