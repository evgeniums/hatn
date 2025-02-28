/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/service.h
  *
  */

/****************************************************************************/

#ifndef HATNAPISERVICE_H
#define HATNAPISERVICE_H

#include <hatn/common/allocatoronstack.h>

#include <hatn/api/api.h>
#include <hatn/api/apiconstants.h>

HATN_API_NAMESPACE_BEGIN

class Service
{
    public:

        using NameType=common::StringOnStackT<ServiceNameLengthMax>;

        template <typename T>
        Service(T&& name, uint8_t version=1) : m_name(std::forward<T>(name)),m_version(version)
        {}

        const NameType& name() const noexcept
        {
            return m_name;
        }

        uint8_t version() const noexcept
        {
            return m_version;
        }

    private:

        NameType m_name;
        uint8_t m_version;
};

HATN_API_NAMESPACE_END

#endif // HATNAPISERVICE_H
