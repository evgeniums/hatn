/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file logcontext/fileloggertraits.h
  *
  */

/****************************************************************************/

#ifndef HATNFILELOGGERTRAITS_H
#define HATNFILELOGGERTRAITS_H

#include <hatn/common/pmr/allocatorfactory.h>

#include <hatn/logcontext/context.h>
#include <hatn/logcontext/streamlogger.h>

HATN_LOGCONTEXT_NAMESPACE_BEGIN

using LoggerThread=common::ThreadWithQueue<common::TaskInlineContext<common::FmtAllocatedBufferChar>>;
using LoggerThreadTask=common::TaskInlineContext<common::FmtAllocatedBufferChar>;

struct FileLoggerBufWrapper
{
    FileLoggerBufWrapper(common::FmtAllocatedBufferChar* notThreadSafeBuf)
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

    void setThreadTask(LoggerThreadTask* threadTask,
                       common::FmtAllocatedBufferChar* buf
                       )
    {
        m_threadTask=threadTask;
        m_buf=buf;
    }

    common::FmtAllocatedBufferChar* m_buf;
    LoggerThreadTask* m_threadTask;
};

class FileLoggerTraits_p;

class HATN_LOGCONTEXT_EXPORT FileLoggerTraits
{    
    public:

        FileLoggerTraits();
        ~FileLoggerTraits();

        FileLoggerTraits(const FileLoggerTraits&)=delete;
        FileLoggerTraits(FileLoggerTraits&&)=default;
        FileLoggerTraits& operator=(const FileLoggerTraits&)=delete;
        FileLoggerTraits& operator=(FileLoggerTraits&&)=default;

        FileLoggerBufWrapper prepareBuf();        

        void releaseBuf(FileLoggerBufWrapper& bufWrapper);

        void logBuf(const FileLoggerBufWrapper& bufWrapper);

        void logBufError(const FileLoggerBufWrapper& bufWrapper);

        Error loadLogConfig(
            const HATN_BASE_NAMESPACE::ConfigTree& configTree,
            const std::string& configPath,
            HATN_BASE_NAMESPACE::config_object::LogRecords& records
        );

        Error start();

        Error close();

        void setAllocatorFactory(const common::pmr::AllocatorFactory* factory) noexcept;

        std::vector<std::string> listFiles() const;

    private:

        std::unique_ptr<FileLoggerTraits_p> d;
};

HATN_LOGCONTEXT_NAMESPACE_END

#endif // HATNFILELOGGERTRAITS_H
