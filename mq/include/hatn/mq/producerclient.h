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

#include <hatn/mq/scheduler.h>

#include <hatn/mq/mqerror.h>
#include <hatn/mq/message.h>

HATN_MQ_NAMESPACE_BEGIN

namespace client {

HDU_UNIT(producer_config,
    HDU_FIELD(dequeue_retry_interval,TYPE_UINT32,1,false,15)
    HDU_FIELD(message_ttl,TYPE_UINT32,2,false,900)
)

HDU_UNIT_WITH(client_db_message,(HDU_BASE(db::object),HDU_BASE(message)),
              HDU_FIELD(expire_at,TYPE_DATETIME,8)
              HDU_FIELD(expired,TYPE_BOOL,9)
              )

HATN_DB_INDEX(msgOidTypeIdx,message::object_id,message::object_type,message::producer,message::producer_pos)
HATN_DB_INDEX(msgOidOpIdx,message::object_id,message::operation,message::producer,message::producer_pos)
HATN_DB_INDEX(msgObjTypeIdx,message::object_type,message::producer,message::producer_pos)

// expire_at is used by app logic, the messages are not auto deleted when expired
HATN_DB_INDEX(msgExpireAtIdx,client_db_message::expire_at,message::producer,message::producer_pos)
HATN_DB_INDEX(msgExpiredIdx,client_db_message::expired,message::producer,message::producer_pos)
HATN_DB_UNIQUE_INDEX(msgProducerPosIdx,message::producer_pos,client_db_message::expired,message::producer,message::object_type)

HATN_DB_MODEL_WITH_CFG(clientMqMessageModel,client_db_message,db::ModelConfig("mq_messages"),
                       msgProducerPosIdx(),
                       msgOidOpIdx(),
                       msgOidTypeIdx(),
                       msgExpiredIdx(),
                       msgExpireAtIdx(),
                       msgProducerPosIdx(),
                       msgObjTypeIdx()
                       )

enum class MessageStatus : uint8_t
{
    Sent,
    Expired
};

template <typename ServerT, typename SchedulerT, typename NotifierT, typename MessageT=client_db_message::managed>
struct Traits
{
    using Server=ServerT;
    using Scheduler=SchedulerT;
    using Notifier=NotifierT;
    using Message=MessageT;
};

template <typename Traits>
class ProducerClient : public common::pmr::WithFactory,
                       public common::WithMappedThreads,
                       public HATN_BASE_NAMESPACE::ConfigObject<producer_config::type>,
                       public db::WithAsyncClient,
                       public common::TaskSubcontext
{
    public:

        using Server=typename Traits::Server;
        using Scheduler=typename Traits::Scheduler;
        using Notifier=typename Traits::Notifier;
        using Message=typename Traits::Message;

        constexpr static const char* SchedulerJobRefType="mq_producer_dequeue";

        ProducerClient(
            std::shared_ptr<db::AsyncClient> dbClient={},
                const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
            ) : common::pmr::WithFactory(factory),
                db::WithAsyncClient(std::move(dbClient)),
                m_stopped(false),
                m_scheduler(nullptr),
                m_topicJobs(this->factory()->template objectAllocator<topicJobsValueT>())
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
            m_producerIdStr=id;
        }

        const du::ObjectId& producerId() const noexcept
        {
            return m_producerId;
        }

        const auto& producerIdStr() const noexcept
        {
            return m_producerIdStr;
        }

        void setScheduler(Scheduler* scheduler) noexcept
        {
            m_scheduler=scheduler;
        }

        Scheduler* scheduler() const noexcept
        {
            return m_scheduler;
        }

        void setNotifier(Notifier* notifier) noexcept
        {
            m_notifier=notifier;
        }

        Notifier* notifier() const noexcept
        {
            return m_notifier;
        }

        void setServer(Server* server) noexcept
        {
            m_server=server;
        }

