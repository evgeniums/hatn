/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file mq/scheduler.h
  *
  *
  */

/****************************************************************************/

#ifndef HATNSCHEDULER_H
#define HATNSCHEDULER_H

#include <hatn/common/error.h>
#include <hatn/common/thread.h>
#include <hatn/common/sharedptr.h>
#include <hatn/common/taskcontext.h>
#include <hatn/common/asiotimer.h>
#include <hatn/common/cachelru.h>
#include <hatn/common/locker.h>

#include <hatn/base/configobject.h>

#include <hatn/dataunit/syntax.h>

#include <hatn/db/object.h>
#include <hatn/db/model.h>
#include <hatn/db/indexquery.h>
#include <hatn/db/asyncclient.h>

#include <hatn/mq/mq.h>
#include <hatn/mq/backgroundworker.h>

#define HATN_SCHEDULER_NAMESPACE_BEGIN namespace hatn { namespace mq { namespace scheduler {
#define HATN_SCHEDULER_NAMESPACE_END }}}

#define HATN_SCHEDULER_NAMESPACE hatn::mq::scheduler
#define HATN_SCHEDULER_NS scheduler
#define HATN_SCHEDULER_USING using namespace hatn::mq::scheduler;

HATN_SCHEDULER_NAMESPACE_BEGIN

enum class Mode : uint8_t
{
    Schedule,
    Direct,
    Queued
};

enum class JobConflictMode : uint8_t
{
    SkipNewJob,
    Replace,
    UpdateTime
};

HDU_UNIT(config,
    HDU_FIELD(job_bucket_size,TYPE_UINT32,1,false,32)
    HDU_FIELD(job_retry_interval,TYPE_UINT32,2,false,300)
    HDU_FIELD(job_hold_period,TYPE_UINT32,3,false,900)
)

HDU_UNIT_WITH(job,(HDU_BASE(db::object)),
    HDU_FIELD(ref_id,TYPE_UINT32,1,true)
    HDU_FIELD(ref_topic,TYPE_STRING,2)
    HDU_FIELD(ref_type,TYPE_STRING,3)
    HDU_FIELD(next_time,TYPE_DATETIME,4)
    HDU_FIELD(content,TYPE_DATAUNIT,5)
    HDU_FIELD(period,TYPE_UINT32,6)
)

HATN_DB_INDEX(jobTimeIdx,job::next_time)
HATN_DB_UNIQUE_INDEX(jobRefIdx,job::ref_id,job::ref_topic,job::ref_type)
HATN_DB_INDEX(jobRefTypeIdx,job::ref_type)

HATN_DB_MODEL_WITH_CFG(jobModel,job,HATN_DB_NAMESPACE::ModelConfig("scheduler_jobs"),jobTimeIdx(),jobRefIdx(),jobRefTypeIdx())

struct SchedulerTraits
{
    using Job=job::managed;    
    using Worker=hana::false_;

    static const auto& jobModel()
    {
        return HATN_SCHEDULER_NAMESPACE::jobModel();
    }
};

template <typename WorkerT>
class WorkerPool
{
    using Worker=WorkerT;
    common::CacheLru<common::TaskContextId,common::SharedPtr<Worker>> m_workers;
};

struct JobKey
{
    JobKey(
        lib::string_view refId={},
        lib::string_view refTopic={},
        lib::string_view refType={},
        const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
        ) : refId(refId,factory->dataAllocator<common::pmr::string>()),
            refTopic(refTopic,factory->dataAllocator<common::pmr::string>()),
            refType(refType,factory->dataAllocator<common::pmr::string>())
    {}

    std::pmr::string refId;
    std::pmr::string refTopic;
    std::pmr::string refType;
};

