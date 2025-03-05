/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/server/request.h
  *
  */

/****************************************************************************/

#ifndef HATNAPISERVERREQUEST_H
#define HATNAPISERVERREQUEST_H

#include <functional>

#include <hatn/common/objecttraits.h>
#include <hatn/common/error.h>
#include <hatn/common/sharedptr.h>

#include <hatn/logcontext/context.h>

#include <hatn/dataunit/visitors.h>


#include <hatn/api/api.h>
#include <hatn/api/requestunit.h>
#include <hatn/api/protocol.h>
#include <hatn/api/server/response.h>
#include <hatn/api/server/env.h>

HATN_API_NAMESPACE_BEGIN

namespace server {

template <typename EnvT=SimpleEnv>
struct Request : public common::TaskSubcontext
{
    using Env=EnvT;

    Request(const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault())
        : response(factory),
          requestBuf(factory),
          routed(false),
          closeConnection(false)
    {
        requestBuf.setUseInlineBuffers(true);
    }

    common::StringOnStack subject;
    request::type unit;
    common::WeakPtr<common::TaskContext> connectionCtx;

    Response response;

    common::SharedPtr<Env> env;
    protocol::Header header;

    du::WireBufSolidShared requestBuf;
    bool routed;
    bool closeConnection;

    const auto& id() const
    {
        const auto& field=unit.field(request::id);
        return field.value();
    }

    common::ByteArrayShared rawDataShared() const
    {
        return requestBuf.sharedMainContainer();
    }

    common::ByteArray* rawData() const
    {
        return requestBuf.mainContainer();
    }

    common::ByteArrayShared message() const
    {
        const auto& messageField=unit.field(request::message);
        return messageField.skippedNotParsedContent();
    }

    common::ByteArrayShared authMessage() const
    {
        const auto& authField=unit.field(request::session_auth);
        if (authField.isSet())
        {
            const auto& contentField=authField.value().field(auth::content);
            return contentField.skippedNotParsedContent();
        }
        return common::ByteArrayShared{};
    }

    Error parseMessage()
    {
        Error ec;
        requestBuf.setSize(requestBuf.mainContainer()->size());
        du::io::deserialize(unit,requestBuf,ec);
        response.unit.setFieldValue(HATN_API_NAMESPACE::response::id,id());
        return ec;
    }

    ServiceNameAndVersion serviceNameAndVersion() const noexcept
    {
        return ServiceNameAndVersion{unit};
    }
};

template <typename EnvT=SimpleEnv>
using RequestContext=common::TaskContextType<Request<EnvT>,HATN_LOGCONTEXT_NAMESPACE::Context>;

template <typename EnvT=SimpleEnv>
inline auto allocateRequestContext(
        const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
    )
{
    return HATN_COMMON_NAMESPACE::allocateTaskContextType<RequestContext<EnvT>>(
        factory->objectAllocator<RequestContext<EnvT>>(),
        HATN_COMMON_NAMESPACE::subcontexts(
            HATN_COMMON_NAMESPACE::subcontext(factory),
            HATN_COMMON_NAMESPACE::subcontext()
            )
        );
}

template <typename EnvT=SimpleEnv>
using RouteCb=std::function<void (common::SharedPtr<RequestContext<EnvT>> request)>;

template <typename EnvT=SimpleEnv>
using RouteFh=std::function<void (common::SharedPtr<RequestContext<EnvT>> request, RouteCb<EnvT> cb)>;

} // namespace server

HATN_API_NAMESPACE_END

HATN_TASK_CONTEXT_DECLARE(HATN_API_NAMESPACE::server::Request<HATN_API_NAMESPACE::server::SimpleEnv>,HATN_API_EXPORT)

#endif // HATNAPISERVERREQUEST_H