        Server* server() const noexcept
        {
            return m_server;
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
            if (ttl==0)
            {
                ttl=this->config().fieldValue(producer_config::message_ttl);
            }
            if (ttl!=0)
            {
                auto dt=common::DateTime::currentUtc();
                dt.addSeconds(ttl);
                msg->field(client_db_message::expire_at).set(dt);
            }
            msg->field(client_db_message::expired).set(false);

            // save in db
            auto txCb=[ctx,selfCtx{this->sharedMainCtx()},this,cb{std::move(cb)},msg,topic](auto, const Error& ec)
            {
                if (ec)
                {
                    //! to Log error
                }

                // wakeup dequeueing
                if (!ec)
                {
                    postSchedulerJob(ctx,[](auto,auto){},topic);
                }

                // callback with pos and error status
                cb(std::move(ctx),ec,msg->fieldValue(message::pos));
            };
            auto txHandler=[this,topic{std::move(topic)},msg,ctx,objectContent{std::move(objectContent)},notificationContent{std::move(notificationContent)}](db::Transaction* tx)
            {
                auto client=m_db->client();
                auto objectQuery=db::makeQuery(msgOidOpIdx(),db::where(message::object_id,db::query::eq,msg->fieldValue(message::object_id).and_(message::producer,db::query::eq,m_producerId)),topic);
                auto findCreateOpQuery=db::makeQuery(msgOidOpIdx(),db::where(message::object_id,db::query::eq,msg->fieldValue(message::object_id)).
                                                                        and_(message::operation,db::query::eq,Operation::Create).
                                                                        and_(message::producer,db::query::eq,m_producerId)
                                                       ,topic);

                switch (msg->fieldValue(message::operation))
                {
                    case (Operation::Create):
                    {
                        // check if object ID is unique
                        auto r=client->findOne(clientMqMessageModel(),objectQuery);

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
                        auto ec=client->deleteMany(clientMqMessageModel(),objectQuery,tx);
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
                                auto msg=obj.as<client_db_message::managed>();

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
                                ec=client->update(topic,clientMqMessageModel(),msg->fieldValue(db::object::_id),req,tx);
                                if (ec)
                                {
                                    //! @todo report error
                                }

                                // only one message should be processed
                                return false;
                            }
                        };

                        // check if Create message for that object ID exists
                        auto ec=client->findCb(clientMqMessageModel(),findCreateOpQuery,std::move(findCb),tx,true);
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
                auto ec=client->create(topic,clientMqMessageModel(),msg.get(),tx);
                if (ec)
                {
                    //! @todo Log error
                }
                return ec;
            };
            this->dbClient()->transaction(ctx,std::move(txHandler),std::move(txCb));
        }

        template <typename ContextT, typename CallbackT>
        void post(
            common::SharedPtr<ContextT> ctx,
            CallbackT cb,
            std::string topic,
            Operation op,
            lib::string_view objectId,
            lib::string_view objectType,
            uint32_t ttl
            )
        {
            post(std::move(ctx),std::move(cb),std::move(topic),op,objectId,objectType,{},{},ttl);
        }

        template <typename ContextT, typename CallbackT, typename ContentT=du::Unit>
        void post(
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
            post(std::move(ctx),std::move(cb),std::move(topic),op,objectId,objectType,std::move(objectContent),{},ttl);
        }

        template <typename ContextT, typename CallbackT>
        void invokeScheduledJob(
                common::SharedPtr<ContextT> ctx,
                common::SharedPtr<typename Scheduler::Job> jb,
                CallbackT cb
            )
        {
            common::postAsyncTask(
                this->threads()->mappedOrRandomThread(jb->fieldValue(HATN_SCHEDULER_NAMESPACE::job::ref_topic)),
                ctx,
                [ctx,this,selfCtx{sharedMainCtx()},jb](auto, auto cb)
                {
                    auto retryInterval=this->config()->fieldValue(producer_config::dequeue_retry_interval);
                    auto retryLater=[jb{std::move(jb)},cb{std::move(cb)},retryInterval](auto ctx)
                    {
                        jb->setFieldValue(HATN_SCHEDULER_NAMESPACE::job::period,retryInterval);
                        cb(std::move(ctx),Error{},std::move(jb),HATN_SCHEDULER_NAMESPACE::JobResultOp::RetryLater);
                    };

                    // if job for that topic is already under process then return immediately with retry later
                    if (m_stopped.load() || hasTopicJob(jb))
                    {
                        retryLater(std::move(ctx));
                        return;
                    }

                    // dequeue for given topic
                    dequeue(std::move(ctx),
                            [retryLater{std::move(retryLater)}](auto ctx)
                            {
                                retryLater(std::move(ctx));
                            },
                            std::move(jb)
                    );
                },
                std::move(cb)
            );
        }

