/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/server/serverservice.h
  *
  */

/****************************************************************************/

#ifndef HATNAPISERVERSERVICE_H
#define HATNAPISERVERSERVICE_H

#include <hatn/api/api.h>
#include <hatn/api/service.h>
#include <hatn/api/server/serverrequest.h>

HATN_API_NAMESPACE_BEGIN

namespace server
{

enum class ServiceMethodStatus : uint8_t
{
    Ok=0,
    UnsupportedMethod=1,
    MessageMissing=2,
    InvalidMessageType=3,
    MessageParsingError=4
};

struct NoValidatorT
{
    template <typename MessageT>
    Error operator()(const MessageT&) const
    {
        return OK;
    }
};
constexpr NoValidatorT NoValidator{};

template <typename Traits>
class ServerService : public Service,
                      public common::WithTraits<Traits>
{
    public:

        using Service::Service;

        template <typename ...TraitsArgs>
        ServerService(
            lib::string_view name,
            TraitsArgs&& ...traitsArgs
        ) : Service(name,version),
            common::WithTraits<Traits>::WithTraits(this,std::forward<TraitsArgs>(traitsArgs)...)
        {}

        template <typename RequestT>
        void handleRequest(
            common::SharedPtr<api::server::RequestContext<RequestT>> request,
            api::server::RouteCb<RequestT> callback
        ) const;

#if 0
        template <typename RequestT>
        void prepareMessage(
            lib::string_view method, bool messageExists, lib::string_view messageType) const
        {
            auto handleMessage=[](ServiceMethodStatus status, auto msg, auto validator, auto handler)
            {
                // check status

                // parse message

                // validate message

                // invoke handler
                handler();
            };
            return this->traits.prepareMessage(method,messageExists,messageType,handleMessage);
        }
#endif
        template <typename MessageT,typename RequestT>
        Error parseMessage(const common::SharedPtr<api::server::RequestContext<RequestT>>& request, common::SharedPtr<MessageT>& msg) const;

        template <typename MessageT,typename RequestT,typename ValidatorT=NoValidatorT>
        Error parseValidateMessage(const common::SharedPtr<api::server::RequestContext<RequestT>>& request, common::SharedPtr<MessageT>& msg,
                                   const ValidatorT& vld=NoValidator) const;

    private:

        /**
         * @brief prepareMessage
         * @param request
         * @param method
         * @param messageType
         * @param messageHandler Is callable type with signature: void (ServiceMethodStatus status, HandlerT handler, auto msg, auto validator)
         * where
         * HandlerT is a callable type with signature: void(common::SharedPtr<api::server::RequestContext<RequestT>> request, auto msg, CallbackT cb)
         * where HandlerT is a callable type with signature: void (common::SharedPtr<api::server::RequestContext<RequestT>> request, const Error& ec)
         */
        template <typename RequestT, typename MessageHandlerT>
        void prepareHandler(
            const common::SharedPtr<api::server::RequestContext<RequestT>>& request,
            lib::string_view method,
            bool messageExists,
            lib::string_view messageType,
            MessageHandlerT messageHandler
        ) const
        {
            return this->traits.prepareHandler(request,method,messageExists,messageType,messageHandler);
        }

        template <typename MessageT,typename RequestT,typename HandlerT>
        Error execMessage(
            common::SharedPtr<api::server::RequestContext<RequestT>> request,
            common::SharedPtr<MessageT> msg,
            HandlerT handler,
            api::server::RouteCb<RequestT> callback
        ) const;
};

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPISERVERSERVICE_H
