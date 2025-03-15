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

#include <hatn/api/server/serverrequest.h>
#include <hatn/api/server/serverservice.h>

#include <hatn/mq/mq.h>
#include <hatn/mq/message.h>

HATN_MQ_NAMESPACE_BEGIN

namespace server {

HDU_UNIT_WITH(server_db_message,(HDU_BASE(db::object),HDU_BASE(message)),
    HDU_FIELD(subject,TYPE_STRING,48)
    HDU_FIELD(session,TYPE_OBJECT_ID,49)
    HDU_FIELD(session_agent,TYPE_OBJECT_ID,50)
)

HATN_DB_UNIQUE_INDEX(msgPosIdx,message::pos,message::object_type)

HATN_DB_INDEX(msgOidOpPosIdx,message::object_id,message::operation,message::pos)
HATN_DB_INDEX(msgObjTypeOpPosIdx,message::object_type,message::operation,message::pos)
HATN_DB_INDEX(msgOpPosObjTypeIdx,message::operation,message::pos,message::object_type)

HATN_DB_INDEX(msgSubjPosIdx,server_db_message::subject,message::pos)
HATN_DB_INDEX(msgSessPosIdx,server_db_message::session,message::pos)
HATN_DB_INDEX(msgSessAgentPosIdx,server_db_message::session_agent,message::pos)

HATN_DB_MODEL_WITH_CFG(serverMqMessageModel,server_db_message,db::ModelConfig("server_mq_messages"),
                       msgPosIdx(),
                       msgProducerPosIdx(),
                       msgOidOpPosIdx(),
                       msgObjTypeOpPosIdx(),
                       msgOpPosObjTypeIdx(),
                       msgSubjPosIdx(),
                       msgSessPosIdx(),
                       msgSessAgentPosIdx()
                )

template <typename RequestT, typename MessageT=server_db_message::managed>
struct ProducerMethodTraits
{
    using Request=RequestT;
    using Message=MessageT;

    static const auto& messageModel()
    {
        return messageModel();
    }

    void exec(
        common::SharedPtr<api::server::RequestContext<Request>> request,
        api::server::RouteCb<Request> callback,
        common::SharedPtr<Message> msg
    ) const;

    validator::error_report validate(
        const common::SharedPtr<api::server::RequestContext<Request>>& request,
        const MessageT& msg
    ) const;
};

template <typename RequestT, typename MessageT=server_db_message::managed>
using ProducerMethod=api::server::ServiceMethodT<RequestT,ProducerMethodTraits<RequestT,MessageT>,MessageT>;

template <typename RequestT, typename MessageT=server_db_message::managed>
using ProducerService=api::server::ServiceSingleMethod<RequestT,ProducerMethod<RequestT,MessageT>>;

template <typename RequestT, typename MessageT=server_db_message::managed>
using ProducerServiceV=api::server::ServerServiceV<RequestT,ProducerService<RequestT,MessageT>>;

} // namespace server

HATN_MQ_NAMESPACE_END

#endif // HATNBMQPRODUCERSERVICE_H
