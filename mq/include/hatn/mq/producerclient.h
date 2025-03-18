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
              HDU_FIELD(failed,TYPE_BOOL,9)
              HDU_FIELD(error_message,TYPE_STRING,10)
              )

HATN_DB_INDEX(msgOidTypeIdx,message::object_id,message::object_type,message::producer,message::producer_pos)
HATN_DB_INDEX(msgOidOpIdx,message::object_id,message::operation,message::producer,message::producer_pos)
HATN_DB_INDEX(msgObjTypeIdx,message::object_type,message::producer,message::producer_pos)

// expire_at is used by app logic, the messages are not auto deleted when expired or failed
HATN_DB_INDEX(msgExpireAtIdx,client_db_message::expire_at,message::producer,message::producer_pos)
HATN_DB_INDEX(msgFailedIdx,client_db_message::failed,message::producer,message::producer_pos)
HATN_DB_UNIQUE_INDEX(msgProducerPosIdx,message::producer_pos,client_db_message::failed,message::producer,message::object_type)

HATN_DB_MODEL_WITH_CFG(clientMqMessageModel,client_db_message,db::ModelConfig("mq_messages"),
                       msgProducerPosIdx(),
                       msgOidOpIdx(),
                       msgOidTypeIdx(),
                       msgFailedIdx(),
                       msgExpireAtIdx(),
                       msgProducerPosIdx(),
                       msgObjTypeIdx()
                       )

enum class MessageStatus : uint8_t
{
    Sent,
    Failed
};

template <typename ApiClientT, typename SchedulerT, typename NotifierT, typename MessageT=client_db_message::managed>
struct Traits
{
    using ApiClient=ApiClientT;
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

        using ApiClient=typename Traits::ApiClient;
        using Scheduler=typename Traits::Scheduler;
        using Notifier=typename Traits::Notifier;
        using Message=typename Traits::Message;

        constexpr static const char* SchedulerJobRefType="mq_producer_dequeue";

        ProducerClient(
            std::shared_ptr<db::AsyncClient> dbClient={},
            const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
        );

        ProducerClient(
            const common::ConstDataBuf& producerId,
            db::AsyncClient* dbClient=nullptr,
            const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
        );

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

        void setApiClient(ApiClient* server) noexcept
        {
            m_apiClient=server;
        }

        ApiClient* server() const noexcept
        {
            return m_apiClient;
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
        );

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
        );

        template <typename ContextT, typename CallbackT>
        void start(common::SharedPtr<ContextT> ctx, CallbackT cb);

        void stop();

        template <typename ContextT, typename CallbackT>
        void removeLocalFailed(common::SharedPtr<ContextT> ctx, CallbackT cb, std::string topic, std::string objectType);

        template <typename ContextT, typename CallbackT>
        void removeLocalPos(common::SharedPtr<ContextT> ctx, CallbackT cb, std::string topic, std::string objectType, const du::ObjectId& pos);

        template <typename ContextT, typename CallbackT>
        void removeLocal(common::SharedPtr<ContextT> ctx, CallbackT cb, std::string topic, std::string objectType, common::pmr::vector<du::ObjectId> objIds={});

        template <typename ContextT, typename CallbackT>
        void readLocal(common::SharedPtr<ContextT> ctx, CallbackT cb, std::string topic, std::string objectType, common::pmr::vector<du::ObjectId> objIds={});

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
            );

        template <typename ContextT, typename CallbackT, typename Iterator>
        void checkTopicOwned(common::SharedPtr<ContextT> ctx, CallbackT cb,
                             std::shared_ptr<std::pmr::set<db::TopicHolder>> foundTopics,
                             Iterator it);

        template <typename ContextT, typename CallbackT>
        void wakeUpTopics(common::SharedPtr<ContextT> ctx, CallbackT cb);

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
        void sendToserver(common::SharedPtr<ContextT> ctx, CallbackT cb, common::SharedPtr<typename Scheduler::Job> jb, common::SharedPtr<Message> msg);

        template <typename ContextT, typename CallbackT>
        void dequeue(common::SharedPtr<ContextT> ctx, CallbackT cb, common::SharedPtr<typename Scheduler::Job> jb);

        du::ObjectId m_producerId;
        du::ObjectId::String m_producerIdStr;

        db::AsyncClient* m_db;

        std::atomic<bool> m_stopped;

        Scheduler* m_scheduler;
        ApiClient* m_apiClient;
        Notifier* m_notifier;

        mutable common::MutexLock m_topicJobsMutex;
        common::pmr::map<std::string,common::SharedPtr<typename Scheduler::Job>,std::less<>> m_topicJobs;
        using topicJobsValueT=typename common::pmr::map<std::string,common::SharedPtr<typename Scheduler::Job>,std::less<>>::value_type;
};

} // namespace client

HATN_MQ_NAMESPACE_END

#endif // HATNBMQPRODUCERCLIENT_H
