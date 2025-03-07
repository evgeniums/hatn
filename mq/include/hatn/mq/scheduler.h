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

#include <hatn/base/configobject.h>

#include <hatn/dataunit/syntax.h>

#include <hatn/db/object.h>
#include <hatn/db/model.h>
#include <hatn/db/client.h>

#include <hatn/mq/mq.h>

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

HDU_UNIT(config,
    HDU_FIELD(job_bucket_size,TYPE_UINT32,1,false,32)
    HDU_FIELD(job_retry_interval,TYPE_UINT32,2,false,300)
    HDU_FIELD(job_hold_period,TYPE_UINT32,3,false,900)
    HDU_FIELD(worker_count,TYPE_UINT8,4,false,1)
)

HDU_UNIT_WITH(job,(HDU_BASE(db::object)),
    HDU_FIELD(ref_id,TYPE_UINT32,1,true)
    HDU_FIELD(ref_type,TYPE_STRING,2)
    HDU_FIELD(next_time,TYPE_DATETIME,3)
    HDU_FIELD(maybe_busy,TYPE_UINT32,4)
    HDU_FIELD(content,TYPE_DATAUNIT,5)
)

HATN_DB_INDEX(jobTimeIdx,job::next_time)
HATN_DB_INDEX(jobRefIdx,job::ref_id,job::ref_type)
HATN_DB_INDEX(jobBusyIdx,job::maybe_busy)
HATN_DB_INDEX(jobRefTypeIdx,job::ref_type)

HATN_DB_MODEL_WITH_CFG(jobModel,job,HATN_DB_NAMESPACE::ModelConfig("scheduler_jobs"),jobTimeIdx(),jobRefIdx(),jobBusyIdx(),jobRefTypeIdx())

struct SchedulerTraits
{
    using Job=job::managed;    
    using Worker=hana::false_;

    static const auto& jobModel()
    {
        return HATN_SCHEDULER_NAMESPACE::jobModel();
    }
};

template <typename JobT=typename SchedulerTraits::Job>
struct JobItem
{
    using JobShared=common::SharedPtr<JobT>;

    JobShared job;
    int delay=0;
    bool noDb=false;
};

template <typename WorkerT>
class WorkerPool
{
    using Worker=WorkerT;
    common::CacheLru<common::TaskContextId,common::SharedPtr<Worker>> m_workers;
};

template <typename Traits=SchedulerTraits>
class Scheduler : public HATN_BASE_NAMESPACE::ConfigObject<config::type>,
                  public common::TaskSubcontext
{
    public:

        using Job=typename Traits::Job;
        using Worker=typename Traits::Worker;

    private:

        static const auto& jobModel()
        {
            return Traits::jobModel();
        }

        struct JobItem
        {
            using JobShared=common::SharedPtr<Job>;

            JobShared job;
            int delay=0;
            bool noDb=false;
        };

    public:

        void postJob(common::SharedPtr<Job> job, Mode mode=Mode::Queued, int delay=0)
        {

        }

        template <typename IdT>
        void removeJob(IdT refId, IdT refType={})
        {

        }

        void wakeUp()
        {}

        void start()
        {}

        void stop()
        {}

    private:

        std::shared_ptr<db::Client> m_db;
        common::AsioDeadlineTimer m_timer;
        common::CacheLru<du::ObjectId,JobItem> m_queue;
        common::Thread* m_thread;
        common::pmr::AllocatorFactory* m_factory;
        common::SharedPtr<Worker> m_worker;
};

HATN_SCHEDULER_NAMESPACE_END

#endif // HATNSCHEDULER_H
