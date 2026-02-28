/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/client/plaintcprouter.h
  *
  */

/****************************************************************************/

#ifndef HATNAPIPLAINTCPROUTER_H
#define HATNAPIPLAINTCPROUTER_H

#include <hatn/logcontext/logcontext.h>

#include <hatn/api/api.h>
#include <hatn/api/router.h>
#include <hatn/api/client/plaintcpconnection.h>
#include <hatn/api/client/tcpclientconfig.h>

HATN_API_NAMESPACE_BEGIN

namespace client {

class PlainTcpRouterTraits
{
    public:

        using ConnectionContext=PlainTcpConnectionContext;
        using Connection=PlainTcpConnection;

        template <typename ...Args>
        PlainTcpRouterTraits(
                Args&&... args
            ) : m_config(std::forward<Args>(args)...),
                m_allocatorFactory(common::pmr::AllocatorFactory::getDefault())
        {}

        template <typename ...Args>
        PlainTcpRouterTraits(
                const common::pmr::AllocatorFactory* allocatorFactory,
                Args&&... args
            ) : m_config(std::forward<Args>(args)...),
                m_allocatorFactory(allocatorFactory)
        {}

        template <typename ContextT>
        void makeConnection(
                common::SharedPtr<ContextT> ctx,
                RouterCallbackFn<ConnectionContext> callback,
                lib::string_view name={}
            )
        {
            //! @todo Configure timeouts

            std::ignore=ctx;
            callback(Error{},
                     allocatePlainTcpConnectionContext(m_allocatorFactory->objectAllocator<ConnectionContext>(),
                                                       m_config.hosts(),m_config.resolver(),m_config.thread(),m_config.shuffle(),name)
                     );
        }

    private:

        TcpClientConfig m_config;
        const common::pmr::AllocatorFactory* m_allocatorFactory;
};

using PlainTcpRouter=Router<PlainTcpRouterTraits>;

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPIPLAINTCPROUTER_H
