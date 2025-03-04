/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/tenancy.h
  *
  */

/****************************************************************************/

#ifndef HATNAPITENANCY_H
#define HATNAPITENANCY_H

#include <hatn/common/allocatoronstack.h>

#include <hatn/api/api.h>
#include <hatn/api/apiconstants.h>

HATN_API_NAMESPACE_BEGIN

class Tenancy
{
    public:

        lib::string_view tenancyId() const noexcept
        {
            return m_tenancyId;
        }

        lib::string_view tenancyName() const noexcept
        {
            return m_tenancyName;
        }

        void setTenancyId(lib::string_view id)
        {
            m_tenancyId=id;
        }

        void setTenancyName(lib::string_view name)
        {
            m_tenancyName=name;
        }

        static const Tenancy& notTenancy()
        {
            static Tenancy v;
            return v;
        }

        template <typename ContextT>
        static const Tenancy& contextTenancy(const ContextT& ctx)
        {
            return hana::eval_if(
                ctx.template hasSubcontext<Tenancy>(),
                [&](auto _)
                {
                    return _(ctx).template get<Tenancy>();
                },
                []()
                {
                    return Tenancy::notTenancy();
                }
            );
        }

    private:

        common::StringOnStackT<TenancyIdLengthMax> m_tenancyId;
        common::StringOnStack m_tenancyName;
};

template <typename TenancyT=Tenancy>
class WithTenancy
{
    public:

        void setTenancy(common::SharedPtr<TenancyT> tenancy)
        {
            m_tenancy=std::move(tenancy);
        }

        common::SharedPtr<TenancyT> tenancyShared() const noexcept
        {
            return m_tenancy;
        }

        TenancyT* tenancy() const noexcept
        {
            return m_tenancy.get();
        }

    private:

        common::SharedPtr<TenancyT> m_tenancy;
};

HATN_API_NAMESPACE_END

#endif // HATNAPITENANCY_H
