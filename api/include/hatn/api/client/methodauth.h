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

        template <typename MessageT>
        common::Result<MethodAuth> makeAuthHeader(
            const Service& service,
            const Method& method,
            MessageT message,
            lib::string_view topic={},
            const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
        )
        {
            auto r=this->traits().makeAuthHeader(service,method,message,topic);
            HATN_CHECK_RESULT(r)
            MethodAuth wrapper;
            auto ec=wrapper.serializeAuthHeader(this->traits().protocol(),this->traits().protocolVersion,r.takeValue(),factory);
            HATN_CHECK_EC(ec)
            return wrapper;
        }
};

class NoMethodAuthTraits
{
    public:

        template <typename MessageT>
        common::Result<MethodAuth> makeAuthHeader(
                const Service& /*service*/,
                const Method& /*method*/,
                MessageT /*message*/,
                lib::string_view /*topic*/={},
                const common::pmr::AllocatorFactory* /*factory*/=common::pmr::AllocatorFactory::getDefault()
            )
        {
            return MethodAuth{};
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

template <typename MethodAuthHandlerT=MethodAuthHandler<NoMethodAuthTraits>>
class ServiceMethodsAuth
{
    public:

        using MethodAuthHandler=MethodAuthHandlerT;

        ServiceMethodsAuth(const Service* service=nullptr) : m_service(service)
        {}

        void setService(const Service* service) noexcept
        {
            m_service=service;
        }

        const Service* service() const noexcept
        {
            return m_service;
        }

        template <typename MessageT>
        common::Result<MethodAuth> makeAuthHeader(
            const Method& mthd,
            MessageT message,
            lib::string_view topic={},
            const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
            )
        {
            auto m=method(mthd);
            if (m==nullptr)
            {
                return MethodAuth{};
            }
            return m->makeAuthHandler(*m_service,mthd,message,topic,factory);
        }

        MethodAuthHandler* method(const Method& mthd) const
        {
            auto it=m_methods.find(mthd.name());
            if (it!=m_methods.end())
            {
                return it->second;
            }
            return nullptr;
        }

        void registerMethod(const Method& mthd, std::shared_ptr<MethodAuthHandler> handler)
        {
            m_methods[mthd.name()]=std::move(handler);
        }

    private:

        const Service* m_service;
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
