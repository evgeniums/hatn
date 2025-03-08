/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/server/connectionsstore.h
  *
  */

/****************************************************************************/

#ifndef HATNAPICONNECTIONSSTORE_H
#define HATNAPICONNECTIONSSTORE_H

#include <hatn/common/pmr/pmrtypes.h>
#include <hatn/common/pmr/allocatorfactory.h>
#include <hatn/common/taskcontext.h>
#include <hatn/common/sharedptr.h>
#include <hatn/common/locker.h>

#include <hatn/api/api.h>

HATN_API_NAMESPACE_BEGIN

namespace server {

template <typename ConnectionContextT, typename ConnectionT>
class ConnectionsStore
{
    public:

        using ConnectionContext=ConnectionContextT;
        using Connection=ConnectionT;

        ConnectionsStore(
                const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
            ) : m_connections(factory->objectAllocator<typename decltype(m_connections)::value_type>())
        {}

        void registerConnection(const common::SharedPtr<ConnectionContext>& ctx)
        {
            common::MutexScopedLock l{m_mutex};
            m_connections.emplace(ctx->id(),ctx);
        }

        void removeConnection(const lib::string_view& id)
        {
            common::MutexScopedLock l{m_mutex};
            m_connections.erase(id);
        }

        common::SharedPtr<ConnectionContext> connectionCtx(const lib::string_view& id) const
        {
            common::MutexScopedLock l{m_mutex};
            auto it=m_connections.find(id);
            if (it!=m_connections.end())
            {
                return it->second;
            }
            return common::SharedPtr<ConnectionContext>{};
        }

        std::pair<common::SharedPtr<ConnectionContext>,Connection*> connection(const lib::string_view& id) const
        {
            auto ctx=connectionCtx(id);
            if (ctx)
            {
                auto connection=&ctx->template get<Connection>();
                std::make_pair(std::move(ctx),connection);
            }
            return std::pair<Connection*,common::SharedPtr<ConnectionContext>>{{},nullptr};
        }

        void clear()
        {
            common::MutexScopedLock l{m_mutex};
            for (auto&& it:m_connections)
            {
                auto connection=&it.second->template get<Connection>();
                connection.close();
            }
            m_connections.clear();
        }

        auto closeConnection(const lib::string_view& id)
        {
            common::MutexScopedLock l{m_mutex};
            auto conn=connection(id);
            if (conn.first)
            {
                conn.second.close();
                m_connections.erase(conn.first->id());
            }
            return conn;
        }

        size_t count() const noexcept
        {
            common::MutexScopedLock l{m_mutex};
            return m_connections.size();
        }

    private:

        mutable common::MutexLock m_mutex;
        common::pmr::map<common::TaskContextId,common::SharedPtr<ConnectionContext>,std::less<common::TaskContextId>> m_connections;
};

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPICONNECTIONSSTORE_H

