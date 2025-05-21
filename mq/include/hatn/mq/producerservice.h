/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file mq/producerservice.h
  *
  *
  */

/****************************************************************************/

#ifndef HATNBMQPRODUCERSERVICE_H
#define HATNBMQPRODUCERSERVICE_H

#include <hatn/common/meta/hasmethod.h>

#include <hatn/base/configobject.h>

#include <hatn/db/client.h>

#include <hatn/api/server/serverrequest.h>
#include <hatn/api/server/serverservice.h>

#include <hatn/mq/mq.h>
#include <hatn/mq/message.h>

HATN_MQ_NAMESPACE_BEGIN

namespace server {

HDU_UNIT_WITH(server_db_message,(HDU_BASE(db::object),HDU_BASE(message)),
    HDU_FIELD(sender,TYPE_STRING,48)
    HDU_FIELD(session,TYPE_OBJECT_ID,49)
    HDU_FIELD(session_client,TYPE_OBJECT_ID,50)
)

HATN_DB_UNIQUE_INDEX(msgPosIdx,message::pos,message::object_type)

HATN_DB_INDEX(msgOidOpPosIdx,message::object_id,message::operation,message::pos)
HATN_DB_INDEX(msgObjTypeOpPosIdx,message::object_type,message::operation,message::pos)
HATN_DB_INDEX(msgOpPosObjTypeIdx,message::operation,message::pos,message::object_type)

HATN_DB_INDEX(msgSenderPosIdx,server_db_message::sender,message::pos)
HATN_DB_INDEX(msgSessPosIdx,server_db_message::session,message::pos)
HATN_DB_INDEX(msgSessClientPosIdx,server_db_message::session_client,message::pos)

HATN_DB_MODEL_WITH_CFG(serverMqMessageModel,server_db_message,db::ModelConfig("server_mq_messages"),
                       msgPosIdx(),
                       msgProducerPosIdx(),
                       msgOidOpPosIdx(),
                       msgObjTypeOpPosIdx(),
                       msgOpPosObjTypeIdx(),
                       msgSenderPosIdx(),
                       msgSessPosIdx(),
                       msgSessClientPosIdx()
                )

constexpr const uint32_t DefaultToleratedTimeOffset=60*60*24*15; // 15 days in seconds

HDU_UNIT(mq_config,
    HDU_FIELD(tolerated_time_offset,TYPE_UINT32,1,false,DefaultToleratedTimeOffset)
)

class MqConfig : public HATN_BASE_NAMESPACE::ConfigObject<mq_config::type>
{};

template <typename Traits>
class ObjectHandler : public common::WithTraits<Traits>
{
    public:

        using Object=typename Traits::Object;

        static const auto& objectModel()
        {
            return Traits::objectModel();
        }

        using common::WithTraits<Traits>::WithTraits;

        template <typename RequestT, typename CallbackT, typename MessageT, typename UpdateObjectT>
        void preprocess(
            common::SharedPtr<RequestT> request,
            CallbackT callback,
            common::SharedPtr<MessageT> msg,
            common::SharedPtr<Object> obj,
            common::SharedPtr<UpdateObjectT> updateObj
        ) const
        {
            this->traits().preprocess(
                std::move(request),
                std::move(callback),
                std::move(msg),
                std::move(obj),
                std::move(updateObj)
            );
        }

        Error dbCreate(
            lib::string_view topic,
            db::Client* dbClient,
            db::Transaction* tx,
            common::SharedPtr<Object> obj
        ) const;

        Error dbUpdate(
            lib::string_view topic,
            db::Client* dbClient,
            db::Transaction* tx,
            const du::ObjectId& oid,
            const db::update::Request& req
        ) const;

        Error dbDelete(
            lib::string_view topic,
            db::Client* dbClient,
            db::Transaction* tx,
            const du::ObjectId& oid
        ) const;

    private:

        HATN_PREPARE_HAS_METHOD(dbCreate)
        HATN_PREPARE_HAS_METHOD(dbUpdate)
        HATN_PREPARE_HAS_METHOD(dbDelete)
};

template <typename RequestT, typename ObjectHandlerT, typename NotifierT, typename MessageT=server_db_message::managed>
struct ProducerMethodTraits
{
    using Request=RequestT;
    using Message=MessageT;
    using ObjectHandler=ObjectHandlerT;
    using Object=typename ObjectHandler::Object;
    using Notifier=NotifierT;

    ProducerMethodTraits(
            std::shared_ptr<ObjectHandler> objectHandler,
            std::shared_ptr<NotifierT> notifier
        ) : objectHandler(std::move(objectHandler)),
            notifier(std::move(notifier))
    {}

    static const auto& messageModel()
    {
        return serverMqMessageModel();
    }

    void exec(
        common::SharedPtr<api::server::RequestContext<Request>> request,
        api::server::RouteCb<Request> callback,
        common::SharedPtr<Message> msg
    ) const;

    HATN_VALIDATOR_NAMESPACE::error_report validate(
        const common::SharedPtr<api::server::RequestContext<Request>>& request,
        const MessageT& msg
    ) const;

    std::shared_ptr<ObjectHandler> objectHandler;
    std::shared_ptr<NotifierT> notifier;
};

template <typename RequestT, typename ObjectHandlerT, typename NotifierT, typename MessageT=server_db_message::managed>
using ProducerMethod=api::server::ServiceMethodT<ProducerMethodTraits<RequestT,ObjectHandlerT,NotifierT,MessageT>,MessageT,RequestT>;

template <typename RequestT, typename ObjectHandlerT, typename NotifierT, typename MessageT=server_db_message::managed>
using ProducerService=api::server::ServiceSingleMethod<ProducerMethod<RequestT,ObjectHandlerT,NotifierT,MessageT>,RequestT>;

template <typename RequestT, typename ObjectHandlerT, typename NotifierT, typename MessageT=server_db_message::managed>
using ProducerServiceV=api::server::ServerServiceV<ProducerService<RequestT,ObjectHandlerT,NotifierT,MessageT>,RequestT>;

} // namespace server

HATN_MQ_NAMESPACE_END

#endif // HATNBMQPRODUCERSERVICE_H