        template <typename ContextT, typename CallbackT>
        void start(common::SharedPtr<ContextT> ctx, CallbackT cb)
        {
            m_stopped.store(false);
            wakeUpTopics(std::move(ctx),std::move(cb));
        }

        void stop()
        {
            m_stopped.store(true);

            m_topicJobsMutex.lock();
            m_topicJobs.clear();
            m_topicJobsMutex.unlock();
        }

        template <typename ContextT, typename CallbackT>
        void removeLocalExpired(common::SharedPtr<ContextT> ctx, CallbackT cb, std::string topic, std::string objectType)
        {
            lib::string_view topicView{topic};
            auto q=db::wrapQueryBuilder(
                [topic{std::move(topic)},objectType{std::move(objectType)},this,selfCtx{this->sharedMainCtx()}]()
                {
                    return db::makeQuery(msgExpiredIdx(),db::where(client_db_message::expired,db::query::eq,true).
                                                              and_(message::object_type,db::query::eq,objectType).
                                                              and_(message::producer,db::query::eq,m_producerId)
                                                    ,
                                                    topic
                                         );
                }
            );

            auto deleteCb=[ctx,selfCtx{this->sharedMainCtx()},cb{std::move(cb)}](auto, const Error& ec)
            {
                if (ec)
                {
                    //! @todo Log error
                }
                cb(std::move(ctx),ec);
            };
            m_db->deleteMany(ctx,std::move(deleteCb),clientMqMessageModel(),std::move(q),nullptr,topicView);
        }

        template <typename ContextT, typename CallbackT>
        void removeLocalPos(common::SharedPtr<ContextT> ctx, CallbackT cb, std::string topic, std::string objectType, const du::ObjectId& pos)
        {
            lib::string_view topicView{topic};
            auto q=db::wrapQueryBuilder(
                [topic{std::move(topic)},objectType{std::move(objectType)},this,selfCtx{this->sharedMainCtx()},pos]()
                {
                    return db::makeQuery(messagePosIdx(),db::where(message::pos,db::query::eq,pos).
                                                              and_(message::object_type,db::query::eq,objectType).
                                                              and_(message::producer,db::query::eq,m_producerId)
                                         ,
                                         topic
                                         );
                }
                );

            auto deleteCb=[ctx,selfCtx{this->sharedMainCtx()},cb{std::move(cb)}](auto, const Error& ec)
            {
                if (ec)
                {
                    //! @todo Log error
                }
                cb(std::move(ctx),ec);
            };
            m_db->deleteMany(ctx,std::move(deleteCb),clientMqMessageModel(),std::move(q),nullptr,topicView);
        }

        template <typename ContextT, typename CallbackT>
        void removeLocal(common::SharedPtr<ContextT> ctx, CallbackT cb, std::string topic, std::string objectType, common::pmr::vector<du::ObjectId> objIds={})
        {
            lib::string_view topicView{topic};
            auto q=db::wrapQueryBuilder(
                [topic{std::move(topic)},objectType{std::move(objectType)},this,selfCtx{this->sharedMainCtx()},objIds{std::move(objIds)}]()
                {
                    if (objIds.empty())
                    {
                        return db::makeQuery(objectTypeIdx(),db::where(message::object_type,db::query::eq,objectType).
                                                                       and_(message::producer,db::query::eq,m_producerId),
                                             topic
                                             );
                    }
                    return db::makeQuery(msgOidTypeIdx(),db::where(message::object_id,db::query::in,objIds).
                                                          and_(message::object_type,db::query::eq,objectType).
                                                          and_(message::producer,db::query::eq,m_producerId),
                                         topic
                                         );
                }
                );

            auto deleteCb=[ctx,selfCtx{this->sharedMainCtx()},cb{std::move(cb)}](auto, const Error& ec)
            {
                if (ec)
                {
                    //! @todo Log error
                }
                cb(std::move(ctx),ec);
            };
            m_db->deleteMany(ctx,std::move(deleteCb),clientMqMessageModel(),std::move(q),nullptr,topicView);
        }

