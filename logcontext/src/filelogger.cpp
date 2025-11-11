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

#include <boost/asio/signal_set.hpp>

#include <hatn/validator/validator.hpp>
#include <hatn/validator/operators/in.hpp>

#include <hatn/common/locker.h>
#include <hatn/common/plainfile.h>
#include <hatn/common/filesystem.h>
#include <hatn/common/datetime.h>

#include <hatn/dataunit/syntax.h>
#include <hatn/dataunit/ipp/syntax.ipp>

#include <hatn/logcontext/logcontext.h>
#include <hatn/logcontext/contextlogger.h>
#include <hatn/logcontext/buflogger.h>

#include <hatn/logcontext/fileloggertraits.h>
#include <hatn/logcontext/filelogger.h>

HATN_LOGCONTEXT_NAMESPACE_BEGIN

namespace {

constexpr const uint64_t UsPerHour=3600000000;

constexpr const size_t MaxTsFileSize=8;

#ifdef HATN_FILELOGGER_LOGROTATE_MAX_FILE_COUNT
constexpr const uint8_t DefaultMaxFileCount=HATN_FILELOGGER_LOGROTATE_MAX_FILE_COUNT;
#else
constexpr const uint8_t DefaultMaxFileCount=4;
#endif

constexpr uint32_t DefaultRotationPeriodHours=24*7;

constexpr const char* ErrorLogModeMain="main_log";
constexpr const char* ErrorLogModeError="error_log";
constexpr const char* ErrorLogModeBoth="main_and_error";

enum class ErrorLogMode : uint8_t
{
    Error,
    Main,
    Both
};

constexpr const char* LogRotateModeNone="none";
constexpr const char* LogRotateModeInapp="inapp";
constexpr const char* LogRotateModeSighup="sighup";

enum class LogRotateMode : uint8_t
{
    None,
    Inapp,
    Sighup
};

}

HDU_UNIT(logrotate_config,
    HDU_FIELD(max_file_count,TYPE_UINT8,1,false,DefaultMaxFileCount)
    HDU_FIELD(period_hours,TYPE_UINT32,2,false,DefaultRotationPeriodHours)
    HDU_FIELD(mode,TYPE_STRING,3,false,LogRotateModeInapp)
)

HDU_UNIT(filelogger_config,
    HDU_FIELD(log_file,TYPE_STRING,1)
    HDU_FIELD(error_file,TYPE_STRING,2)
    HDU_FIELD(error_log_mode,TYPE_STRING,3,false,ErrorLogModeMain)
    HDU_FIELD(log_console,TYPE_BOOL,4)
    HDU_FIELD(logger_thread,TYPE_BOOL,5,false,true)
    HDU_FIELD(logrotate,logrotate_config::TYPE,6)
)

