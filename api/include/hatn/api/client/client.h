/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/client/client.h
  *
  */

/****************************************************************************/

#ifndef HATNAPICLIENT_H
#define HATNAPICLIENT_H

#include <hatn/common/stdwrappers.h>
#include <hatn/common/allocatoronstack.h>

#include <hatn/dataunit/objectid.h>

#include <hatn/api/api.h>
#include <hatn/api/apiconstants.h>
#include <hatn/api/connectionpool.h>
#include <hatn/api/client/router.h>

HATN_API_NAMESPACE_BEGIN

using RequestId=du::ObjectId;
using Topic=common::lib::string_view;

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

enum class Priority : uint8_t
{
    Lowest,
    Low,
    Normal,
    Higher,
    Highest,
    Urgent
};

namespace client {

template <typename RouterTraits>
class Client
{
    public:

        using Connection=typename RouterTraits::Connection;

        template <typename ContextT, typename SessionT, typename UnitT, typename CallbackT>
        RequestId exec(
            common::SharedPtr<ContextT> ctx,
            const SessionT& session,
            const Service& service,
            const Method& method,
            common::SharedPtr<UnitT> content,
            CallbackT callback,
            const Topic& topic=Topic{},
            uint32_t timeoutMs=0,
            Priority priority=Priority::Normal
        );

        template <typename ContextT, typename CallbackT>
        void close(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback
        );

    private:

        ConnectionPool<Connection> m_connections;
        common::SharedPtr<Router<RouterTraits>> m_router;
};

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPICLIENT_H