        template <typename ContextT, typename CallbackT>
        void readLocal(common::SharedPtr<ContextT> ctx, CallbackT cb, std::string topic, std::string objectType, common::pmr::vector<du::ObjectId> objIds={})
        {
            lib::string_view topicView{topic};
            auto q=db::wrapQueryBuilder(
                [topic{std::move(topic)},objectType{std::move(objectType)},this,selfCtx{this->sharedMainCtx()},objIds{std::move(objIds)}]()
                {
                    if (objIds.empty())
                    {
                        return db::makeQuery(objectTypeIdx(),db::where(message::object_type,db::query::eq,objectType).
                                             and_(message::producer,db::query::eq,m_producerId),
                                             topic
                                             );
                    }
                    return db::makeQuery(msgOidTypeIdx(),db::where(message::object_id,db::query::in,objIds).
                                                          and_(message::object_type,db::query::eq,objectType).
                                                          and_(message::producer,db::query::eq,m_producerId),
                                         topic
                                         );
                }
                );

            auto findCb=[ctx,selfCtx{this->sharedMainCtx()},cb{std::move(cb)}](auto, Result<common::pmr::vector<db::DbObject>> r)
            {
                if (r)
                {
                    //! @todo Log error
                }
                cb(std::move(ctx),std::move(r));
            };
            m_db->find(ctx,std::move(findCb),clientMqMessageModel(),std::move(q),topicView);
        }

        bool hasTopicJob(lib::string_view topic) const
        {
            common::MutexScopedLock l(m_topicJobsMutex);

            auto it=m_topicJobs.find(topic);
            return it!=m_topicJobs.end();
        }

        bool hasTopicJob(const common::SharedPtr<typename Scheduler::Job>& jb) const
        {
            return hasTopicJob(jb->field(HATN_SCHEDULER_NAMESPACE::job::ref_topic).value());
        }

    private:

        template <typename ContextT, typename CallbackT>
        void postSchedulerJob(
                common::SharedPtr<ContextT> ctx,
                CallbackT callback,
                std::string topic
            )
        {
            m_scheduler->postJob(std::move(ctx),std::move(callback),m_producerIdStr,topic,SchedulerJobRefType,HATN_SCHEDULER_NAMESPACE::Mode::Queued);
        }

        template <typename ContextT, typename CallbackT, typename Iterator>
        void checkTopicOwned(common::SharedPtr<ContextT> ctx, CallbackT cb,
                             std::shared_ptr<std::pmr::set<db::TopicHolder>> foundTopics,
                             Iterator it)
        {
            if (it==foundTopics->end() || m_stopped.load())
            {
                cb(ctx,Error{});
                return;
            }

            auto q=db::wrapQuery(this->factory(),msgProducerPosIdx(),db::where(message::producer,db::query::eq,m_producerId).
                                                                          and_(client_db_message::expired,db::query::eq,false)
                                   ,*it);
            auto findCb=[ctx,cb{std::move(cb)},selfCtx{common::toWeakPtr(this->sharedMainCtx())},this,
                           foundTopics{std::move(foundTopics)},it](auto, Result<db::DbObjectT<Message>> res)
            {
                if (res)
                {
                    //! @todo Log error
                    cb(ctx,res.takeError());
                    return;
                }

                if (m_stopped.load())
                {
                    cb(ctx,Error{});
                    return;
                }

                auto sCtx=selfCtx.lock();
                if (!sCtx)
                {
                    return;
                }

                if (!res->isNull())
                {
                    postSchedulerJob(ctx,[](auto, auto){},*it);
                }

                auto it1=it;
                ++it1;
                checkTopicOwned(std::move(ctx),std::move(cb),std::move(foundTopics),it1);
            };
            m_db->findOne(ctx,findCb,clientMqMessageModel(),std::move(q),*it);
        }