namespace {

HATN_VALIDATOR_USING

auto makeValidator()
{
    return validator(
            _[filelogger_config::error_log_mode](in,range({ErrorLogModeMain,ErrorLogModeError,ErrorLogModeBoth})),
            _[filelogger_config::log_file](empty(flag,false)),
            _[filelogger_config::logrotate][logrotate_config::mode](in,range({LogRotateModeNone,LogRotateModeInapp,LogRotateModeSighup})),
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
        LogRotateMode logRotateMode=LogRotateMode::None;

        bool useLogThread() const noexcept
        {
            return thread && thread->isStarted();
        }

        void closeFiles()
        {
            Error ec;
            if (logFile)
            {
                std::ignore=logFile->flush();
                logFile->close(ec);
            }
            if (errorLogFile)
            {
                std::ignore=errorLogFile->flush();
                errorLogFile->close(ec);
            }
        }

        void reopenLogFile(bool closeBeforeOpen=true)
        {
            if (logFile)
            {
                Error ec;
                if (closeBeforeOpen)
                {
                    std::ignore=logFile->flush();
                    logFile->close(ec);
                }
                ec=logFile->open(common::PlainFile::Mode::append);
                if (ec)
                {
                    std::cerr << fmt::format("Failed to reopen log file {} : ({}) - {}",logFile->filename(), ec.value(), ec.message()) << std::endl;
                }
                else
                {
                    std::cout << "Reopened log file " << logFile->filename() << " for log rotation" << std::endl;
                }
            }
        }


        void reopenErrorLogFile(bool closeBeforeOpen=true)
        {
            if (errorLogFile)
            {
                Error ec;
                if (closeBeforeOpen)
                {
                    std::ignore=errorLogFile->flush();
                    errorLogFile->close(ec);
                }
                ec=errorLogFile->open(common::PlainFile::Mode::append);
                if (ec)
                {
                    std::cerr << fmt::format("Failed to reopen log file {} : ({}) - {}",errorLogFile->filename(), ec.value(), ec.message()) << std::endl;
                }
                else
                {
                    std::cout << "Reopened error log file " << logFile->filename() << " for log rotation" << std::endl;
                }
            }
        }

        void reopenFiles(bool closeBeforeOpen=true)
        {
            reopenLogFile(closeBeforeOpen);
            reopenErrorLogFile(closeBeforeOpen);
        }

        void logrotate()
        {
            if (logFile && thread)
            {
                auto periodHours=config().field(filelogger_config::logrotate).get().fieldValue(logrotate_config::period_hours);

                lib::filesystem::path path{logFile->filename()};
                auto newExt=fmt::format("{}.ts",path.extension().string());
                path.replace_extension(newExt);
                auto timestampFileName=path.string();

                // find out if rotation needed
                lib::fs_error_code fsec;
                bool doRotation=!lib::filesystem::exists(timestampFileName,fsec);
                if (!doRotation)
                {
                    common::PlainFile tsFile;
                    auto ec=tsFile.open(timestampFileName,common::File::Mode::scan);
                    if (ec || tsFile.size(ec)>MaxTsFileSize)
                    {
                        doRotation=true;
                    }
                    else
                    {
                        std::string buf;
                        ec=tsFile.readAll(buf);
                        if (ec)
                        {
                            doRotation=true;
                        }
                        else
                        {
                            uint32_t epochTs;
                            auto r = std::from_chars(buf.data(), buf.data() + buf.size(), epochTs, 16);
                            if (r.ec != std::errc())
                            {
                                std::cerr << "Failed to parse last rotation timestamp" << std::endl;
                                doRotation=true;
                            }
                            else
                            {
                                auto dt=common::DateTime::fromEpoch(epochTs);
                                auto current=common::DateTime::currentUtc();
                                auto nextTime=dt;
                                nextTime.addHours(periodHours);
                                doRotation=current.after(nextTime);
                            }
                        }
                    }
                }

                // do rotation
                if (doRotation)
                {
                    // close files
                    closeFiles();
                    auto current=common::DateTime::currentUtc();
                    std::string ts=fmt::format("{:08x}",current.toEpoch());

                    // rename and reopen log file
                    Error ec;
                    if (logFile)
                    {
                        if (logFile->size(ec)!=0)
                        {
                            lib::fs_error_code fsec1;
                            lib::filesystem::path path{logFile->filename()};
                            auto newExt=fmt::format("{}.{}",path.extension().string(),ts);
                            path.replace_extension(newExt);
                            lib::filesystem::rename(logFile->filename(),path,fsec1);
                        }
                        reopenLogFile(false);
                    }

                    // rename and reopen error log file
                    if (errorLogFile)
                    {
                        if (errorLogFile->size(ec)!=0)
                        {
                            lib::fs_error_code fsec1;
                            lib::filesystem::path path{errorLogFile->filename()};
                            auto newExt=fmt::format("{}.{}",path.extension().string(),ts);
                            path.replace_extension(newExt);
                            lib::filesystem::rename(errorLogFile->filename(),path,fsec1);
                        }
                        reopenErrorLogFile(false);
                    }

                    // update timestamp file
                    common::PlainFile tsFile;
                    ec=tsFile.open(timestampFileName,common::File::Mode::write);
                    if (ec)
                    {
                        std::cerr << fmt::format("Failed to open log timestamp file {} : ({}) - {}",timestampFileName, ec.value(), ec.message()) << std::endl;
                    }
                    else
                    {
                        tsFile.write(ts,ec);
                        if (ec)
                        {
                            std::cerr << fmt::format("Failed to write log timestamp file {} : ({}) - {}",timestampFileName, ec.value(), ec.message()) << std::endl;
                        }
                        std::ignore=tsFile.flush();
                        tsFile.close();
                    }

                    // delete outdated files
                    auto maxFileCount=config().field(filelogger_config::logrotate).get().fieldValue(logrotate_config::max_file_count);
                    auto deleteOutdated=[maxFileCount,&timestampFileName](const std::string& baseFilename)
                    {
                        // list log files
                        std::vector<std::string> files;
                        lib::fs_error_code fsec1;
                        lib::filesystem::path path(baseFilename);
                        auto dir=path.parent_path();
                        for (const auto& entry : lib::filesystem::directory_iterator(dir,fsec1))
                        {                                                        
                            auto fileName=entry.path().string();
                            if (fileName!=baseFilename
                                &&
                                fileName!=timestampFileName
                                &&
                                boost::algorithm::starts_with(fileName,baseFilename))
                            {
                                files.push_back(fileName);
                            }
                        }

                        // remove oldest files
                        if (files.size()>maxFileCount)
                        {
                            int removeCount=static_cast<int>(files.size())-static_cast<int>(maxFileCount);
                            std::sort(files.begin(),files.end());
                            for (auto it=files.begin();it!=files.end();++it)
                            {
                                lib::filesystem::remove(*it,fsec1);
                                removeCount--;
                                if (removeCount<=0)
                                {
                                    break;
                                }
                            }
                        }
                    };
                    if (maxFileCount>0)
                    {
                        if (logFile)
                        {
                            deleteOutdated(logFile->filename());
                        }
                        if (errorLogFile)
                        {
                            deleteOutdated(errorLogFile->filename());
                        }
                    }
                }

                thread->installTimer(
                    UsPerHour,
                    [this]()
                    {
                        logrotate();
                        return true;
                    },
                    true
                );
            }
        }
};

//---------------------------------------------------------------

FileLoggerTraits::FileLoggerTraits() : d(std::make_unique<FileLoggerTraits_p>())
{
}

//---------------------------------------------------------------

FileLoggerTraits::~FileLoggerTraits()
{
    std::ignore=close();
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
#ifndef WIN32
    if (d->thread)
    {
        d->thread->start();
        if (d->logRotateMode==LogRotateMode::Sighup)
        {
            boost::asio::signal_set signals(d->thread->asioContextRef(), SIGHUP);
            signals.async_wait(
                [this](const boost::system::error_code& ec,
                                           int signal_number)
                {
                    std::ignore=signal_number;
                    if (ec)
                    {
                        return;
                    }

                    d->reopenFiles();
                }
            );
        }
    }
#endif
    return OK;
}

//---------------------------------------------------------------

Error FileLoggerTraits::close()
{
    Error ec;
    if (d->thread)
    {
        // flush pending logs
        std::ignore=d->thread->execSync([]{},100);

        d->thread->stop();
        d->thread.reset();
    }

    // close open files
    if (d->errorLogFile)
    {
        std::ignore=d->errorLogFile->flush();
        d->errorLogFile->close(ec);
        d->errorLogFile.reset();
    }
    if (d->logFile)
    {
        std::ignore=d->logFile->flush();
        d->logFile->close(ec);
        d->logFile.reset();
    }

    // done
    return ec;
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

    // check config
    const auto& logRotate=d->config().field(filelogger_config::logrotate);
    if (logRotate.isSet())
    {
        auto logRotateMode=logRotate.get().fieldValue(logrotate_config::mode);
        if (logRotateMode==LogRotateModeInapp)
        {
            d->logRotateMode=LogRotateMode::Inapp;
        }
        else if (logRotateMode==LogRotateModeSighup)
        {
            d->logRotateMode=LogRotateMode::Sighup;
        }
        else
        {
            d->logRotateMode=LogRotateMode::None;
        }
    }
    if (d->logRotateMode==LogRotateMode::Sighup && !d->config().fieldValue(filelogger_config::logger_thread))
    {
        return baseError(HATN_BASE_NAMESPACE::BaseError::CONFIG_OBJECT_VALIDATE_ERROR,
                         _TR("log rotation mode sighup can be used only if logger_thread is enabled","logcontext")
                         );
    }

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

        // change permissions
        lib::filesystem::permissions(
            logFilePath.parent_path(),
            lib::filesystem::perms::owner_all,
            lib::filesystem::perm_options::replace,
            fsec
        );
        if (fsec)
        {
            auto ec1=lib::makeFilesystemError(fsec);
            HATN_CTX_ERROR(ec1,"failed to change permissions of logs folder");
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

            // change permissions
            lib::filesystem::permissions(
                errorLogFilePath.parent_path(),
                lib::filesystem::perms::owner_all,
                lib::filesystem::perm_options::replace,
                fsec
            );
            if (fsec)
            {
                auto ec1=lib::makeFilesystemError(fsec);
                HATN_CTX_ERROR(ec1,"failed to change permissions of logs folder");
            }
        }
        ec=d->errorLogFile->open(errorLogFileName,common::PlainFile::Mode::append);
        HATN_CHECK_CHAIN_EC(ec,fmt::format(_TR("failed to open error log file {}","logcontext"),errorLogFileName))
    }

    // init log thread
    if (d->config().fieldValue(filelogger_config::logger_thread))
    {
        d->thread=std::make_shared<LoggerThread>("logger");
    }

    // init logrotate
    if (d->logRotateMode==LogRotateMode::Inapp)
    {
        d->logrotate();
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
                d->errorLogFile->write(ret.data(),ret.size());
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

std::vector<std::string> FileLoggerTraits::listFiles() const
{
    std::vector<std::string> files;
    if (!d->logFile)
    {
        return files;
    }

    //! @todo Handle timestampFileName in one place
    lib::filesystem::path path{d->logFile->filename()};
    auto newExt=fmt::format("{}.ts",path.extension().string());
    path.replace_extension(newExt);
    auto timestampFileName=path.string();

    auto list=[&timestampFileName,&files](const std::string& baseFilename)
    {
        lib::fs_error_code fsec1;
        lib::filesystem::path path(baseFilename);
        auto dir=path.parent_path();
        for (const auto& entry : lib::filesystem::directory_iterator(dir,fsec1))
        {
            auto fileName=entry.path().string();
            if (
                fileName!=timestampFileName
                &&
                boost::algorithm::starts_with(fileName,baseFilename))
            {
                files.push_back(fileName);
            }
        }
        std::sort(files.begin(),files.end());
    };
    list(d->logFile->filename());
    if (d->errorLogFile)
    {
        list(d->errorLogFile->filename());
    }

    return files;
}

//---------------------------------------------------------------

HATN_LOGCONTEXT_NAMESPACE_END
