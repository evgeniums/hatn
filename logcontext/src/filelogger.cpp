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

#include <hatn/validator/validator.hpp>
#include <hatn/validator/operators/in.hpp>

#include <hatn/common/locker.h>
#include <hatn/common/plainfile.h>
#include <hatn/common/filesystem.h>

#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/ipp/syntax.ipp>

#include <hatn/logcontext/logcontext.h>
#include <hatn/logcontext/buflogger.h>

#include <hatn/logcontext/fileloggertraits.h>
#include <hatn/logcontext/filelogger.h>

HATN_LOGCONTEXT_NAMESPACE_BEGIN

namespace {

#ifdef HATN_FILELOGGER_LOGROTATE_MAX_FILE_COUNT
constexpr static uint8_t DefaultMaxFileCount=8;
#else
constexpr uint8_t DefaultMaxFileCount=8;
#endif

constexpr uint32_t DefaultRotationPeriodHours=24*7;

constexpr const char* ErrorLogModeMain="main_log";
constexpr const char* ErrorLogModeError="error_log";
constexpr const char* ErrorLogModeBoth="both_logs";

enum class ErrorLogMode : uint8_t
{
    Error,
    Main,
    Both
};

constexpr const char* LogrotateModeNone="none";
constexpr const char* LogrotateModeInapp="inapp";
constexpr const char* LogrotateModeSighup="sighup";

}

//! @todo Set inapp default logrotate mode

HDU_UNIT(logrotate_config,
    HDU_FIELD(max_file_count,TYPE_UINT8,1,false,DefaultMaxFileCount)
    HDU_FIELD(period_hours,TYPE_UINT32,2,false,DefaultRotationPeriodHours)
    HDU_FIELD(mode,TYPE_STRING,3,false,LogrotateModeNone)
)

HDU_UNIT(filelogger_config,
    HDU_FIELD(log_file,TYPE_STRING,1)
    HDU_FIELD(error_file,TYPE_STRING,2)
    HDU_FIELD(error_log_mode,TYPE_STRING,3,false,ErrorLogModeMain)
    HDU_FIELD(log_console,TYPE_BOOL,4)
    HDU_FIELD(logrotate,logrotate_config::TYPE,5)
)

namespace {

HATN_VALIDATOR_USING

auto makeValidator()
{
    return validator(
            _[filelogger_config::error_log_mode](in,range({ErrorLogModeMain,ErrorLogModeError,ErrorLogModeBoth})),
            _[filelogger_config::log_file](empty(flag,false)),
            _[filelogger_config::logrotate][logrotate_config::mode](in,range({LogrotateModeNone,LogrotateModeInapp,LogrotateModeSighup})),
            _[filelogger_config::logrotate][logrotate_config::period_hours](gte,1),
            _[filelogger_config::logrotate][logrotate_config::max_file_count](gte,1)
        );
}

}

//---------------------------------------------------------------

template class HATN_LOGCONTEXT_EXPORT BufLoggerT<FileLoggerTraits>;

//---------------------------------------------------------------

class FileLoggerTraits_p : public HATN_BASE_NAMESPACE::ConfigObject<filelogger_config::type>
{
    public:

        const common::pmr::AllocatorFactory* factory=common::pmr::AllocatorFactory::getDefault();
        std::shared_ptr<LoggerThread> thread;
        common::FmtAllocatedBufferChar sharedBuf;

        common::MutexLock mutex;
        bool locked=false;

        lib::optional<common::PlainFile> logFile;
        lib::optional<common::PlainFile> errorLogFile;
        bool logConsole=false;
        ErrorLogMode errorLogMode=ErrorLogMode::Both;

        bool useLogThread() const noexcept
        {
            return thread && thread->isStarted();
        }
};

//---------------------------------------------------------------

FileLoggerTraits::FileLoggerTraits() : d(std::make_unique<FileLoggerTraits_p>())
{
}

//---------------------------------------------------------------

FileLoggerTraits::~FileLoggerTraits()
{
}

//---------------------------------------------------------------

