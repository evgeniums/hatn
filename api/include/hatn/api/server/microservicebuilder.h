/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/server/microservicebuilder.h
  *
  */

/****************************************************************************/

#ifndef HATNAPIMICROSERVICEBUILDER_H
#define HATNAPIMICROSERVICEBUILDER_H

#include <hatn/api/api.h>
#include <hatn/api/apiliberror.h>
#include <hatn/api/server/microservice.h>

HATN_API_NAMESPACE_BEGIN

namespace server {

template <typename Traits, typename DispatcherT, typename AuthDispatcherT>
class MicroServiceBuilder
{
    public:

        MicroServiceBuilder(
            DispatchersStore<DispatcherT> dispatchers,
            DispatchersStore<AuthDispatcherT> authDispatchers={}
            ) : m_dispatchers(std::move(dispatchers)),
                m_authDispatchers(std::move(authDispatchers))
        {}

        Result<std::shared_ptr<MicroService>> operator()
            (std::string name, std::string dispatcherName, std::string authDispatcherName) const
        {
            auto dispatcher=m_dispatchers.dispatcher(dispatcherName);
            if (!dispatcher)
            {
                return apiLibError(ApiLibError::UNKNOWN_SERVICE_DISPATCHER,std::make_shared<common::NativeError>(dispatcherName));
            }

            std::shared_ptr<AuthDispatcherT> authDispathcer;
            if (!authDispatcherName.empty())
            {
                authDispathcer=m_authDispatchers.dispatcher(authDispatcherName);
                if (!authDispathcer)
                {
                    return apiLibError(ApiLibError::UNKNOWN_AUTH_DISPATCHER,std::make_shared<common::NativeError>(authDispatcherName));
                }
            }

            return Traits::makeMicroService(std::move(name),std::move(dispatcher),std::move(authDispathcer));
        }

    private:

        DispatchersStore<DispatcherT> m_dispatchers;
        DispatchersStore<AuthDispatcherT> m_authDispatchers;
};

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPIMICROSERVICEBUILDER_H
