/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file mq/producerservice.ipp
  *
  *
  */

/****************************************************************************/

#ifndef HATNBMQPRODUCERSERVICE_IPP
#define HATNBMQPRODUCERSERVICE_IPP

#include <hatn/dataunit/visitors.h>

#include <hatn/db/updateserialization.h>
#include <hatn/db/ipp/updateserialization.ipp>

#include <hatn/mq/mqerror.h>
#include <hatn/mq/message.h>
#include <hatn/mq/producerservice.h>

HATN_MQ_NAMESPACE_BEGIN

namespace server {

//---------------------------------------------------------------

template <typename RequestT, typename ObjectHandlerT, typename NotifierT, typename MessageT>
validator::error_report ProducerMethodTraits<RequestT,ObjectHandlerT,NotifierT,MessageT>::validate(
        const common::SharedPtr<api::server::RequestContext<RequestT>>& request,
        const MessageT& msg
    ) const
{
    auto& req=request->template get<Request>();
    const auto& cfg=req.env->template get<MqConfig>();

/** @todo implement mq message validation
 *
 * object_type must be not empty
 * object_type must be the same as in ObjectHandlerT
 * object.object must be set for Operation::Create and Operation::Update at producer
 * producer_pos and producer must be set
 * datetime of producer_pos must be not too far in the past or in the future, check it with
 * cfg.config().fieldValue(mq_config::tolerated_time_offset)
 */
    return validator::error_report{};
}

//---------------------------------------------------------------

template <typename RequestT, typename ObjectHandlerT, typename NotifierT, typename MessageT>
void ProducerMethodTraits<RequestT,ObjectHandlerT,NotifierT,MessageT>::exec(
        common::SharedPtr<api::server::RequestContext<RequestT>> request,
        api::server::RouteCb<Request> callback,
        common::SharedPtr<Message> msg
    ) const
{
    auto& req=request->template get<Request>();

    // fill db message fields
    db::initObject(*msg);
    msg->setFieldValue(message::pos,msg->fieldValue(db::object::_id));
    msg->setFieldValue(server_db_message::sender,req.sender);
    msg->setFieldValue(server_db_message::session,req.sessionId);
    msg->setFieldValue(server_db_message::session_client,req.sessionClientId);

    // parse content
    common::SharedPtr<Object> obj;
    common::SharedPtr<db::update::message::shared_managed> updateObj;
    const auto& contentField=request->unit.field(message::content);
    if (contentField.isSet())
    {
        const auto& objField=contentField.value().field(message_content::object);
        du::WireBufSolidShared buf{objField.skippedNotParsedContent()};

        switch (msg->fieldValue(message::operation))
        {
            case(Operation::Create):
            {
                if (!du::io::deserialize(*obj,buf))
                {
                    //! @todo Log error
                    //! @todo decribe that object content is invalid
                    req.response.setStatus(api::protocol::ResponseStatus::FormatError);
                    cb(std::move(request));
                    return;
                }
            }
            break;

            case(Operation::Update):
            {
                if (!du::io::deserialize(*updateObj,buf))
                {
                    //! @todo Log error
                    //! @todo decribe that update content is invalid
                    req.response.setStatus(api::protocol::ResponseStatus::FormatError);
                    cb(std::move(request));
                    return;
                }
            }
            break;

            case(Operation::Delete): break;
        }
    }

    auto handleMsg=[callback{std::move(callback)},objHandlerW{common::toWeakPtr(objectHandler)},notifierW{common::toWeakPtr(notifier)}](
                            common::SharedPtr<api::server::RequestContext<RequestT>> request,
                            common::SharedPtr<Message> msg,
                            common::SharedPtr<Object> obj,
                            common::SharedPtr<db::update::message::shared_managed> updateObj,
                            const Error& ec
                            )
    {
        // check if handler still alive
        auto objHandler=objHandlerW.lock();
        if (!objHandler)
        {
            return;
        };

        // check error
        if (ec)
        {
            // logging must be done in object handler
            callback(std::move(request));
            return;
        }

        auto txCb=[request,msg,callback{std::move(callback)},notifierW{std::move(notifierW)},objHandlerW{common::toWeakPtr(objHandler)}](auto, const Error& ec)
        {
            // check if handler still alive
            auto objHandler=objHandlerW.lock();
            if (!objHandler)
            {
                return;
            };

            // check error
            if (ec)
            {
                //! @todo log error
                //! @todo if ec does not contain ApiError then construct api error
                req.response.setStatus(api::protocol::ResponseStatus::ServiceError,ec);
                cb(std::move(request));
                return;
            }

            // check if notifier still alive
            auto notifier=notifierW.lock();
            if (notifier)
            {
                // notify that mq message was received
                auto cb=[request,callback{std::move(callback)}]()
                {
                    // invoke callback
                    callback(std::move(request));
                };
                notifier->notify(request,std::move(msg),std::move(cb));
            }
            else
            {
                // invoke callback
                callback(std::move(request));
            }
        };
        auto& req=request->template get<Request>();
        auto dbClient=req.env->template get<api::server::Db>().dbClient(req.topic());
        auto txHandler=[request,dbClient,msg,obj{std::move(obj)},updateObj{std::move(updateObj)},objHandlerW{common::toWeakPtr(objHandler)}](db::Transaction* tx)
        {
            // check if handler still alive
            auto objHandler=objHandlerW.lock();
            if (!objHandler)
            {
                return Error{};
            };

            auto& req=request->template get<Request>();
            auto client=dbClient->client();

            Error ec;

            // check if producer_pos outdated
            auto q=db::makeQuery(msgProducerPosIdx(),db::where(message::producer,db::query::eq,msg->field(message::producer).value()).
                                                         and_(message::producer_pos,db::query::gte,msg->field(message::producer_pos).value()),
                                                         req.topic()
                                                        );
            //! @todo iplement exists() method in db
            auto r=client->findOne(messageModel(),q);
            if (r)
            {
                //! @todo log error
                return r.takeError();
            }
            if (!r->isNull())
            {
                // messages with later pos already exist, do nothing, just return success
                return Error{};
            }

            // save/update/delete object, note that message::pos must be set in object
            switch (msg->fieldValue(message::operation))
            {
                case(Operation::Create):
                {
                    obj->setFieldValue(mq_object::mq_pos,msg->field(message::pos).value());
                    ec=objHandler->dbCreate(
                        req.topic(),
                        client,
                        tx,
                        std::move(obj)
                    );
                }
                break;

                case(Operation::Update):
                {
                    // deserialize update request
                    db::update::Request updateReq;
                    auto res=db::update::deserialize(*updateObj,updateReq,req.env->template get<api::server::AllocatorFactory>().factory());
                    if (res)
                    {
                        //! @todo Log error
                        //! @todo decribe that update content is invalid
                        req.response.setStatus(api::protocol::ResponseStatus::FormatError);
                        return mqError(MqError::INVALID_UPDATE_MESSAGE);
                    }
                    updateReq.push_back(db::update::field(mq_object::mq_pos,db::update::set,msg->field(message::pos).value()));

                    // update object
                    ec=objHandler->dbUpdate(
                        req.topic(),
                        client,
                        tx,
                        msg->fieldValue(message::object_id),
                        updateReq
                    );
                }
                break;

                case(Operation::Delete):
                {
                    ec=objHandler->dbDelete(
                        req.topic(),
                        client,
                        tx,
                        msg->fieldValue(message::object_id)
                    );
                }
                break;
            }
            if (ec)
            {
                //! @todo log error
                return ec;
            }

            // save mq message in db
            ec=client->create(req.topic(),*msg,tx);
            if (ec)
            {
                //! @todo log error
            }

            // done tx
            return ec;
        };
        dbClient->transaction(request,std::move(txCb),std::move(txHandler),req.topic());
    };

    // call object handler to preprocess object content
    objectHandler->preprocess(
        std::move(request),
        std::move(handleMsg),
        std::move(msg),
        std::move(obj),
        std::move(updateObj)
    );
}

//---------------------------------------------------------------

template <typename Traits>
Error ObjectHandler<Traits>::dbCreate(
        lib::string_view topic,
        db::Client* dbClient,
        db::Transaction* tx,
        common::SharedPtr<Object> obj
    ) const
{
    if constexpr (decltype(has_dbCreate<Traits>(topic,dbClient,tx,obj))::value)
    {
        return this->traits().dbCreate(
                        topic,
                        dbClient,
                        tx,
                        std::move(obj)
                    );
    }
    else
    {
        return dbClient->create(topic,objectModel(),*obj,tx);
    }
}

//---------------------------------------------------------------

template <typename Traits>
Error ObjectHandler<Traits>::dbUpdate(
    lib::string_view topic,
    db::Client* dbClient,
    db::Transaction* tx,
    const du::ObjectId& oid,
    const db::update::Request& req
    ) const
{
    if constexpr (decltype(has_dbUpdate<Traits>(topic,dbClient,tx,oid,req))::value)
    {
        return this->traits().dbCreate(
                        topic,
                        dbClient,
                        tx,
                        oid,
                        req
                    );
    }
    else
    {
        return dbClient->update(topic,objectModel(),oid,req,tx);
    }
}

//---------------------------------------------------------------

template <typename Traits>
Error ObjectHandler<Traits>::dbDelete(
    lib::string_view topic,
    db::Client* dbClient,
    db::Transaction* tx,
    const du::ObjectId& oid
    ) const
{
    if constexpr (decltype(has_dbDelete<Traits>(topic,dbClient,tx,oid))::value)
    {
        return this->traits().dbDelete(
                        topic,
                        dbClient,
                        tx,
                        oid
                    );
    }
    else
    {
        return dbClient->deleteObject(topic,objectModel(),oid,tx);
    }
}

//---------------------------------------------------------------

} // namespace server

HATN_MQ_NAMESPACE_END

#endif // HATNBMQPRODUCERSERVICE_IPP
