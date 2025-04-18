/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file logcontext/filelogger.сpp
  *
  * Contains definition of file logger.
  *
  */

#include <hatn//common/locker.h>

#include <hatn/logcontext/logcontext.h>
#include <hatn/logcontext/buflogger.h>

#include <hatn/logcontext/fileloggertraits.h>
#include <hatn/logcontext/filelogger.h>

HATN_LOGCONTEXT_NAMESPACE_BEGIN

//---------------------------------------------------------------

template class HATN_LOGCONTEXT_EXPORT BufLoggerT<FileLoggerTraits>;

//---------------------------------------------------------------

class FileLoggerTraits_p
{
    public:

        const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault();
        std::shared_ptr<LoggerThread> thread;
        common::FmtAllocatedBufferChar sharedBuf;

        common::MutexLock mutex;
        bool locked;

        std::vector<std::ofstream> files;
};

//---------------------------------------------------------------

FileLoggerBufWrapper FileLoggerTraits::prepareBuf()
{
    FileLoggerBufWrapper wrapper{&d->sharedBuf};

    if (d->thread && d->thread->isStarted())
    {
        auto* task=d->thread->prepare();
        auto* buf=task->createObj(d->factory->dataAllocator<char>());
        wrapper.setThreadTask(task,buf);
    }
    else
    {
        d->mutex.lock();
        d->locked=true;
    }

    return wrapper;
}

//---------------------------------------------------------------

void FileLoggerTraits::releaseBuf(FileLoggerBufWrapper&)
{
    if (d->locked)
    {
        d->locked=false;
        d->mutex.unlock();
    }
    else
    {
        // buf is destroyed in worker thread
    }
}

//---------------------------------------------------------------

void FileLoggerTraits::setAllocatorFactory(const common::pmr::AllocatorFactory* factory) noexcept
{
    d->factory=factory;
}

//---------------------------------------------------------------

Error FileLoggerTraits::start()
{
    if (d->thread)
    {
        d->thread->start();
    }
    return OK;
}

//---------------------------------------------------------------

Error FileLoggerTraits::close()
{
    if (d->thread)
    {
        d->thread->stop();
    }

    // close open files
    for (auto&& file : d->files)
    {
        file.flush();
        file.close();
    }

    // done
    return OK;
}

//---------------------------------------------------------------

Error FileLoggerTraits::loadLogConfig(
        const HATN_BASE_NAMESPACE::ConfigTree& configTree,
        const std::string& configPath,
        HATN_BASE_NAMESPACE::config_object::LogRecords& records
    )
{
    //! @todo load config

    //! @todo init logrotate

    //! @todo open files

    // done
    return OK;
}

//---------------------------------------------------------------

HATN_LOGCONTEXT_NAMESPACE_END