template <typename Traits=SchedulerTraits>
class Scheduler : public HATN_BASE_NAMESPACE::ConfigObject<config::type>,
                  public common::TaskSubcontext
{
    public:

        using Job=typename Traits::Job;
        using Worker=typename Traits::Worker;
        using WorkContextBuilder=typename Traits::WorkContextBuilder;
        using BackgroundWorker=HATN_MQ_NAMESPACE::BackgroundWorker<Scheduler<Traits>,WorkContextBuilder>;

    private:

        static const auto& jobModel()
        {
            return Traits::jobModel();
        }

    public:

        Scheduler(
                std::shared_ptr<db::AsyncClient> db,
                common::TaskWithContextThread* thread=common::TaskWithContextThread::current(),
                const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
            ) : m_db(std::move(db)),
                m_thread(thread),
                m_factory(factory),
                m_timer(thread),
                m_jobRunner(nullptr),
                m_background(nullptr)
        {
            m_queue.setCapacity(0);
        }

        void init(BackgroundWorker* backgroundWorker, Worker* jobRunner)
        {
            m_background=backgroundWorker;
            m_jobRunner=jobRunner;
            m_background->setWorker(this);            
        }

        void setDefaultTopic(std::string val)
        {
            m_topic=std::move(val);
        }

        const std::string& defaultTopic() const noexcept
        {
            return m_topic;
        }

        template <typename ContextT, typename CallbackT>
        void enqueueOrPostJob(
                common::SharedPtr<ContextT> ctx,
                CallbackT callback,
                const du::ObjectId& refId,
                lib::string_view refTopic,
                lib::string_view refType={},
                int period=0
            )
        {
            auto q=db::allocateQuery(m_factory,jobRefIdx(),
                                       db::query::where(job::ref_id,db::query::eq,refId).
                                       and_(job::ref_topic,db::query::eq,refTopic).
                                       and_(job::ref_type,db::query::eq,refType),
                                       m_topic
                                       );

            auto readCb=[selfCtx{sharedMainCtx()},this,ctx,callback{std::move(callback)},period,refId,refType,refTopic](auto, auto r)
            {
                if (r)
                {
                    //! @todo log
                    callback(std::move(ctx),r.takeError());
                    return;
                }

                if (r->isNull())
                {
                    auto newJob=m_factory->createObject<Job>();
                    newJob->setFieldValue(job::period,period);
                    newJob->setFieldValue(job::ref_id,refId);
                    newJob->setFieldValue(job::ref_type,refType);
                    newJob->setFieldValue(job::ref_topic,refTopic);
                    postJob(std::move(ctx),std::move(callback),std::move(newJob));
                    return;
                }

                enqueueJob(std::move(r->shared()));
                callback(std::move(ctx),Error{});
            };
            m_db->findOne(
                ctx,
                std::move(readCb),
                jobModel(),
                std::move(q),
                m_topic
            );
        }

        void enqueueJob(common::SharedPtr<Job> newJob)
        {
            common::MutexScopedLock l{m_cacheMutex};
            if (!m_queue.hasItem(*newJob))
            {
                //! @todo optimization: too many string allocations in job key
                m_queue.emplaceItem(JobKey{newJob->fieldValue(job::ref_id),
                                           newJob->fieldValue(job::ref_type),
                                           newJob->fieldValue(job::ref_type),
                                           m_factory
                                    },
                                    std::move(newJob));
            }
        }

        template <typename ContextT, typename CallbackT>
        void postJob(
                common::SharedPtr<ContextT> ctx,
                CallbackT callback,
                common::SharedPtr<Job> newJob,
                Mode mode=Mode::Queued,
                JobConflictMode conflictMode=JobConflictMode::SkipNewJob
            )
        {
            // prepare job
            db::initObject(*newJob);
            if (!newJob->field(job::next_time).isSet())
            {
                auto dt=common::DateTime::current();
                if (newJob->field(job::period).isSet())
                {
                    dt.addSeconds(newJob->fieldValue(job::period));
                }
                else
                {
                    dt.addSeconds(this->config().fieldValue(config::job_retry_interval));
                }
                newJob->setFieldValue(job::next_time,dt);
            }

            // in direct mode invoke worker directly without saving job in db
            if (mode==Mode::Direct)
            {
                m_jobRunner->invoke(
                    std::move(ctx),
                    std::move(newJob),
                    std::move(callback)
                );
                return;
            }

            // job posting callback
            auto jobPtr=newJob.get();
            auto cb=[ctx,selfCtx{sharedMainCtx()},this,callback{callback},mode,newJob{std::move(newJob)}](const Error& ec)
            {
                if (!ec)
                {
                    if (mode==Mode::Queued)
                    {
                        enqueueJob(std::move(newJob));
                    }
                    wakeUp();
                }

                //! @todo log
                callback(std::move(ctx),ec);
            };

            // job creation callback
            auto createCb=[this,jobPtr,cb{std::move(cb)},conflictMode](auto ctx, const Error& ec)
            {
                if (ec && ec.is(db::DbError::DUPLICATE_UNIQUE_KEY,db::DbErrorCategory::getCategory()) && conflictMode!=JobConflictMode::SkipNewJob)
                {
                    //! @todo Log it

                    // conflicting job by ref_id

                    auto q=db::allocateQuery(m_factory,jobRefIdx(),
                                           db::query::where(job::ref_id,db::query::eq,jobPtr->fieldValue(job::ref_id)).
                                                        and_(job::ref_topic,db::query::eq,jobPtr->fieldValue(job::ref_topic)).
                                                        and_(job::ref_type,db::query::eq,jobPtr->fieldValue(job::ref_type)),
                                           m_topic
                                        );
                    if (conflictMode==JobConflictMode::UpdateTime)
                    {
                        // conflictMode==JobConflictMode::UpdateTime

                        auto req=db::update::allocateRequest(m_factory,db::update::field(job::next_time,db::update::set,jobPtr->fieldValue(job::next_time)));
                        auto updateCb=[this,cb{std::move(cb)}](auto, const Error& ec)
                        {
                            //! @todo Log it
                            cb(ec);
                        };
                        m_db->updateMany(
                            ctx,
                            std::move(updateCb),
                            jobModel(),
                            std::move(q),
                            std::move(req),
                            jobPtr,
                            nullptr,
                            m_topic
                        );
                    }
                    else
                    {
                        // conflictMode==JobConflictMode::Replace

                        m_cacheMutex.lock();
                        m_queue.removeItem(*jobPtr);
                        m_cacheMutex.unlock();

                        auto transactionCb=[this,cb{std::move(cb)}](auto, const Error& ec)
                        {
                            //! @todo Log it
                            cb(ec);
                        };
                        auto replaceTx=[this,jobPtr,q{std::move(q)}](db::Transaction* tx)
                        {
                            //! @todo critical: Implement async transaction

                            auto client=m_db->client();

                            //! @todo Log it
                            auto ec=client->deleteMany(jobModel(),*q,tx);
                            HATN_CHECK_EC(ec)
                            ec=client->create(m_topic,jobModel(),jobPtr.get(),tx);
                            return ec;
                        };
                        m_db->transaction(ctx,std::move(transactionCb),std::move(replaceTx),m_topic);
                    }

                    return;
                }

                cb(ec);
            };

            // optimistically try to save a new job in db
            m_db->create(
                ctx,
                std::move(createCb),
                m_topic,
                jobModel(),
                jobPtr
            );
        }

        template <typename ContextT, typename CallbackT>
        void removeJob(
                common::SharedPtr<ContextT> ctx,
                CallbackT callback,
                lib::string_view refId,
                lib::string_view refTopic={},
                lib::string_view refType={}
            )
        {
            {
                common::MutexScopedLock l{m_cacheMutex};
                m_queue.removeItem(JobKey{refId,refTopic,refType,m_factory});
            }

            auto q=db::allocateQuery(m_factory,jobRefIdx(),
                                       db::query::where(job::ref_id,db::query::eq,refId).
                                       and_(job::ref_topic,db::query::eq,refTopic).
                                       and_(job::ref_type,db::query::eq,refType),
                                       m_topic
                                       );
            auto removeCb=[selfCtx{sharedMainCtx()},this,ctx,callback{std::move(callback)}](auto, const Error& ec)
            {
                if (ec)
                {
                    //! @todo log
                }

                callback(std::move(ctx),ec);
            };
            m_db->deleteMany(
                ctx,
                std::move(removeCb),
                jobModel(),
                std::move(q),
                nullptr,
                m_topic
            );

        }

        void wakeUp()
        {
            m_background->wakeUp();
        }

        void start()
        {
            m_background->start();
        }

        void stop()
        {
            m_background->stop();
        }

        template <typename ContextT>
        void run(common::SharedPtr<ContextT> ctx)
        {
            dequeueJobs(std::move(ctx),true);
        }

    private:

        template <typename ContextT>
        void fetchJobs(common::SharedPtr<ContextT> ctx)
        {
            //! @todo read bucket of jobs from database

            //! @todo sleep if no jobs found

            //! @todo update next read time of each job

            //! @todo read updated jobs

            //! @todo post jobs to queue

            //! @todo invoke dequeueing
            // dequeueJobs(false);

            //! @todo sleep if queue is full

            //! @todo repeat
        }

        template <typename ContextT>
        void dequeueJobs(common::SharedPtr<ContextT> ctx, bool async)
        {
            //! @todo if queue is empty then invoke fetchJobs()

            //! @todo exec async

            //! @todo check in loop if worker can accept jobs
            //! @todo dequeue job and post it to worker
            //! @todo repeat until queue is not empty or bucket size is reached

            //! @todo if bucket size is reached then invoke dequeueJobs() again to break a dequeuing loop
            // dequeueJobs(true);

            //! @todo if queue is empty then invoke fetching jobs again
            m_background->wakeUp();
        }

        std::shared_ptr<db::AsyncClient> m_db;
        common::TaskWithContextThread* m_thread;
        const common::pmr::AllocatorFactory* m_factory;

        common::AsioDeadlineTimer m_timer;

        common::CacheLru<JobKey,common::SharedPtr<Job>> m_queue;
        common::MutexLock m_cacheMutex;

        Worker* m_jobRunner;
        BackgroundWorker* m_background;
        std::string m_topic;
};

