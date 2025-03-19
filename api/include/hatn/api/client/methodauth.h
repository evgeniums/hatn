/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/client/methodauth.h
  *
  */

/****************************************************************************/

#ifndef HATNAPIMETHODAUTH_H
#define HATNAPIMETHODAUTH_H

#include <hatn/common/objecttraits.h>
#include <hatn/common/flatmap.h>

#include <hatn/api/api.h>
#include <hatn/api/service.h>
#include <hatn/api/method.h>
#include <hatn/api/message.h>
#include <hatn/api/auth.h>

HATN_API_NAMESPACE_BEGIN

struct ServiceMethod
{
    Service service;
    Method method;
};

namespace client {

class MethodAuth : public Auth
{
    public:

        template <typename UnitT>
        Error serializeAuthHeader(lib::string_view protocol, uint32_t protocolVersion, common::SharedPtr<UnitT> content,
                                  const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
                                  );
};

template <typename Traits>
class MethodAuthHandler : public common::WithTraits<Traits>
{
    public:

        using common::WithTraits<Traits>::WithTraits;

        template <typename ContextT, typename CallbackT, typename MessageT>
        void makeAuthHeader(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback,
            const Service& service,
            const Method& method,
            MessageT message,
            lib::string_view topic={},
            const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
        ) const
        {
            auto cb=[factory,callback{std::move(callback)},protocol{this->traits().protocol()},protocolVersion{this->traits().protocolVersion()}](auto ctx, const Error& ec, auto content)
            {
                if (ec)
                {
                    callback(std::move(ctx),ec,MethodAuth{});
                    return;
                }
                MethodAuth wrapper;
                auto ec1=wrapper.serializeAuthHeader(protocol,protocolVersion,std::move(content),factory);
                callback(std::move(ctx),ec1,std::move(wrapper));
            };

            this->traits().makeAuthHeader(std::move(ctx),service,method,std::move(message),topic);
        }

        const char* protocol() const noexcept
        {
            return this->traits().protocol();
        }

        uint8_t protocolVersion() const noexcept
        {
            return this->traits().protocolVersioon();
        }
};

class NoMethodAuth
{
    public:

        template <typename ContextT, typename CallbackT, typename ServiceT, typename MessageT>
        void makeAuthHeader(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback,
            common::SharedPtr<ServiceT>,
            const Method&,
            MessageT,
            lib::string_view ={},
            const common::pmr::AllocatorFactory* =common::pmr::AllocatorFactory::getDefault()
        ) const
        {
            callback(std::move(ctx),Error{},MethodAuth{});
        }

        constexpr static const char* protocol() noexcept
        {
            return "noauth";
        }

        constexpr static uint8_t protocolVersion() noexcept
        {
            return 1;
        }
};

template <typename MethodAuthHandlerT=NoMethodAuth>
class ServiceMethodsAuthSingle
{
    public:

        using MethodAuthHandler=MethodAuthHandlerT;

        template <typename ...Args>
        ServiceMethodsAuthSingle(Args&& ...args) : ServiceMethodsAuthSingle(std::make_shared<MethodAuthHandlerT>(std::forward<Args>(args)...))
        {}

        ServiceMethodsAuthSingle(
                std::shared_ptr<MethodAuthHandler> methodAuth
            ) : m_methodAuth(std::move(methodAuth))
        {}

        void registerMethodAuth(const Method&, std::shared_ptr<MethodAuthHandler> handler)
        {
            m_methodAuth=std::move(handler);
        }

        void setMethodAuth(std::shared_ptr<MethodAuthHandler> handler) noexcept
        {
            m_methodAuth=std::move(handler);
        }

        std::shared_ptr<MethodAuthHandler> methodAuthShared(const Method&) const noexcept
        {
            return m_methodAuth;
        }

        const MethodAuthHandler* methodAuth(const Method&) const noexcept
        {
            return m_methodAuth.get();
        }

        template <typename ContextT, typename CallbackT, typename ServiceT, typename MessageT>
        void makeAuthHeader(            
            common::SharedPtr<ContextT> ctx,
            CallbackT callback,
            common::SharedPtr<ServiceT> service,
            const Method& mthd,
            MessageT message,
            lib::string_view topic={},
            const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
            )
        {
            if (!m_methodAuth)
            {
                callback(std::move(ctx),Error{},MethodAuth{});
                return;
            }
            m_methodAuth->makeAuthHandler(std::move(ctx),std::move(callback),std::move(service),mthd,std::move(message),topic,factory);
        }

    private:

        std::shared_ptr<MethodAuthHandler> m_methodAuth;
};


template <typename MethodAuthHandlerT=NoMethodAuth>
class ServiceMethodsAuthMultiple
{
    public:

        using MethodAuthHandler=MethodAuthHandlerT;

        template <typename ContextT, typename CallbackT, typename ServiceT, typename MessageT>
        void makeAuthHeader(
                common::SharedPtr<ContextT> ctx,
                CallbackT callback,
                common::SharedPtr<ServiceT> service,
                const Method& mthd,
                MessageT message,
                lib::string_view topic={},
                const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
            )
        {
            auto mthdAuth=methodAuth(mthd);
            if (!mthdAuth)
            {
                callback(std::move(ctx),Error{},MethodAuth{});
                return;
            }
            mthdAuth->makeAuthHandler(std::move(ctx),std::move(callback),std::move(service),mthd,std::move(message),topic,factory);
        }

        MethodAuthHandler* methodAuth(const Method& mthd) const
        {
            auto it=m_methods.find(mthd.name());
            if (it!=m_methods.end())
            {
                return it->second.get();
            }
            return nullptr;
        }

        std::shared_ptr<MethodAuthHandler> methodAuthShared(const Method& mthd) const
        {
            auto it=m_methods.find(mthd.name());
            if (it!=m_methods.end())
            {
                return it->second;
            }
            return std::shared_ptr<MethodAuthHandler>{};
        }

        void registerMethodAuth(const Method& mthd, std::shared_ptr<MethodAuthHandler> handler)
        {
            m_methods[mthd.name()]=std::move(handler);
        }

    private:

        common::FlatMap<Method::NameType,std::shared_ptr<MethodAuthHandler>> m_methods;
};

}

HATN_API_NAMESPACE_END

namespace std
{
    template <>
    struct less<HATN_API_NAMESPACE::ServiceMethod>
    {
        template <typename T1, typename T2>
        bool operator () (const T1& left, const T2& right) const noexcept
        {
            auto comp=left.name().compare(right.name());
            if (comp<0)
            {
                return true;
            }
            if (comp>0)
            {
                return false;
            }
            if (left.version()<right.version())
            {
                return true;
            }
            if (left.version()>right.version())
            {
                return false;
            }
            return false;
        }
    };

}

#endif // HATNAPIMETHODAUTH_H
