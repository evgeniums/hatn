/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file mq/producerclient.h
  *
  *
  */

/****************************************************************************/

#ifndef HATNBMQPRODUCERCLIENT_H
#define HATNBMQPRODUCERCLIENT_H

#include <hatn/common/pmr/allocatorfactory.h>
#include <hatn/common/taskcontext.h>

#include <hatn/dataunit/syntax.h>

#include <hatn/db/asyncclient.h>
#include <hatn/db/ipp/updateserialization.ipp>
#include <hatn/db/ipp/updateunit.ipp>

#include <hatn/mq/mq.h>
#include <hatn/mq/mqerror.h>
#include <hatn/mq/message.h>

HATN_MQ_NAMESPACE_BEGIN

HDU_UNIT(producer_config,
    HDU_FIELD(dequeue_retry_interval,TYPE_UINT32,1,false,15)
    HDU_FIELD(publish_ttl,TYPE_UINT32,2,false,900)
)

template <typename ServerT, typename SchedulerT, typename NotifierT, typename MessageT=message::managed>
struct Traits
{
    using Server=ServerT;
    using Scheduler=SchedulerT;
    using Notifier=NotifierT;
    using Message=MessageT;
};

template <typename Traits>
class ProducerClient : public common::pmr::WithFactory,
                       public db::WithAsyncClient,
                       public common::TaskSubcontext
{
    public:

        using LocalStorage=typename Traits::LocalStorage;
        using Server=typename Traits::Server;
        using Scheduler=typename Traits::Scheduler;
        using Notifier=typename Traits::Notifier;
        using Message=typename Traits::Message;

        ProducerClient(
                db::AsyncClient* dbClient=nullptr,
                const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
            ) : common::pmr::WithFactory(factory),
                db::WithAsyncClient(dbClient),
                m_stopped(false)
        {}

        ProducerClient(
                const common::ConstDataBuf& producerId,
                db::AsyncClient* dbClient=nullptr,
                const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
            ) : ProducerClient(dbClient,factory)
        {
            setProducerId(producerId);
        }

        void setProducerId(const common::ConstDataBuf& id)
        {
            m_producerId.set(id);
        }

        const du::ObjectId& producerId() const noexcept
        {
            return m_producerId;
        }

        template <typename ContextT, typename CallbackT, typename ContentT=du::Unit, typename NofificationT=du::Unit>
        void post(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            std::string topic,
            Operation op,
            lib::string_view objectId,
            lib::string_view objectType,
            common::SharedPtr<ContentT> objectContent={},
            common::SharedPtr<NofificationT> notificationContent={},
            uint32_t ttl=0
        )
        {
            // create message
            auto msg=this->factory()->template createObject<Message>();
            db::initObject(*msg);

            // set producer's fields
            msg->field(message::producer).set(m_producerId);
            msg->field(message::producer_pos).mutableValue()->generate();
            msg->field(message::pos).set(msg->field(message::producer_pos).value());

            // set fields from arguments
            msg->field(message::operation).set(op);
            msg->field(message::object_id).set(objectId);
            msg->field(message::object_type).set(objectType);

            // set message content
            common::SharedPtr<message_content::managed> msgContent;
            auto prepareContent=[this,&msgContent,&msg]()
            {
                if (!msgContent)
                {
                    msgContent=this->factory()->template createObject<message_content::managed>();
                    msg->field(message::content).set(msgContent);
                }
            };
            if (objectContent)
            {
                prepareContent();

                // serialize for update, use as is otherwise
                hana::eval_if(
                    hana::is_a<db::update::RequestTag,ContentT>,
                    [&](auto _)
                    {
                        Assert(op==Operation::Update,"Invalid object content for Update operation");

                        auto subunit=this->factory()->template createObject<db::update::message::managed>();
                        auto ec=db::update::serialize(*_(objectContent),*subunit);
                        if (ec)
                        {
                            //! @todo Log error
                            _(cb)(_(ctx),ec,_(msg)->field(message::pos).value());
                            return;
                        }
                        _(msgContent)->field(message_content::object).set(std::move(subunit));
                    },
                    [&](auto _)
                    {
                        Assert(op!=Operation::Update,"Invalid object content for non-Update operation");
                        _(msgContent)->field(message_content::object).set(_(objectContent));
                    }
                );
            }
            if (notificationContent)
            {
                prepareContent();

                // serialize for update, use as is otherwise
                hana::eval_if(
                    hana::is_a<db::update::RequestTag,NofificationT>,
                    [&](auto _)
                    {
                        Assert(op==Operation::Update,"Invalid notification content for Update operation");

                        auto subunit=this->factory()->template createObject<db::update::message::managed>();
                        auto ec=db::update::serialize(*_(notificationContent),*subunit);
                        if (ec)
                        {
                            //! @todo Log error
                            _(cb)(_(ctx),ec,_(msg)->field(message::pos).value());
                            return;
                        }
                        _(msgContent)->field(message_content::notification).set(std::move(subunit));
                    },
                    [&](auto _)
                    {
                        Assert(op!=Operation::Update,"Invalid notification content for non-Update operation");
                        _(msgContent)->field(message_content::notification).set(_(notificationContent));
                    }
                );
            }

            // set expiration
            if (ttl!=0)
            {
                auto dt=common::DateTime::currentUtc();
                dt.addSeconds(ttl);
                msg->field(message::expire_at).set(dt);
            }

            // save in db
            auto txCb=[ctx,selfCtx{this->sharedMainCtx()},cb{std::move(cb)},msg](auto, const Error& ec)
            {
                if (ec)
                {
                    //! to Log error
                }

                // callback with pos and error status
                cb(std::move(ctx),ec,msg->fieldValue(message::pos));
            };
            auto txHandler=[this,topic{std::move(topic)},msg,ctx,objectContent{std::move(objectContent)},notificationContent{std::move(notificationContent)}](db::Transaction* tx)
            {
                auto client=m_db->client();
                auto objectQuery=db::makeQuery(objectIdOpIdx(),db::where(message::object_id,db::query::eq,msg->fieldValue(message::object_id)),topic);
                auto findCreateOpQuery=db::makeQuery(objectIdOpIdx(),db::where(message::object_id,db::query::eq,msg->fieldValue(message::object_id)).
                                                                        and_(message::operation,db::query::eq,Operation::Create)
                                                       ,topic);

                switch (msg->fieldValue(message::operation))
                {
                    case (Operation::Create):
                    {
                        // check if object ID is unique
                        auto r=client->findOne(mqMessageModel(),objectQuery);

                        // check error
                        if (r)
                        {
                            //! @todo Log error
                            return r.takeError();
                        }

                        // check if duplicate
                        if (!r->isNull())
                        {
                            auto ec=mqError(MqError::DUPLICATE_OBJECT_ID);
                            //! @todo Log error
                            return ec;
                        }

                        // save message in db (below the switch)
                    }
                    break;

                    case (Operation::Delete):
                    {
                        // delete all pending messages for that object ID
                        auto ec=client->deleteMany(mqMessageModel(),objectQuery,tx);
                        if (ec)
                        {
                            //! @todo Log error
                            return ec;
                        }

                        // save message in db (below the switch)
                    }
                    break;

                    case (Operation::Update):
                    {
                        bool updateExisting=false;
                        auto findCb=[this,&updateExisting,&topic,tx,client](db::DbObject obj, Error& ec)
                        {
                            if (!obj.isNull())
                            {
                                // "create" operation message is still in queue, update it
                                updateExisting=true;
                                auto msg=obj.as<db_message::managed>();

                                // prepare update request for updating message in local db
                                db::update::Request req;
                                auto& contentField=msg->field(message::content);
                                if (contentField.isSet())
                                {
                                    auto* contentUnit=contentField.mutableValue();
                                    auto& objectContentField=contentUnit->field(message_content::object);
                                    if (objectContent && objectContentField.isSet())
                                    {
                                        db::update::apply(objectContentField.mutableValue(),*objectContent);
                                        req.push_back(db::update::field(db::nested(message::content,message_content::object),db::update::set,db::Subunit(objectContentField.nativeValue())));
                                    }
                                    auto& notificationContentField=contentUnit->field(message_content::notification);
                                    if (notificationContent && notificationContentField.isSet())
                                    {
                                        db::update::apply(notificationContentField.mutableValue(),*notificationContent);
                                        req.push_back(db::update::field(db::nested(message::content,message_content::object),db::update::set,db::Subunit(notificationContentField.nativeValue())));
                                    }
                                }

                                // update message in local db
                                ec=client->update(topic,mqMessageModel(),msg->fieldValue(db::object::_id),req,tx);
                                if (ec)
                                {
                                    //! @todo report error
                                }

                                // only one message should be processed
                                return false;
                            }
                        };

                        // check if Create message for that object ID exists
                        auto ec=client->findCb(mqMessageModel(),findCreateOpQuery,std::move(findCb),tx,true);
                        if (ec)
                        {
                            //! @todo Log error
                            return ec;
                        }

                        if (updateExisting)
                        {
                            return Error{};
                        }

                        // otherwise just save message in db (below the switch)
                    }
                    break;
                }

                // create message in db
                auto ec=client->create(topic,mqMessageModel(),msg.get(),tx);
                if (ec)
                {
                    //! @todo Log error
                }
                return ec;
            };
            this->dbClient()->transaction(ctx,std::move(txHandler),std::move(txCb));
        }

        template <typename ContextT, typename CallbackT>
        void publish(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            std::string topic,
            Operation op,
            lib::string_view objectId,
            lib::string_view objectType,
            uint32_t ttl
            )
        {
            publish(std::move(ctx),std::move(cb),std::move(topic),op,objectId,objectType,{},{},ttl);
        }

        template <typename ContextT, typename CallbackT, typename ContentT=du::Unit>
        void publish(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            std::string topic,
            Operation op,
            lib::string_view objectId,
            lib::string_view objectType,
            common::SharedPtr<ContentT> objectContent,
            uint32_t ttl
            )
        {
            publish(std::move(ctx),std::move(cb),std::move(topic),op,objectId,objectType,std::move(objectContent),{},ttl);
        }

        template <typename ContextT, typename JobT, typename CallbackT>
        void invokeScheduledJob(
                common::SharedPtr<ContextT> ctx,
                common::SharedPtr<JobT> jobObj,
                CallbackT cb
            )
        {

        }

        void start()
        {

        }

        void stop()
        {

        }

        template <typename ContextT, typename CallbackT>
        void removeLocalExpired(common::SharedPtr<ContextT> ctx, CallbackT cb, lib::string_view topic)
        {

        }

        template <typename ContextT, typename CallbackT>
        void removeLocalPos(common::SharedPtr<ContextT> ctx, CallbackT cb, lib::string_view topic, const du::ObjectId& pos)
        {
        }

        template <typename ContextT, typename CallbackT>
        void removeLocal(common::SharedPtr<ContextT> ctx, CallbackT cb, lib::string_view topic, const common::pmr::vector<du::ObjectId>& objIds={})
        {
        }

        template <typename ContextT, typename CallbackT>
        void readLocal(common::SharedPtr<ContextT> ctx, CallbackT cb, lib::string_view topic, const common::pmr::vector<du::ObjectId>& objIds={})
        {
        }

    private:

        template <typename ContextT>
        void dequeue(common::SharedPtr<ContextT> ctx)
        {
            // check if message is expired

            // send to server

            // remove from storage on success

            // notify that mq_item was sent
        }

        du::ObjectId m_producerId;

        db::AsyncClient* m_db;

        bool m_running;
        bool m_stopped;
};

HATN_MQ_NAMESPACE_END

#endif // HATNBMQPRODUCERCLIENT_H