        template <typename ContextT, typename CallbackT>
        void wakeUpTopics(common::SharedPtr<ContextT> ctx, CallbackT cb)
        {
            auto topicsCb=[ctx,cb{std::move(cb)},this,selfCtx{this->sharedMainCtx()}](auto, Result<std::pmr::set<db::TopicHolder>> res)
            {
                if (res)
                {
                    //! @todo Log error
                    cb(ctx,res.takeError());
                    return;
                }

                if (m_stopped.load())
                {
                    cb(ctx,Error{});
                    return;
                }

                auto topics=std::make_shared<std::pmr::set<db::TopicHolder>>(res.takeValue());
                auto it=topics->begin();
                checkTopicOwned(std::move(ctx),std::move(cb),std::move(topics),it);
            };

            m_db->listModelTopics(
                ctx,
                topicsCb,
                clientMqMessageModel()
            );
        }

        void removeTopicJob(lib::string_view topic)
        {
            common::MutexScopedLock l(m_topicJobsMutex);

            m_topicJobs.erase(topic);
        }

        void removeTopicJob(const common::SharedPtr<typename Scheduler::Job>& jb)
        {
            removeTopicJob(jb->field(HATN_SCHEDULER_NAMESPACE::job::ref_topic).value());
        }

        template <typename ContextT, typename CallbackT>
        void sendToserver(common::SharedPtr<ContextT> ctx, CallbackT cb, common::SharedPtr<typename Scheduler::Job> jb, common::SharedPtr<Message> msg)
        {
            auto sendCb=[cb{std::move(cb)},jb{std::move(jb)},selfCtx{this->sharedMainCtx()},this](auto ctx, const Error& ec, auto msg)
            {
                if (ec)
                {
                    cb(std::move(ctx),ec,std::move(jb),HATN_SCHEDULER_NAMESPACE::JobResultOp::RetryLater);
                    return;
                }

                // remove message
                auto delCb=[ctx{std::move(ctx)},this,selfCtx{this->sharedMainCtx()},jb,cb{std::move(cb)},msg](auto, common::Error& ec)
                {
                    if (ec)
                    {
                        cb(std::move(ctx),ec,std::move(jb),HATN_SCHEDULER_NAMESPACE::JobResultOp::RetryLater);
                        return;
                    }

                    auto notifyCb=[ctx,this,selfCtx{this->sharedMainCtx()},jb,cb{std::move(cb)}](auto, common::Error&)
                    {
                        // dequeue next message
                        dequeue(std::move(ctx),std::move(cb),std::move(jb));
                    };
                    // notify that message was sent
                    m_notifier->notify(
                        ctx,notifyCb,jb->fieldValue(HATN_SCHEDULER_NAMESPACE::job::ref_topic),msg,MessageStatus::Sent
                    );
                };
                m_db->deleteObject(ctx,delCb,jb->fieldValue(HATN_SCHEDULER_NAMESPACE::job::ref_topic),clientMqMessageModel(),msg->fieldValue(db::object::_id));
            };

            m_server->send(std::move(ctx),std::move(msg),std::move(sendCb));
        }

