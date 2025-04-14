/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/server/microservice.h
  *
  */

/****************************************************************************/

#ifndef HATNAPIMICROSERVICE_H
#define HATNAPIMICROSERVICE_H

#include <hatn/common/error.h>
#include <hatn/common/objecttraits.h>

#include <hatn/app/baseapp.h>

#include <hatn/api/api.h>
#include <hatn/api/apiliberror.h>

#include <hatn/api/server/servicedispatcher.h>
#include <hatn/api/server/authdispatcher.h>

HATN_API_NAMESPACE_BEGIN

namespace server {

class HATN_API_EXPORT MicroService
{
    public:

        MicroService()=default;
        virtual ~MicroService();

        MicroService(const MicroService&)=delete;
        MicroService(MicroService&&)=default;
        MicroService& operator=(const MicroService&)=delete;
        MicroService& operator=(MicroService&&)=default;

        virtual Error start(
            const HATN_APP_NAMESPACE::BaseApp& app,
            const HATN_BASE_NAMESPACE::ConfigTree& configTree,
            const HATN_BASE_NAMESPACE::ConfigTreePath& configTreePath
        )=0;

        virtual void close()=0;

        virtual const std::string& name() const noexcept=0;
};

template <typename ImplT>
class MicroServiceV : public common::WithImpl<ImplT>,
                      public MicroService
{
    public:

        using common::WithImpl<ImplT>::common::WithImpl;

        virtual Error start(
                const HATN_APP_NAMESPACE::BaseApp& app,
                const HATN_BASE_NAMESPACE::ConfigTree& configTree,
                const HATN_BASE_NAMESPACE::ConfigTreePath& configTreePath
            ) override
        {
            return this->impl().start(app,configTree,configTreePath);
        }

        virtual void close() override
        {
            this->impl().close();
        }

        virtual const std::string& name() const noexcept override
        {
            return this->impl().name();
        }
};

template <
         typename Traits,
         typename EnvConfigT,
         typename DispatcherT=ServiceDispatcher<typename EnvConfigT::Env>,
         typename AuthDispatcherT=AuthDispatcher<typename EnvConfigT::Env>>
class MicroServiceT : public common::WithTraits<Traits>
{
    public:

        using EnvConfig=EnvConfigT;
        using Env=typename EnvConfig::Env;

        using Dispatcher=DispatcherT;
        using AuthDispatcher=AuthDispatcherT;

        template <typename ...TraitsArgs>
        MicroServiceT(
                std::string name,
                std::shared_ptr<Dispatcher> dispatcher,
                std::shared_ptr<AuthDispatcher> authDispatcher,
                TraitsArgs&& ...traitsArgs
            ) : common::WithTraits<Traits>(std::forward<TraitsArgs>(traitsArgs)...),
                m_name(std::move(name)),
                m_dispatcher(std::move(dispatcher)),
                m_authDispatcher(std::move(authDispatcher))
        {}

        template <typename ...TraitsArgs>
        MicroServiceT(
                std::string name,
                std::shared_ptr<Dispatcher> dispatcher,
                TraitsArgs&& ...traitsArgs
            ) : common::WithTraits<Traits>(std::forward<TraitsArgs>(traitsArgs)...),
                m_name(std::move(name)),
                m_dispatcher(std::move(dispatcher))
        {}

        const std::string& name() const noexcept
        {
            return m_name;
        }

        std::shared_ptr<Dispatcher> dispatcher() const
        {
            return m_dispatcher;
        }

        std::shared_ptr<AuthDispatcher> authDispatcher()
        {
            return m_authDispatcher;
        }

        Error start(
                const HATN_APP_NAMESPACE::BaseApp& app,
                const HATN_BASE_NAMESPACE::ConfigTree& configTree,
                const HATN_BASE_NAMESPACE::ConfigTreePath& configTreePath
            )
        {
            auto envR=EnvConfig::makeEnv(app,configTree,configTreePath);
            if (envR)
            {
                return envR.takeError();
            }
            return this->traits().start(envR.takeValue(),app,configTree,configTreePath);
        }

        void close()
        {
            this->traits().close();
        }

    private:

        std::string m_name;
        std::shared_ptr<Dispatcher> m_dispatcher;
        std::shared_ptr<AuthDispatcher> m_authDispatcher;
};

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPIMICROSERVICE_H