HATN_SCHEDULER_NAMESPACE_END

namespace std {

template <>
struct std::less<HATN_SCHEDULER_NAMESPACE::JobKey>
{
    bool operator()(const HATN_SCHEDULER_NAMESPACE::JobKey& l, const HATN_SCHEDULER_NAMESPACE::JobKey& r) const noexcept
    {
        int comp=l.refId.compare(r.refId);
        if (comp<0)
        {
            return true;
        }
        if (comp>0)
        {
            return false;
        }

        comp=l.refTopic.compare(r.refTopic);
        if (comp<0)
        {
            return true;
        }
        if (comp>0)
        {
            return false;
        }

        comp=l.refType.compare(r.refType);
        if (comp<0)
        {
            return true;
        }
        if (comp>0)
        {
            return false;
        }

        return false;
    }

    template <typename T>
    bool operator()(const HATN_SCHEDULER_NAMESPACE::JobKey& l, const T& r) const noexcept
    {
        int comp=l.refId.compare(r.field(HATN_SCHEDULER_NAMESPACE::job::ref_id).value());
        if (comp<0)
        {
            return true;
        }
        if (comp>0)
        {
            return false;
        }

        comp=l.refTopic.compare(r.field(HATN_SCHEDULER_NAMESPACE::job::ref_topic).value());
        if (comp<0)
        {
            return true;
        }
        if (comp>0)
        {
            return false;
        }

        comp=l.refType.compare(r.field(HATN_SCHEDULER_NAMESPACE::job::ref_type).value());
        if (comp<0)
        {
            return true;
        }
        if (comp>0)
        {
            return false;
        }

        return false;
    }

    template <typename T>
    bool operator()(const T& l, const HATN_SCHEDULER_NAMESPACE::JobKey& r) const noexcept
    {
        return !std::less<HATN_SCHEDULER_NAMESPACE::JobKey>{}(r,l);
    }
};

}

#endif // HATNSCHEDULER_H
