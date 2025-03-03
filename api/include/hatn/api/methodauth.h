/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/methodauth.h
  *
  */

/****************************************************************************/

#ifndef HATNAPIMETHODAUTH_H
#define HATNAPIMETHODAUTH_H

#include <hatn/common/objecttraits.h>

#include <hatn/api/api.h>
#include <hatn/api/service.h>
#include <hatn/api/method.h>
#include <hatn/api/message.h>
#include <hatn/api/auth.h>

HATN_API_NAMESPACE_BEGIN

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
            return std::move(wrapper);
        }
};

}

HATN_API_NAMESPACE_END

#endif // HATNAPIMETHODAUTH_H
