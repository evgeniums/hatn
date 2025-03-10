/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file mq/backgroundworker.h
  *
  *
  */

/****************************************************************************/

#ifndef HATNBACKGROUNDWORKER_H
#define HATNBACKGROUNDWORKER_H

#include <atomic>

#include <hatn/common/error.h>
#include <hatn/common/thread.h>
#include <hatn/common/taskcontext.h>
#include <hatn/common/asiotimer.h>
#include <hatn/common/weakptr.h>
#include <hatn/common/pointerwithinit.h>

#include <hatn/mq/mq.h>

HATN_MQ_NAMESPACE_BEGIN

template <typename WorkerT, typename WorkContextBuilderT, typename TimerT=common::AsioDeadlineTimer>
class BackgroundWorker : public common::TaskSubcontext
{
    public:

        using Worker=WorkerT;
        using WorkContextBuilder=WorkContextBuilderT;

        BackgroundWorker(
                Worker worker,
                WorkContextBuilder workContextBuilder,
                common::Thread* thread=common::Thread::currentThread()
            ) : m_worker(std::move(worker)),
                m_workContextBuilder(std::move(workContextBuilder)),
                m_thread(thread),
                m_timer(thread),
                m_running(false),
                m_stopped(false),
                m_wakeUpCount(0),
                m_async(true)
        {
            init();
        }

        BackgroundWorker(
            common::Thread* thread=common::Thread::currentThread()
        ) : m_thread(thread),
            m_timer(thread),
            m_running(false),
            m_stopped(false),
            m_wakeUpCount(0),
            m_async(true)
        {
            init();
        }

        //! Set period in microseconds
        void setPeriodUs(uint64_t microseconds)
        {
            m_timer.setPeriodUs(microseconds);
        }

        //! Get period in microseconds
        size_t periodUs() const noexcept
        {
            return m_timer.periodUs();
        }

        //! Set async mode
        void setAsync(bool mode)
        {
            m_async=mode;
        }

        //! Get async mode
        bool isAsync() const noexcept
        {
            return m_async;
        }

        void setWorker(Worker worker)
        {
            m_worker=std::move(worker);
        }

        void setWorkContextBuilder(WorkContextBuilder builder)
        {
            m_workContextBuilder=std::move(builder);
        }

        void wakeUp()
        {
            if (m_stopped.load())
            {
                return;
            }

            if (m_wakeUpCount.fetch_add(1)>0)
            {
                return;
            }

            auto selfCtx=common::toWeakPtr(this->sharedMainCtx());
            m_thread->execAsync(
                [selfCtx{std::move(selfCtx)},this]()
                {
                    auto ctx=selfCtx.lock();
                    if (!ctx)
                    {
                        return;
                    }

                    run();
                }
            );
        }

        void start()
        {
            m_stopped.store(false);
            wakeUp();
        }

        void stop()
        {
            m_stopped.store(true);
            m_timer.stop();
        }

        bool isStopped() const noexcept
        {
            return m_stopped.load();
        }

        bool isRunning() const noexcept
        {
            return m_running.load();
        }

    private:

        void init()
        {
            m_timer.setSingleShot(true);
            m_timer.setAutoAsyncGuardEnabled(false);
        }

        void run()
        {
            if (m_stopped.load())
            {
                return;
            }

            auto selfCtx=common::toWeakPtr(this->sharedMainCtx());
            auto cb=[selfCtx{std::move(selfCtx)},this]()
            {
                auto ctx=selfCtx.lock();
                if (!ctx)
                {
                    return;
                }

                auto handler=[selfCtx{std::move(selfCtx)},this]()
                {
                    auto ctx=selfCtx.lock();
                    if (!ctx)
                    {
                        return;
                    }

                    m_running.store(false);

                    if (m_stopped.load())
                    {
                        return;
                    }

                    if (m_wakeUpCount.exchange(0)>0)
                    {
                        wakeUp();
                    }
                    else
                    {
                        if (m_timer.periodUs()!=0)
                        {
                            auto selfCtx=common::toWeakPtr(this->sharedMainCtx());
                            m_timer.start(
                                [selfCtx{std::move(selfCtx)},this](common::TimerTypes::Status status)
                                {
                                    if (status==common::TimerTypes::Status::Cancel)
                                    {
                                        return;
                                    }

                                    auto ctx=selfCtx.lock();
                                    if (!ctx)
                                    {
                                        return;
                                    }

                                    wakeUp();
                                }
                            );
                        }
                    }
                };

                if (m_async)
                {
                    m_thread->execAsync(std::move(handler));
                }
                else
                {
                    handler();
                }
            };

            m_wakeUpCount.store(0);
            m_running.store(true);
            auto workCtx=m_workContextBuilder->makeContext();
            workCtx->onAsyncHandlerEnter();
            m_worker->run(workCtx,std::move(cb));
            workCtx->onAsyncHandlerExit();
            workCtx.reset();
        }

        common::PointerWithInit<Worker> m_worker;
        common::PointerWithInit<WorkContextBuilder> m_workContextBuilder;

        common::Thread* m_thread;
        TimerT m_timer;

        std::atomic<bool> m_running;
        std::atomic<bool> m_stopped;
        std::atomic<size_t> m_wakeUpCount;

        bool m_async;
};

HATN_MQ_NAMESPACE_END

#endif // HATNBACKGROUNDWORKER_H
