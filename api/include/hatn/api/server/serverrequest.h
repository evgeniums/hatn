/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/server/serverrequest.h
  *
  */

/****************************************************************************/

#ifndef HATNAPISERVERREQUEST_H
#define HATNAPISERVERREQUEST_H

#include <functional>

#include <hatn/common/objecttraits.h>
#include <hatn/common/error.h>
#include <hatn/common/sharedptr.h>
#include <hatn/common/translate.h>

#include <hatn/logcontext/contextlogger.h>

#include <hatn/dataunit/visitors.h>

#include <hatn/api/api.h>
#include <hatn/api/apiliberror.h>
#include <hatn/api/requestunit.h>
#include <hatn/api/protocol.h>
#include <hatn/api/authprotocol.h>
#include <hatn/api/server/serverresponse.h>
#include <hatn/api/server/env.h>
#include <hatn/api/service.h>
#include <hatn/api/makeapierror.h>

HATN_API_NAMESPACE_BEGIN

namespace server {

struct ResponseError
{
    protocol::ResponseStatus status=protocol::ResponseStatus::InternalServerError;
    common::ApiError apiError;
    const common::ApiError* apiErrorPtr=nullptr;

    ~ResponseError()=default;
    ResponseError(const ResponseError&)=delete;
    ResponseError& operator=(const ResponseError&)=delete;

    ResponseError(ResponseError&& other)
    {
        moveOther(std::move(other));
    }

    ResponseError& operator=(ResponseError&& other)
    {
        if (this==&other)
        {
            return *this;
        }

        moveOther(std::move(other));

        return *this;
    }

    ResponseError(protocol::ResponseStatus status, const common::ApiError* apiErrorPtr)
        : status(status), apiErrorPtr(apiErrorPtr)
    {}

    ResponseError(protocol::ResponseStatus status, common::ApiError apiError)
        : status(status), apiError(std::move(apiError)), apiErrorPtr(&this->apiError)
    {}

    const common::ApiError* actualApiError() const
    {
        return apiErrorPtr;
    }

    void moveOther(ResponseError&& other)
    {
        if (other.apiErrorPtr!=&other.apiError)
        {
            apiErrorPtr=other.apiErrorPtr;
            apiError=std::move(other.apiError);
        }
        else
        {
            apiError=std::move(other.apiError);
            apiErrorPtr=&apiError;
        }
        status=other.status;
        other.apiErrorPtr=nullptr;
        other.status=protocol::ResponseStatus::InternalServerError;
    }
};

template <typename EnvT=BasicEnv, typename RequestUnitT=protocol::request::type>
struct Request : public common::TaskSubcontext
{
    using Env=EnvT;
    using RequestUnit=RequestUnitT;

    Request(common::SharedPtr<Env> env_)
        : env(std::move(env_)),
          unit(env->template get<AllocatorFactory>().factory()),
          response(this),          
          requestBuf(env->template get<AllocatorFactory>().factory()),
          routed(false),
          closeConnection(false),
          requestThread(nullptr),
          translator(nullptr),
          complete(false)
    {
        requestBuf.setUseInlineBuffers(true);
    }

    common::SharedPtr<Env> env;

    common::StringOnStack sender;
    RequestUnitT unit;
    common::WeakPtr<common::TaskContext> connectionCtx;

    Response<Env,RequestUnitT> response;
    protocol::Header header;

    du::WireBufSolidShared requestBuf;
    bool routed;
    bool closeConnection;

    common::ThreadQWithTaskContext* requestThread;

    du::ObjectId sessionId;
    du::ObjectId sessionClienttId;

    const common::Translator* translator;

    Error error;
    lib::optional<ResponseError> responseError;
    bool complete;

    void setResponseError(Error ec, protocol::ResponseStatus status=protocol::ResponseStatus::InternalServerError, bool overrideRespError=false)
    {
        if (error)
        {
            error=common::chainErrors(std::move(error),std::move(ec));
        }
        else
        {
            error=std::move(ec);
        }

        if (!responseError || overrideRespError)
        {
            responseError=ResponseError{status,error.apiError()};
        }
    }

    void setResponseError(Error ec, common::ApiError apiError, protocol::ResponseStatus status=protocol::ResponseStatus::InternalServerError, bool overrideRespError=false)
    {
        if (ec)
        {
            auto apiErrEc=cloneApiError(std::move(apiError),ApiLibError::SERVER_RESPONDED_WITH_ERROR,&ApiLibErrorCategory::getCategory());
            auto nextEc=common::chainErrors(std::move(ec),std::move(apiErrEc));
            setResponseError(std::move(nextEc),status,overrideRespError);
        }
        else
        {
            setResponseError(std::move(apiError),status,overrideRespError);
        }
    }

    void setResponseError(common::ApiError apiError, protocol::ResponseStatus status=protocol::ResponseStatus::ExecFailed, bool overrideRespError=false)
    {
        if (!responseError || overrideRespError)
        {
            responseError=ResponseError{status,std::move(apiError)};
        }
    }

    void setResponseError(protocol::ResponseStatus status, bool overrideRespError=false)
    {
        if (!responseError || overrideRespError)
        {
            responseError=ResponseError{status,error.apiError()};
        }
    }