FileLoggerBufWrapper FileLoggerTraits::prepareBuf()
{
    FileLoggerBufWrapper wrapper{&d->sharedBuf};

    if (d->useLogThread())
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
        d->sharedBuf.clear();
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
    if (d->errorLogFile)
    {
        std::ignore=d->errorLogFile->flush();
        d->errorLogFile->close();
        d->errorLogFile.reset();
    }
    if (d->logFile)
    {
        std::ignore=d->logFile->flush();
        d->logFile->close();
        d->logFile.reset();
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
    // load config
    auto validator=makeValidator();
    auto ec=d->loadLogConfig(configTree,configPath,records,validator);
    HATN_CHECK_EC(ec)
    d->logConsole=d->config().fieldValue(filelogger_config::log_console);

    //! @todo init logrotate

    // open log file
    d->logFile=common::PlainFile{};
    std::string logFileName{d->config().fieldValue(filelogger_config::log_file)};
    lib::filesystem::path logFilePath{logFileName};
    if (!lib::filesystem::exists(logFilePath))
    {
        lib::fs_error_code fsec;
        lib::filesystem::create_directories(logFilePath.parent_path(),fsec);
        if (fsec)
        {
            ec=lib::makeFilesystemError(fsec);
            HATN_CHECK_CHAIN_EC(ec,fmt::format(_TR("failed to create parent directories for log file {}","logcontext"),logFileName))
        }
    }
    ec=d->logFile->open(logFileName,common::PlainFile::Mode::append);
    HATN_CHECK_CHAIN_EC(ec,fmt::format(_TR("failed to open log file {}","logcontext"),logFileName))

    // open error log file
    auto errorLogMode=d->config().fieldValue(filelogger_config::error_log_mode);
    if (errorLogMode==ErrorLogModeMain)
    {
        d->errorLogMode=ErrorLogMode::Main;
    }
    else if (errorLogMode==ErrorLogModeError)
    {
        d->errorLogMode=ErrorLogMode::Error;
    }
    else
    {
        d->errorLogMode=ErrorLogMode::Both;
    }
    if (d->errorLogMode!=ErrorLogMode::Main)
    {
        std::string errorLogFileName{d->config().fieldValue(filelogger_config::error_file)};
        if (errorLogFileName.empty())
        {
            lib::filesystem::path path{logFileName};
            auto newExt=fmt::format("error{}",path.extension().string());
            path.replace_extension(newExt);
            errorLogFileName=path.string();
        }
        d->errorLogFile=common::PlainFile{};
        lib::filesystem::path errorLogFilePath{errorLogFileName};
        if (!lib::filesystem::exists(errorLogFilePath))
        {
            lib::fs_error_code fsec;
            lib::filesystem::create_directories(errorLogFilePath.parent_path(),fsec);
            if (fsec)
            {
                ec=lib::makeFilesystemError(fsec);
                HATN_CHECK_CHAIN_EC(ec,fmt::format(_TR("failed to create parent directories for error log file {}","logcontext"),errorLogFileName))
            }
        }
        ec=d->logFile->open(logFileName,common::PlainFile::Mode::append);
        HATN_CHECK_CHAIN_EC(ec,fmt::format(_TR("failed to open error log file {}","logcontext"),errorLogFileName))
    }

    // done
    return OK;
}

//---------------------------------------------------------------

void FileLoggerTraits::logBuf(const FileLoggerBufWrapper& bufWrapper)
{
    auto log=[this](const common::FmtAllocatedBufferChar& buf)
    {
        bool fallbackLogConsole=false;
        try
        {
            if (d->logFile)
            {
                d->logFile->write(buf.data(),buf.size());
                lib::string_view ret("\n");
                d->logFile->write(ret.data(),ret.size());
            }
        }
        catch (...)
        {
            fallbackLogConsole=true;
        }

        if (fallbackLogConsole || d->logConsole)
        {
            std::copy(buf.begin(),buf.end(),std::ostream_iterator<char>(std::cout));
            std::cout<<std::endl;
        }
    };

    if (d->useLogThread())
    {
        bufWrapper.m_threadTask->handler=[log](const common::FmtAllocatedBufferChar& buf)
        {
            log(buf);
        };
        d->thread->post(bufWrapper.m_threadTask);
    }
    else
    {
        log(bufWrapper.buf());
    }
}

//---------------------------------------------------------------

void FileLoggerTraits::logBufError(const FileLoggerBufWrapper& bufWrapper)
{
    auto log=[this](const common::FmtAllocatedBufferChar& buf)
    {
        bool fallbackLogConsole=false;
        try
        {
            lib::string_view ret("\n");
            if (d->errorLogFile)
            {
                d->errorLogFile->write(buf.data(),buf.size());
                d->logFile->write(ret.data(),ret.size());
            }
            if (d->errorLogMode!=ErrorLogMode::Error && d->logFile)
            {
                d->logFile->write(buf.data(),buf.size());

                d->logFile->write(ret.data(),ret.size());
            }
        }
        catch (...)
        {
            fallbackLogConsole=true;
        }
        if (fallbackLogConsole || d->logConsole)
        {
            std::copy(buf.begin(),buf.end(),std::ostream_iterator<char>(std::cout));
            std::cout<<std::endl;
        }
    };

    if (d->useLogThread())
    {
        bufWrapper.m_threadTask->handler=[log](const common::FmtAllocatedBufferChar& buf)
        {
            log(buf);
        };
        d->thread->post(bufWrapper.m_threadTask);
    }
    else
    {
        log(bufWrapper.buf());
    }
}

//---------------------------------------------------------------

HATN_LOGCONTEXT_NAMESPACE_END
