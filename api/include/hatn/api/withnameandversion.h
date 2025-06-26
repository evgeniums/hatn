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

class WithVersion
{
    public:

        using VersionType=uint32_t;

        WithVersion(VersionType version=1) : m_version(version)
        {}

        void setVersion(VersionType version) noexcept
        {
            m_version=version;
        }

        VersionType version() const noexcept
        {
            return m_version;
        }

        template <typename T>
        bool operator < (const T& other) const noexcept
        {
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

        VersionType m_version;
};


template <size_t NameLength>
class WithNameAndVersion : public WithVersion
{
    public:

        using NameType=common::StringOnStackT<NameLength>;

        WithNameAndVersion()
        {}

        WithNameAndVersion(lib::string_view name, VersionType version=1) : WithVersion(version), m_name(name)
        {}

        void setName(lib::string_view name)
        {
            m_name=name;
        }

        lib::string_view name() const noexcept
        {
            return m_name;
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
            if (version()<other.version())
            {
                return true;
            }
            if (version()>other.version())
            {
                return false;
            }
            return false;
        }

    private:

        NameType m_name;
};

HATN_API_NAMESPACE_END

#endif // HATNAPIWITHNAMEANDVERSION_H
