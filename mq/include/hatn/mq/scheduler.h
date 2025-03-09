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

HDU_UNIT(scheduler_config,
    HDU_FIELD(job_bucket_size,TYPE_UINT32,1,false,64)
    HDU_FIELD(job_queue_depth,TYPE_UINT32,2,false,128)
    HDU_FIELD(job_retry_interval,TYPE_UINT32,3,false,300)
    HDU_FIELD(job_hold_period,TYPE_UINT32,4,false,300)
)

//! @todo use ObjectId for next_time to simplify jobs ordering when they are posted at the same time

HDU_UNIT_WITH(job,(HDU_BASE(db::object)),
    HDU_FIELD(ref_id,TYPE_STRING,1,true)
    HDU_FIELD(ref_topic,TYPE_STRING,2)
    HDU_FIELD(ref_type,TYPE_STRING,3)
    HDU_FIELD(next_time,TYPE_DATETIME,4)    
    HDU_FIELD(period,TYPE_UINT32,5)
    HDU_FIELD(check_id,TYPE_OBJECT_ID,6,true)
    HDU_FIELD(content,TYPE_DATAUNIT,7)
)

HATN_DB_INDEX(jobTimeIdx,job::next_time)
HATN_DB_UNIQUE_INDEX(jobRefIdx,job::ref_id,job::ref_topic,job::ref_type)
HATN_DB_INDEX(jobRefTypeIdx,job::ref_type)

HATN_DB_MODEL_WITH_CFG(jobModel,job,HATN_DB_NAMESPACE::ModelConfig("scheduler_jobs"),jobTimeIdx(),jobRefIdx(),jobRefTypeIdx())

struct SchedulerTraits
{
    using Job=job::managed;    
    using Worker=void; // implement worker and rebind type in derived traits
    using WorkContextBuilder=void; // implement builder and rebind type in derived traits

    static const auto& jobModel()
    {
        return HATN_SCHEDULER_NAMESPACE::jobModel();
    }
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

    template <typename T>
    JobKey (const T* jobPtr, const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault())
        : JobKey(
              jobPtr->fieldValue(job::ref_id),
              jobPtr->fieldValue(job::ref_type),
              jobPtr->fieldValue(job::ref_type),
              factory
            )
    {}

    std::pmr::string refId;
    std::pmr::string refTopic;
    std::pmr::string refType;
};

enum class JobResultOp : uint8_t
{
    Remove=1,
    UpdateNextTime=2,
    RetryLater=3
};

template <typename JobT>
void setJobNextTime(JobT* jobPtr, const common::DateTime& dt)
{
    jobPtr->setFieldValue(job::next_time,dt);
}

template <typename JobT>
void addToJobNextTime(JobT* jobPtr, int seconds)
{
    auto dt=jobPtr->fieldValue(job::next_time);
    dt.addSeconds(seconds);
    setJobNextTime(jobPtr,dt);
}