    common::ThreadQWithTaskContext* thread() const
    {
        if (requestThread!=nullptr)
        {
            return requestThread;
        }
        if (env)
        {
            return env->template get<common::WithMappedThreads>().threads()->defaultThread();
        }
        return common::ThreadQWithTaskContext::current();
    }

    const auto& id() const
    {
        const auto& field=unit.field(protocol::request::id);
        return field.value();
    }

    auto& db()
    {
        return env->template get<Db>();
    }

    auto topic() const
    {
        const auto& field=unit.field(protocol::request::topic);
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
        const auto& messageField=unit.field(protocol::request::message);
        return messageField.skippedNotParsedContent();
    }

    common::ByteArrayShared authMessage() const
    {
        const auto& authField=unit.field(protocol::request::session_auth);
        if (authField.isSet())
        {
            const auto& contentField=authField.value().field(auth::content);
            return contentField.skippedNotParsedContent();
        }
        return common::ByteArrayShared{};
    }

    const auto& authField() const
    {
        return unit.field(protocol::request::session_auth);
    }

    AuthProtocol authProtocol() const
    {
        const auto& authField=unit.field(protocol::request::session_auth);
        if (authField.isSet())
        {
            AuthProtocol ret{authField.field(auth_protocol::protocol).value(),authField.field(auth_protocol::version).value()};
            return ret;
        }
        return AuthProtocol{};
    }

    Error parseMessage()
    {
        Error ec;
        requestBuf.setSize(requestBuf.mainContainer()->size());
        du::io::deserialize(unit,requestBuf,ec);
        response.unit.setFieldValue(HATN_API_NAMESPACE::protocol::response::id,id());
        return ec;
    }

    ServiceNameAndVersion serviceNameAndVersion() const noexcept
    {
        return ServiceNameAndVersion{unit};
    }

    void close(const Error& ec);

    Request& request()
    {
        return *this;
    }

    const Request& request() const
    {
        return *this;
    }

    template <typename T>
    T& envContext() noexcept
    {
        return env->template get<T>();
    }

    template <typename T>
    const T& envContext() const noexcept
    {
        return env->template get<T>();
    }

    const common::pmr::AllocatorFactory* factory() const noexcept
    {
        return envContext<AllocatorFactory>().factory();
    }

    void setResponseStatus();
};

template <typename RequestT=Request<>>
using RequestContext=common::TaskContextType<RequestT,HATN_LOGCONTEXT_NAMESPACE::Context>;

template <typename RequestT>
inline auto allocateRequestContext(
        common::SharedPtr<typename RequestT::Env> env
    )
{
    return HATN_COMMON_NAMESPACE::allocateTaskContextType<RequestContext<RequestT>>(
        env->template get<AllocatorFactory>().factory()->template objectAllocator<RequestContext<RequestT>>(),
        HATN_COMMON_NAMESPACE::subcontexts(
            HATN_COMMON_NAMESPACE::subcontext(env),
            HATN_COMMON_NAMESPACE::subcontext()
            )
        );
}

template <typename RequestT>
auto allocateAndInitRequestContext(common::SharedPtr<typename RequestT::Env> env)
{
    auto reqCtx=allocateRequestContext<RequestT>(std::move(env));
    auto& req=reqCtx->template get<RequestT>();
    req.requestThread=common::ThreadQWithTaskContext::current();
    auto& envLogger=req.env->template get<Logger>();
    auto& logCtx=reqCtx->template get<HATN_LOGCONTEXT_NAMESPACE::Context>();
    logCtx.setLogger(envLogger.logger());
    return reqCtx;
}

template <typename Request>
struct requestT
{
    template <typename T>
    auto& operator() (const T& context) const
    {
        return context->template get<Request>();
    }
};
template <typename Request>
constexpr requestT<Request> request{};

template <typename Request>
struct requestEnvT
{
    template <typename T>
    auto& operator() (const T& context) const
    {
        return context->template get<Request>().request().env;
    }
};
template <typename Request>
constexpr requestEnvT<Request> requestEnv{};


template <typename RequestT=Request<>>
using RouteCb=std::function<void (common::SharedPtr<RequestContext<RequestT>> request)>;

template <typename RequestT=Request<>>
using RouteFh=std::function<void (common::SharedPtr<RequestContext<RequestT>> request, RouteCb<RequestT> cb)>;

template <typename Traits=BasicEnvConfig, typename RequestT=Request<typename Traits::Env>>
struct EnvConfigT
{
    using Env=typename Traits::Env;
    using Request=RequestT;

    static Result<common::SharedPtr<Env>> makeEnv(
        const HATN_APP_NAMESPACE::App& app,
        const HATN_BASE_NAMESPACE::ConfigTree& configTree,
        const HATN_BASE_NAMESPACE::ConfigTreePath& configTreePath
        )
    {
        return Traits::makeEnv(app,configTree,configTreePath);
    }
};
using EnvConfig=EnvConfigT<>;

} // namespace server

HATN_API_NAMESPACE_END

HATN_TASK_CONTEXT_DECLARE(HATN_API_SERVER_NAMESPACE::Request<HATN_API_SERVER_NAMESPACE::BasicEnv>,HATN_API_EXPORT)

#endif // HATNAPISERVERREQUEST_H