        template <typename ContextT, typename CallbackT>
        void dequeue(common::SharedPtr<ContextT> ctx, CallbackT cb, common::SharedPtr<typename Scheduler::Job> jb)
        {
            if (m_stopped.load())
            {
                cb(std::move(ctx),Error{},std::move(jb),HATN_SCHEDULER_NAMESPACE::JobResultOp::RetryLater);
                return;
            }

            auto jobResultStatus=this->factory()->template createObject<
                std::pair<bool,common::SharedPtr<HATN_SCHEDULER_NAMESPACE::JobResultOp>>
            >(true,HATN_SCHEDULER_NAMESPACE::JobResultOp::RetryLater);

            auto txCb=[ctx,cb,this,selfCtx{this->sharedMainCtx()},jb,jobResultStatus](auto, const common::Error ec)
            {
                if (ec)
                {
                    //! @todo report error
                }
                if (jobResultStatus->first)
                {
                    cb(std::move(ctx),ec,std::move(jb),*jobResultStatus);
                }
            };
            auto txHandler=[this,ctx,cb,jb,jobResultStatus](db::Transaction* tx)
            {
                if (m_stopped)
                {
                    return Error{};
                }

                auto client=m_db->client();

                size_t foundCount=0;
                bool findCb=[ctx,jb,cb,this,jobResultStatus,&foundCount,tx](db::DbObject obj, Error& ec)
                {
                    if (ec)
                    {
                        //! @todo report error
                        return false;
                    }

                    // message not found
                    if (obj.isNull())
                    {
                        return false;
                    }

                    auto msg=db::DbObjectT<Message>{std::move(obj)};

                    // check if message expired
                    if (msg->fieldValue(client_db_message::expire_at)>common::DateTime::currentUtc())
                    {
                        // update object - set expired=true
                        auto req=db::update::request(db::update::field(client_db_message::expired,db::update::Operator::set,true));
                        ec=client->update(jb->fieldValue(HATN_SCHEDULER_NAMESPACE::job::ref_topic),
                                            clientMqMessageModel(),
                                            msg->fieldValue(db::object::_id),
                                            req,
                                            tx
                                            );
                        if (ec)
                        {
                            //! @todo log error
                            return false;
                        }

                        // notify that message expired
                        auto notifyCb=[](auto, common::Error&){};
                        m_notifier->notify(
                            ctx,notifyCb,jb->fieldValue(HATN_SCHEDULER_NAMESPACE::job::ref_topic),msg,MessageStatus::Expired
                        );

                        // read next message
                        return true;
                    }
                    foundCount++;

                    // send message to server
                    jobResultStatus->first=false;
                    sendToserver(std::move(ctx),std::move(cb),std::move(jb),std::move(msg));

                    // wait for result of message sending
                    return false;
                };

                // find non expired messages sorted by producer_pos
                auto q=db::makeQuery(msgProducerPosIdx(),db::where(message::producer_pos,db::query::gte,db::query::First,db::query::Order::Asc).
                                                                          and_(client_db_message::expired,db::query::eq,false).
                                                                          and_(message::producer,db::query::eq,m_producerId),
                                       jb->fieldValue(HATN_SCHEDULER_NAMESPACE::job::ref_topic)
                                    );
                auto ec=client->findCb(clientMqMessageModel(),q,findCb,tx,true);
                if (ec)
                {
                    //! @todo report error
                }
                else if (foundCount==0)
                {
                    // all messages were prosecced, remove the job
                    jobResultStatus->second=HATN_SCHEDULER_NAMESPACE::JobResultOp::Remove;
                    removeTopicJob(jb);
                }
            };
            m_db->transaction(ctx,std::move(txCb),std::move(txHandler),jb->fieldValue(HATN_SCHEDULER_NAMESPACE::job::ref_topic));
        }

        du::ObjectId m_producerId;
        du::ObjectId::String m_producerIdStr;

        db::AsyncClient* m_db;

        std::atomic<bool> m_stopped;

        Scheduler* m_scheduler;
        Server* m_server;
        Notifier* m_notifier;

        mutable common::MutexLock m_topicJobsMutex;
        common::pmr::map<std::string,common::SharedPtr<typename Scheduler::Job>,std::less<>> m_topicJobs;
        using topicJobsValueT=typename common::pmr::map<std::string,common::SharedPtr<typename Scheduler::Job>,std::less<>>::value_type;
};

} // namespace client

HATN_MQ_NAMESPACE_END

#endif // HATNBMQPRODUCERCLIENT_H