template <typename Traits=SchedulerTraits>
class Scheduler : public HATN_BASE_NAMESPACE::ConfigObject<scheduler_config::type>,
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
                const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
            ) : m_db(std::move(db)),
                m_factory(factory),
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

            JobKey key{newJob.get()};
            if (!m_queue.hasItem(key))
            {
                //! @todo optimization: too many string allocations in job key
                m_queue.emplaceItem(std::move(key),std::move(newJob));
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
                // if next time not explicitly set then set it
                auto existingItem=m_queue.hasItem(*newJob);
                if (existingItem==nullptr)
                {
                    // if no such job is in queue then schedule for now
                    setJobNextTime(newJob.get(),now());
                }
                else
                {
                    // if such job is already in queue then use next time of that job
                    setJobNextTime(newJob.get(),existingItem->job->fieldValue(job::next_time));
                }
            }
            newJob->field(job::check_id).mutableValue()->generate();

            // in direct mode invoke worker directly without saving job in db
            if (mode==Mode::Direct)
            {
                m_jobRunner->invoke(
                    std::move(ctx),
                    std::move(newJob),
                    [callback{std::move(callback)}](auto ctx, const Error& ec, auto doneJob, JobResultOp postCompleteOp)
                    {
                        callback(std::move(ctx),ec);
                        std::ignore=doneJob;
                        std::ignore=postCompleteOp;
                    }
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


            auto transactionCb=[this,cb{std::move(cb)}](auto, const Error& ec)
            {
                //! @todo Log it
                cb(ec);
            };
            auto txHandler=[this,jobPtr,conflictMode,newJob{std::move(newJob)}](db::Transaction* tx)
            {
                //! @todo critical: Implement async transaction

                auto client=m_db->client();
                auto q=db::makeQuery(jobRefIdx(),
                                       db::query::where(job::ref_id,db::query::eq,jobPtr->fieldValue(job::ref_id)).
                                       and_(job::ref_topic,db::query::eq,jobPtr->fieldValue(job::ref_topic)).
                                       and_(job::ref_type,db::query::eq,jobPtr->fieldValue(job::ref_type)),
                                       m_topic
                                    );

                auto findCb=[this,&client,tx,conflictMode,jobPtr,newJob{std::move(newJob)}](db::DbObject obj, Error& ec)
                {
                    if (ec)
                    {
                        //! @todo Log error
                        return false;
                    }

                    // check if job exists
                    if (obj.isNull())
                    {
                        // job does not exist, create a new one

                        ec=client->create(m_topic,jobModel(),jobPtr,tx);
                        if (ec)
                        {
                            //! @todo Log error
                        }

                        replaceJobInQueue(std::move(newJob));
                        return false;
                    }

                    // job exists

                    switch (conflictMode)
                    {
                        case(JobConflictMode::UpdateTime):
                        {
                            // update next time only to lesser new time
                            db::DbObjectT<Job> foundJob{std::move(obj)};
                            if (foundJob->fieldValue(job::next_time)>jobPtr->fieldValue(job::next_time))
                            {
                                auto req=db::update::request(
                                                        db::update::field(job::next_time,db::update::set,jobPtr->fieldValue(job::next_time)),
                                                        db::update::field(job::check_id,db::update::set,jobPtr->fieldValue(job::check_id))
                                                    );
                                auto r=client->readUpdate(m_topic,jobModel(),foundJob->fieldValue(db::object::_id),req,db::update::ModifyReturn::After,tx);
                                if (r)
                                {
                                    //! @todo Log error
                                    ec=r.takeError();
                                    return false;
                                }
                                replaceJobInQueue(r.takeValue());
                            }
                        }
                        break;

                        case(JobConflictMode::Replace):
                        {
                            // update next time only to lesser new time
                            db::DbObjectT<Job> foundJob{std::move(obj)};
                            if (foundJob->fieldValue(job::next_time)>jobPtr->fieldValue(job::next_time))
                            {
                                ec=client->deleteObject(m_topic,jobModel(),foundJob->fieldValue(db::object::_id),tx);
                                if (ec)
                                {
                                    //! @todo Log error
                                    return false;
                                }
                                ec=client->create(m_topic,jobModel(),jobPtr,tx);
                                if (ec)
                                {
                                    //! @todo Log error
                                    return false;
                                }
                                replaceJobInQueue(std::move(newJob));
                            }
                        }
                        break;

                        case(JobConflictMode::SkipNewJob): break;
                    }

                    // stop further lookups
                    return false;
                };

                auto ec=client->findCb(jobModel(),q,std::move(findCb),tx,true);
                if (ec)
                {
                    //! @todo Log error
                }
                return ec;
            };
            m_db->transaction(ctx,std::move(transactionCb),std::move(txHandler),m_topic);
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

        template <typename ContextT, typename CallbackT>
        void removeJob(
            common::SharedPtr<ContextT> ctx,
            CallbackT callback,
            common::SharedPtr<Job> jobObj
            )
        {
            auto q=db::allocateQuery(m_factory,jobRefIdx(),
                                    db::query::where(db::object::_id,db::query::eq,jobObj->fieldValue(db::object::_id)).
                                                and_(job::check_id,db::query::eq,jobObj->fieldValue(job::check_id)),
                                     m_topic
                                    );
            auto removeCb=[selfCtx{sharedMainCtx()},this,ctx,callback{std::move(callback)},jobObj{std::move(jobObj)}](auto, const Error& ec)
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

        template <typename ContextT, typename CallbackT>
        void run(common::SharedPtr<ContextT> ctx, CallbackT cb)
        {
            dequeueJobs(std::move(ctx),std::move(cb));
        }

        template <typename ContextT, typename CallbackT>
        void updateJobNextTime(common::SharedPtr<ContextT> ctx, CallbackT cb, common::SharedPtr<Job> jobObj)
        {
            auto q=db::allocateQuery(m_factory,jobRefIdx(),
                                       db::query::where(db::object::_id,db::query::eq,jobObj->fieldValue(db::object::_id)).
                                                    and_(job::check_id,db::query::eq,jobObj->fieldValue(job::check_id)),
                                       m_topic
                                    );

            auto jobPtr=jobObj.get();
            auto updateCb=[this,jobObj{std::move(jobObj)},cb{std::move(cb)}](auto, const Error& ec)
            {
                if (ec)
                {
                    //! @todo Log error
                }
                cb(ec);
            };
            m_db->updateMany(
                ctx,
                std::move(updateCb),
                jobModel(),
                std::move(q),
                db::update::allocateRequest(m_factory,db::update::field(job::next_time,db::update::set,jobPtr->fieldValue(job::next_time))),
                nullptr,
                m_topic
            );
        }

        static auto now()
        {
            return common::DateTime::currentUtc();
        }

    private:

        struct JobWrapper
        {
            common::SharedPtr<Job> job;
        };

        template <typename ContextT, typename CallbackT>
        void fetchJobs(common::SharedPtr<ContextT> ctx, CallbackT backgroundCb)
        {
            //! @todo Implement async transaction
            auto txHandler=[this](db::Transaction* tx)
            {
                auto client=m_db->client();
                size_t jobsCount=0;

                // read jobs from database
                auto q=db::makeQuery(jobRefIdx(),
                                   db::query::where(job::next_time,db::query::lte,now(),db::query::Order::Asc),
                                   m_topic
                                );
                auto findCb=[this,&client,tx,&jobsCount](db::DbObject obj, Error& ec)
                {
                    if (ec)
                    {
                        //! @todo Log error
                        return false;
                    }

                    // no job found, return
                    if (obj.isNull())
                    {
                        return false;
                    }

                    // hold job until it is compleled or job_hold_interval elapsed
                    db::DbObjectT<Job> foundJob{std::move(obj)};
                    auto dt=now();
                    dt.addSeconds(this->config().fieldValue(scheduler_config::job_hold_period));
                    setJobNextTime(foundJob.get(),dt);
                    auto req=db::update::request(
                            db::update::field(job::next_time,db::update::set,foundJob->fieldValue(job::next_time))
                        );
                    ec=client->update(m_topic,jobModel(),foundJob->fieldValue(db::object::_id),req,tx);
                    if (ec)
                    {
                        //! @todo Log error
                        return false;
                    }

                    // enqueue job
                    enqueueJob(std::move(foundJob));
                    jobsCount++;

                    // read next job if queue is not full
                    if (m_queue.size() < this->config().fieldValue(scheduler_config::job_queue_depth))
                    {
                        return true;
                    }
                    return false;
                };
                auto ec=client->findCb(jobModel(),q,std::move(findCb),tx,true);
                if (ec)
                {
                    //! @todo Log error
                    return ec;
                }

                // read next job if queueu is not full
                //! @todo Log job count and queue size

                // done transaction
                return Error{};
            };
            auto transactionCb=[this,ctx{std::move(ctx)},selfCtx{sharedMainCtx()},backgroundCb{std::move(backgroundCb)}](auto, const Error& ec)
            {
                if (ec)
                {
                    //! @todo Log error
                }

                // call background worker callback here
                backgroundCb();

                m_cacheMutex.lock();
                auto queueEmpty=m_queue.empty();
                m_cacheMutex.unlock();

                if (!queueEmpty)
                {
                    wakeUp();
                }
                else
                {
                    // sleep
                }
            };
            m_db->transaction(ctx,std::move(transactionCb),std::move(txHandler),m_topic);
        }

        template <typename ContextT, typename CallbackT>
        void dequeueJobs(common::SharedPtr<ContextT> ctx, CallbackT backgroundCb)
        {
            // if queue is empty then invoke fetchJobs()
            {
                common::MutexScopedLock l{m_cacheMutex};
                if (m_queue.empty())
                {
                    fetchJobs(std::move(ctx),std::move(backgroundCb));
                    return;
                }
            }

            // check in loop if worker can accept jobs
            size_t dequeCount=0;
            bool hasMoreJobs=false;
            while (m_jobRunner->canAcceptJobs())
            {
                // dequeue job
                m_cacheMutex.lock();
                hasMoreJobs=!m_queue.empty();
                if (!hasMoreJobs)
                {
                    m_cacheMutex.unlock();
                    break;
                }
                auto item=m_queue.mruItem();
                auto dequeueJob=item->job;
                m_queue.removeItem(item);
                m_cacheMutex.unlock();
                dequeCount++;

                // invoke job in worker
                auto completeCb=[selfCtx{this->sharedMainCtx()},this](auto ctx, const Error& ec, auto doneJob, JobResultOp postCompleteOp=JobResultOp::RetryLater)
                {
                    if (ec)
                    {
                        //! @todo Log error
                    }

                    switch (postCompleteOp)
                    {
                        case (JobResultOp::Remove):
                        {
                            // remove job
                            auto jobPtr=doneJob.get();
                            removeJob(std::move(ctx),
                                  [](auto, const Error& ec)
                                  {
                                    if (ec)
                                    {
                                        //! @todo Log error
                                    }
                                  },
                                  std::move(doneJob)
                            );
                        }
                        break;

                        case (JobResultOp::UpdateNextTime):
                        {
                            // update next time using explicit time value from job object
                            auto updateCb=[](auto, const Error& ec)
                            {
                                if (ec)
                                {
                                    //! @todo Log error
                                }
                            };
                            updateJobNextTime(std::move(ctx),std::move(updateCb),std::move(doneJob));
                        }
                        break;

                        case (JobResultOp::RetryLater):
                        {
                            // update next time using job_retry_interval
                            auto updateCb=[](auto, const Error& ec)
                            {
                                if (ec)
                                {
                                    //! @todo Log error
                                }
                            };
                            setJobRetryTime(doneJob.get(),ec);
                            updateJobNextTime(std::move(ctx),std::move(updateCb),std::move(doneJob));
                        }
                        break;
                    }
                };
                m_jobRunner->invoke(ctx,std::move(dequeueJob),std::move(completeCb));

                // repeat until queue is not empty or bucket size is reached
                if (dequeCount > config().fieldValue(scheduler_config::job_bucket_size))
                {
                    break;
                }
            }

            // call background worker callback here
            backgroundCb();

            // if worker can not accept jobs then sleep
            if (!m_jobRunner->canAcceptJobs())
            {                
                return;
            }

            // invoke wake up to dequeue/fetch jobs asynchronously
            wakeUp();
        }

        template <typename JobT>
        void setJobRetryTime(JobT* jobPtr, const Error& ec={})
        {
            auto dt=common::DateTime::current();
            if (!ec && jobPtr->field(job::period).isSet())
            {
                dt.addSeconds(jobPtr->fieldValue(job::period));
            }
            else
            {
                dt.addSeconds(this->config().fieldValue(scheduler_config::job_retry_interval));
            }
            jobPtr->setFieldValue(job::next_time,dt);
        }

        void replaceJobInQueue(common::SharedPtr<Job> newJob)
        {
            common::MutexScopedLock l{m_cacheMutex};

            auto* item=m_queue.item(*newJob);
            if (item!=nullptr)
            {
                item->job=std::move(newJob);
            }
        }

        std::shared_ptr<db::AsyncClient> m_db;
        const common::pmr::AllocatorFactory* m_factory;

        common::CacheLru<JobKey,JobWrapper> m_queue;
        common::MutexLock m_cacheMutex;

        Worker* m_jobRunner;
        BackgroundWorker* m_background;
        std::string m_topic;
};

HATN_SCHEDULER_NAMESPACE_END

namespace std {

template <>
struct less<HATN_SCHEDULER_NAMESPACE::JobKey>
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
        return !less<HATN_SCHEDULER_NAMESPACE::JobKey>{}(r,l);
    }
};

} // namespace std

#endif // HATNSCHEDULER_H
