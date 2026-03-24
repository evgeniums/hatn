/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/ipp/rawtransport.ipp
  *
  */

/****************************************************************************/

#ifndef HATNAPIRAWTRANSPORT_IPP
#define HATNAPIRAWTRANSPORT_IPP

#include <hatn/common/meta/chain.h>

#include <hatn/base/configobject.h>

#include <hatn/dataunit/unitwrapper.h>
#include <hatn/dataunit/syntax.h>

#include <hatn/api/connectionpool.h>
#include <hatn/api/client/rawtransport.h>

HATN_API_NAMESPACE_BEGIN

namespace client {

//---------------------------------------------------------------

template <typename RouterT, typename Traits>
template <typename RequestT, typename CallbackT>
void RawTransport<RouterT,Traits>::sendRequest(
        common::SharedPtr<RequestT> req,
        CallbackT callback
    )
{
    auto send=[this](auto&& recv, common::SharedPtr<RequestT> req, CallbackT callback)
    {
        auto reqPtr=req.get();
        m_connectionPool.send(
            reqPtr->taskCtx,
            reqPtr->priority(),
            reqPtr->spanBuffers(),
            [recv=std::move(recv),req=std::move(req),callback=std::move(callback)](const Error& ec, auto connection) mutable
            {
                if (ec)
                {
                    callback(ec);
                    return;
                }

                // receive response
                recv(std::move(req),std::move(callback),connection);
            }
        );
    };

    auto recv=[this](common::SharedPtr<RequestT> req, CallbackT callback, auto connection)
    {
        auto reqPtr=req.get();

        m_connectionPool.recv(
            reqPtr->taskCtx,
            std::move(connection),
            reqPtr->responseData,
            std::move(callback)
        );
    };

    auto chain=hatn::chain(
        std::move(send),
        std::move(recv)
    );
    chain(std::move(req),std::move(callback));
}

//---------------------------------------------------------------

template <typename RouterT, typename Traits>
template <typename RequestT>
Error RawTransport<RouterT,Traits>::serializeRequest(
        common::SharedPtr<RequestT> req,
        lib::string_view topic,
        const Tenancy& tenancy
    )
{
    return req->serialize(topic,tenancy);
}

//---------------------------------------------------------------

template <typename RouterT, typename Traits>
template <typename RequestT>
Error RawTransport<RouterT,Traits>::serializeRequest(
        common::SharedPtr<RequestT> req
    )
{
    return req->serialize();
}

//---------------------------------------------------------------

template <typename RouterT, typename Traits>
template <typename RequestT>
Error RawTransport<RouterT,Traits>::parseResponse(
        common::SharedPtr<RequestT> req
    )
{
    Error ec;
    auto respUnit=req->factory()->template createObject<ResponseManaged>(req->factory());
    du::io::deserialize(*respUnit,req->responseData,ec);
    HATN_CHECK_EC(ec)

    Response resp{};
    resp.setId(std::string{respUnit->fieldValue(protocol::response::id)});
    resp.setStatus(respUnit->fieldValue(protocol::response::status));
    resp.setMessageType(std::string{respUnit->fieldValue(protocol::response::message_type)});
    resp.setMessageData(respUnit->field(protocol::response::message).skippedNotParsedContent());
    req->setResponse(std::move(resp));

    auto parseError=[&resp]()
    {
        const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault();

        // if error message field not set then construct api error from response status
        if (resp.messageType()!=protocol::response_error_message::conf().name || resp.messageData().isNull())
        {
            return makeApiError(
                ApiLibError::SERVER_RESPONDED_WITH_ERROR,
                ApiLibErrorCategory::getCategory(),
                resp.status(),
                ApiGenericErrorCategory::getCategory()
            );
        }

        // if error message field is set then parse it
        du::WireBufSolidShared buf{resp.messageData()};
        protocol::response_error_message::managed errUnit{factory};
        if (!du::io::deserialize(errUnit,buf))
        {
            return apiLibError(ApiLibError::FAILED_DESERIALIZE_RESPONSE_ERROR);
        }

        // make api error from response_error_message
        auto nativeError=std::make_shared<common::NativeError>(ApiLibError::SERVER_RESPONDED_WITH_ERROR,&ApiLibErrorCategory::getCategory());
        common::ApiError apiError{errUnit.fieldValue(protocol::response_error_message::code)};
        nativeError->setApiError(std::move(apiError));
        nativeError->mutableApiError()->setDescription(std::string{errUnit.fieldValue(protocol::response_error_message::description)});
        nativeError->mutableApiError()->setFamily(std::string{errUnit.fieldValue(protocol::response_error_message::family)});
        nativeError->mutableApiError()->setStatus(std::string{errUnit.fieldValue(protocol::response_error_message::status)});
        nativeError->mutableApiError()->setDataType(std::string{errUnit.fieldValue(protocol::response_error_message::data_type)});
        const auto& dataField=errUnit.field(protocol::response_error_message::data);
        if (dataField.isSet())
        {
            nativeError->mutableApiError()->setData(dataField.byteArrayShared());
        }
        return Error{ApiLibError::SERVER_RESPONDED_WITH_ERROR,std::move(nativeError)};
    };

    if (!resp.isSuccess())
    {
        resp.setError(parseError());
    }

    return OK;
}

//---------------------------------------------------------------

template <typename RouterT, typename Traits>
Error RawTransport<RouterT,Traits>::loadLogConfig(
    const HATN_BASE_NAMESPACE::ConfigTree& configTree,
    const std::string& configPath,
    HATN_BASE_NAMESPACE::config_object::LogRecords& records,
    const HATN_BASE_NAMESPACE::config_object::LogSettings& settings
    )
{
    auto ec=base::ConfigObject<raw_transport_config::type>::loadLogConfig(configTree,configPath,records,settings);
    HATN_CHECK_EC(ec)
    m_connectionPool.setMaxConnectionsPerPriority(config().fieldValue(raw_transport_config::max_pool_priority_connections));

    return OK;
}

//---------------------------------------------------------------

template <typename RouterT, typename Traits>
bool RawTransport<RouterT,Traits>::canSend(Priority p) const
{
    return m_connectionPool.canSend(p);
}

//---------------------------------------------------------------

template <typename RouterT, typename Traits>
template <typename ContextT, typename CallbackT>
void RawTransport<RouterT,Traits>::close(
    common::SharedPtr<ContextT> ctx,
    CallbackT callback
    )
{
    m_connectionPool.close(std::move(ctx),std::move(callback));
}

//---------------------------------------------------------------

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPIRAWTRANSPORT_IPP
