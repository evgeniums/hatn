/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/withnameandversion.h
  *
  */

/****************************************************************************/

#ifndef HATNAPIWITHNAMEANDVERSION_H
#define HATNAPIWITHNAMEANDVERSION_H

#include <hatn/common/allocatoronstack.h>

#include <hatn/api/api.h>
#include <hatn/api/apiconstants.h>

HATN_API_NAMESPACE_BEGIN

template <size_t NameLength>
class WithNameAndVersion
{
    public:

        using NameType=common::StringOnStackT<NameLength>;

        WithNameAndVersion() : m_version(1)
        {}

        template <typename T>
        WithNameAndVersion(T&& name, uint8_t version=1) : m_name(std::forward<T>(name)),m_version(version)
        {}

        void setName(lib::string_view name)
        {
            m_name=name;
        }

        lib::string_view name() const noexcept
        {
            return m_name;
        }

        void setVersion(uint8_t version) noexcept
        {
            m_version=version;
        }

        uint8_t version() const noexcept
        {
            return m_version;
        }

        template <typename T>
        bool operator < (const T& other) const noexcept
        {
            if (lib::string_view(m_name)<other.name())
            {
                return true;
            }
            if (lib::string_view(m_name)>other.name())
            {
                return false;
            }
            if (m_version<other.version())
            {
                return true;
            }
            if (m_version>other.version())
            {
                return false;
            }
            return false;
        }

    private:

        NameType m_name;
        uint8_t m_version;
};

HATN_API_NAMESPACE_END

#endif // HATNAPIWITHNAMEANDVERSION_H
