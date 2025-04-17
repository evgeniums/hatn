/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file logcontext/streamlogger.h
  *
  *  Defines context logger to char stream.
  *
  */

/****************************************************************************/

#ifndef HATNFILELOGGER_H
#define HATNFILELOGGER_H

#include <hatn/common/pmr/allocatorfactory.h>

#include <hatn/logcontext/context.h>
#include <hatn/logcontext/streamlogger.h>

HATN_LOGCONTEXT_NAMESPACE_BEGIN

namespace detail {
using LoggerThread=common::ThreadWithQueue<common::TaskInlineContext<common::FmtAllocatedBufferChar>>;

class FileLoggerTraits : public BufToStream
{
    using ThreadTask=common::TaskInlineContext<common::FmtAllocatedBufferChar>;

    public:

        using BufToStream::BufToStream;

        struct BufWrapper
        {
            BufWrapper(common::FmtAllocatedBufferChar* notThreadSafeBuf)
                    :  m_buf(notThreadSafeBuf),
                       m_threadTask(nullptr)
            {}

            common::FmtAllocatedBufferChar& buf()
            {
                return *m_buf;
            }

            const common::FmtAllocatedBufferChar& buf() const
            {
                return *m_buf;
            }

            void setThreadTask(ThreadTask* threadTask,
                               common::FmtAllocatedBufferChar* buf
                               )
            {
                m_threadTask=threadTask;
                m_buf=buf;
            }

            common::FmtAllocatedBufferChar* m_buf;
            ThreadTask* m_threadTask;
        };

        BufWrapper prepareBuf()
        {
            BufWrapper wrapper{&m_notThreadSafeBuf};

            if (m_thread)
            {
                auto* task=m_thread->prepare();
                auto* buf=task->createObj(m_factory->dataAllocator<char>());
                wrapper.setThreadTask(task,buf);
            }

            return wrapper;
        }

        void logBuf(const BufWrapper& bufWrapper)
        {
            this->log(bufWrapper.buf());
        }

        void logBufError(const BufWrapper& bufWrapper)
        {
            this->logError(bufWrapper.buf());
        }

        Error loadConfig(
            const HATN_BASE_NAMESPACE::ConfigTree& configTree,
            const std::string& configPath,
            const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault()
        );

        Error start();

        Error close();

        void setAllocatorFactory(const common::pmr::AllocatorFactory* factory)
        {
            m_factory=factory;
        }

    private:

        const common::pmr::AllocatorFactory* m_factory=common::pmr::AllocatorFactory::getDefault();
        std::shared_ptr<LoggerThread> m_thread;
        common::FmtAllocatedBufferChar m_notThreadSafeBuf;
    };
}

constexpr const char* FileLoggerName="filelogger";

template <typename ContextT=Subcontext>
class FileLoggerT : public BufLoggerT<detail::FileLoggerTraits,ContextT>
{
public:

    template <typename ...TraitsArgs>
    FileLoggerT(TraitsArgs&& ...traitsArgs) :
        BufLoggerT<StreamLoggerTraits,ContextT>(FileLoggerName,std::forward<TraitsArgs>(traitsArgs)...)
    {}

    virtual Error loadConfig(
        const HATN_BASE_NAMESPACE::ConfigTree& configTree,
        const std::string& configPath
        ) override
    {
        return this->traits().loadConfig(configTree,configPath);
    }

    virtual Error start() override
    {
        return this->traits().start();
    }

    virtual Error close() override
    {
        return this->traits().close();
    }

    virtual void setAppConfig(const AppConfig& cfg) override
    {
        this->traits().setAllocatorFactory(cfg.allocatorFactory());
    }
};

using FileLogger=FileLoggerT<>;
using FileLogHandler=FileLogger;

HATN_LOGCONTEXT_NAMESPACE_END

#endif // HATNFILELOGGER_H
