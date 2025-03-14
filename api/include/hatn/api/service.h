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

#include <hatn/api/api.h>
#include <hatn/api/withnameandversion.h>
#include <hatn/api/requestunit.h>

HATN_API_NAMESPACE_BEGIN

class Service : public WithNameAndVersion<protocol::ServiceNameLengthMax>
{
    public:

        using WithNameAndVersion<protocol::ServiceNameLengthMax>::WithNameAndVersion;
};

class ServiceNameAndVersion
{
    public:

        ServiceNameAndVersion(const protocol::request::type& req):req(req)
        {}

        const lib::string_view name() const noexcept
        {
            const auto& field=req.field(protocol::request::service);
            return field.value();
        }

        uint8_t version() const noexcept
        {
            return req.fieldValue(protocol::request::service_version);
        }

    private:

        const protocol::request::type& req;
};

HATN_API_NAMESPACE_END

namespace std
{

template <size_t NameLength>
struct less<HATN_API_NAMESPACE::WithNameAndVersion<NameLength>>
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

#endif // HATNAPISERVICE_H
