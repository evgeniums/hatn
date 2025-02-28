/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/method.h
  *
  */

/****************************************************************************/

#ifndef HATNAPITMETHOD_H
#define HATNAPITMETHOD_H

#include <hatn/common/allocatoronstack.h>

#include <hatn/api/api.h>
#include <hatn/api/apiconstants.h>

HATN_API_NAMESPACE_BEGIN

class Method
{
    public:

        using NameType=common::StringOnStackT<MethodNameLengthMax>;

        template <typename T>
        Method(T&& name) : m_name(std::forward<T>(name))
        {}

        const NameType& name() const noexcept
        {
            return m_name;
        }

    private:

        NameType m_name;
};

HATN_API_NAMESPACE_END

#endif // HATNAPITMETHOD_H
