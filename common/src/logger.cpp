/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/logger.сpp
  *
  *      Logger.
  *
  */

#include <exception>
#include <iomanip>
#include <stdio.h>
#include <string.h>
#include <cstring>

#include <hatn/common/types.h>
#include <hatn/common/utils.h>
#include <hatn/common/translate.h>
#include <hatn/common/makeshared.h>
#include <hatn/common/makeshared.h>
#include <hatn/common/logger.h>
#include <hatn/common/loggermoduleimp.h>

#include <hatn/common/mpscqueue.h>
#include <hatn/common/mutexqueue.h>

//! \note Do not move this include upper because boost asio must be included before stacktrace, otherwise WinSock.h defined error on Windows will arises
#include <boost/stacktrace.hpp>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/syscall.h>
#include <time.h>
#include <ctime>
#endif


#ifdef ANDROID

namespace std {

namespace {
inline int ctoi(char c) {
  switch (c) {
    case '0':
      return 0;
    case '1':
      return 1;
    case '2':
      return 2;
    case '3':
      return 3;
    case '4':
      return 4;
    case '5':
      return 5;
    case '6':
      return 6;
    case '7':
      return 7;
    case '8':
      return 8;
    case '9':
      return 9;
    default:
      throw std::runtime_error("Invalid char conversion");
  }
}


int stoi(const std::string& str) {
  int rtn = 0;
  int exp = 1;
  for (auto cp = str.crbegin(); cp != str.crend(); ++cp) {
    char c = *cp;
    if (isdigit(c)) {
      rtn +=  ctoi(c) * exp;
      exp *= 10;
    } else if (c == '+') {
      return rtn;
    } else if (c == '-') {
      return rtn * -1;
    }
    else
    {
	break;
    }
  }
  return 0;
}

}
}

#endif

#include <hatn/common/loggermoduleimp.h>
INIT_LOG_MODULE(global,HATN_COMMON_EXPORT)

HATN_COMMON_NAMESPACE_BEGIN

/********************** LogModuleTable **************************/

//---------------------------------------------------------------
LogModuleTable* LogModuleTable::instance() noexcept
{
    static LogModuleTable table;
    return &table;
}

//---------------------------------------------------------------
LogModuleTable::LogModuleTable(
    ) : m_logRecordAllocator(std::make_shared<pmr::polymorphic_allocator<LogModule::Log>>()),
        m_bufferMemoryResource(pmr::get_default_resource())
{
}

//---------------------------------------------------------------
void LogModuleTable::reset() noexcept
{
    MutexScopedLock l(m_locker);
    std::ignore=l;
    m_table.clear();
    m_elapsedTimer.reset();
}

//---------------------------------------------------------------
LogModule* LogModuleTable::doRegisterModule(LogModule* module)
{
    MutexScopedLock l(m_locker);
    std::ignore=l;
    auto it=m_table.find(module->name());
    if (it!=m_table.end())
    {
        return it->second;
    }
    m_table[module->name()]=module;
    return module;
}

//---------------------------------------------------------------
LogModule* LogModuleTable::doFindModule(const char* name) noexcept
{
    MutexScopedLock l(m_locker);
    std::ignore=l;
    auto it=m_table.find(name);
    if (it!=m_table.end())
    {
        return it->second;
    }
    return nullptr;
}

//---------------------------------------------------------------
LogModule* LogModuleTable::findModule(const char* name) noexcept
{
    return LogModuleTable::instance()->doFindModule(name);
}

//---------------------------------------------------------------
LogModule* LogModuleTable::registerModule(LogModule* module)
{
    return LogModuleTable::instance()->doRegisterModule(module);
}

//---------------------------------------------------------------
std::vector<LogModule*> LogModuleTable::allModules() noexcept
{
    std::vector<LogModule*> modules;
    MutexScopedLock l(LogModuleTable::instance()->m_locker);
    std::ignore=l;
    for (auto&& it : LogModuleTable::instance()->m_table)
    {
        modules.push_back(it.second);
    }
    return modules;
}

/********************** Logger **************************/

static std::atomic<bool> TraceFatalLogs(true);
static Logger::OutputHandler LogOutputHandler;
static Logger::OutputHandler FatalOutputHandler;
static std::atomic<int> Verbosity(static_cast<int>(LoggerVerbosity::INFO));
static std::atomic<int> DebugLevel(0);
static bool LogInSeparateThread=true;
static std::atomic<bool> LogRunning(false);
static std::atomic<bool> LogFileAppend(true);
static std::atomic<bool> LogToStd(false);

//---------------------------------------------------------------
ThreadWithQueue<TaskInlineContext<FmtAllocatedBufferChar>>* Logger::loggerThread(bool reset, pmr::memory_resource* memResource)
{
    static std::shared_ptr<ThreadWithQueue<TaskInlineContext<FmtAllocatedBufferChar>>> thread;
    if (reset)
    {
        thread.reset();
    }
    else if (!thread)
    {
        auto queue=new MPSCQueue<TaskInlineContext<FmtAllocatedBufferChar>>(memResource);
        thread=std::make_shared<ThreadWithQueue<TaskInlineContext<FmtAllocatedBufferChar>>>("logger",queue,true);
    }
    return thread.get();
}

//---------------------------------------------------------------
bool Logger::isSeparateThread() noexcept
{
    return LogInSeparateThread;
}

//---------------------------------------------------------------
const char* Logger::verbosityToString(LoggerVerbosity verbosity)
{
    switch (verbosity)
    {
        case (LoggerVerbosity::DEBUG): return "DEBUG";
        case (LoggerVerbosity::WARNING): return "WARNING";
        case (LoggerVerbosity::INFO): return "INFORMATION";
        case (LoggerVerbosity::ERR): return "ERROR";
        case (LoggerVerbosity::FATAL): return "FATAL";
        default: return "NONE";
    }
    return nullptr;
}

//---------------------------------------------------------------
LoggerVerbosity Logger::stringToVerbosity(const char *name)
{
    if (strcasecmp(name,"debug")==0)
    {
        return LoggerVerbosity::DEBUG;
    }
    else if (strcasecmp(name,"information")==0)
    {
        return LoggerVerbosity::INFO;
    }
    else if (strcasecmp(name,"error")==0)
    {
        return LoggerVerbosity::ERR;
    }
    else if (strcasecmp(name,"warning")==0)
    {
        return LoggerVerbosity::WARNING;
    }
    else if (strcasecmp(name,"fatal")==0)
    {
        return LoggerVerbosity::FATAL;
    }
    return LoggerVerbosity::NONE;
}

//---------------------------------------------------------------
Logger::Logger(
        const LogRecord& log
    ) :
        fatal(log->verbosity==LoggerVerbosity::FATAL),
        trace(log->trace||(fatal&&TraceFatalLogs.load(std::memory_order_relaxed))),
        stdStream((log->verbosity==LoggerVerbosity::INFO)?&(std::cout):&(std::cerr)),
        forceLogToStdInCallerThread(!LogInSeparateThread||log->logInCallerThread||fatal||!LogRunning.load(std::memory_order_acquire)),
        task(forceLogToStdInCallerThread?nullptr:loggerThread()->prepare()),
        inlineBuffer(FormatAllocator<char>(LogModuleTable::bufferMemoryResource())),
        buffer(forceLogToStdInCallerThread?
                         &inlineBuffer
                            :
                         task->createObj(FormatAllocator<char>(LogModuleTable::bufferMemoryResource()))
                   )
{
    if (!LogRunning.load(std::memory_order_acquire))
    {
        return;
    }

    auto elapsed=LogModuleTable::elapsed();
    append("{:#03}:{:#02}:{:#02}.{:#03} t[{}] {}",
           elapsed.hours,elapsed.minutes,elapsed.seconds,elapsed.milliseconds,
           Thread::currentThreadID(),
           verbosityToString(log->verbosity)
           );
    if (log->verbosity==LoggerVerbosity::DEBUG)
    {
        append(":{}",static_cast<int>(log->debugLevel));
    }
    if (log->name!=nullptr)
    {
        append(":{}",log->name);
    }
    if (log->context!=nullptr)
    {
        append(":{}",log->context);
    }
    if (log->tag!=nullptr)
    {
        append(":{}",log->tag);
    };
    append(" :: ");
}

//---------------------------------------------------------------
Logger::~Logger()
{
    if (!LogRunning.load(std::memory_order_acquire)||!LogOutputHandler)
    {
        *stdStream<<lib::toStringView(*buffer)<<std::endl;
        return;
    }

    // append stack trace
    if (trace)
    {
        append("\nCall stack:\n{}\n", boost::stacktrace::to_string(boost::stacktrace::stacktrace()));
    }

    // send to output handler
    if (fatal)
    {
        if (LogToStd.load(std::memory_order_relaxed))
        {
            *stdStream<<lib::toStringView(*buffer)<<std::endl;
        }
        if (FatalOutputHandler)
        {
            FatalOutputHandler(*buffer);
        }
    }
    else
    {
        if (LogInSeparateThread)
        {
            auto thread=loggerThread();
            if (forceLogToStdInCallerThread)
            {
                *stdStream<<lib::toStringView(*buffer)<<std::endl;
            }
            else
            {
                auto stream=stdStream;
                task->handler=[stream](const FmtAllocatedBufferChar& buffer)
                {
                    if (LogToStd.load(std::memory_order_relaxed))
                    {
                        *stream<<lib::toStringView(buffer)<<std::endl;
                    }
                    LogOutputHandler(buffer);
                };
                thread->post(task);
            }
        }
        else
        {
            if (LogToStd.load(std::memory_order_relaxed))
            {
                *stdStream<<lib::toStringView(*buffer)<<std::endl;
            }
            LogOutputHandler(*buffer);
        }
    }
}

//---------------------------------------------------------------
void Logger::setOutputHandler(OutputHandler handler) noexcept
{
    LogOutputHandler=std::move(handler);
}

//---------------------------------------------------------------
void Logger::setFatalLogHandler(OutputHandler handler) noexcept
{
    FatalOutputHandler=std::move(handler);
}

namespace {

inline LoggerVerbosity DefaultVerbosity() noexcept
{
    return static_cast<LoggerVerbosity>(Verbosity.load(std::memory_order_relaxed));
}
//---------------------------------------------------------------
inline int DefaultDebugLevel() noexcept
{
    return DebugLevel.load(std::memory_order_relaxed);
}

}

//---------------------------------------------------------------
void Logger::setDefaultVerbosity(
        LoggerVerbosity verbosity
    ) noexcept
{
    Verbosity.store(static_cast<int>(verbosity),std::memory_order_relaxed);
}

//---------------------------------------------------------------
LoggerVerbosity Logger::defaultVerbosity() noexcept
{
    return DefaultVerbosity();
}

//---------------------------------------------------------------
void Logger::setDefaultDebugLevel(
        int debugLevel
    ) noexcept
{
    DebugLevel.store(debugLevel,std::memory_order_relaxed);
}

//---------------------------------------------------------------
int Logger::defaultDebugLevel() noexcept
{
    return DefaultDebugLevel();
}

//---------------------------------------------------------------
void Logger::setFatalTracing(bool enable) noexcept
{
    TraceFatalLogs.store(enable,std::memory_order_relaxed);
}

//---------------------------------------------------------------
void Logger::setAppendLogFile(bool enable) noexcept
{
    LogFileAppend.store(enable,std::memory_order_relaxed);
}

//---------------------------------------------------------------
void Logger::setLogToStd(bool enable) noexcept
{
    LogToStd.store(enable,std::memory_order_relaxed);
}

//---------------------------------------------------------------
std::string Logger::configureModules(
        const std::vector<std::string> &modules
    )
{
    // reset all modules
    resetModules();

    // iterate modules
    for (auto&& it : modules)
    {
        // default parameters
        LogModule* module=nullptr;
        std::string name;
        LoggerVerbosity verbosity=LoggerVerbosity::DEFAULT;
        int debugLevel=0;
        std::vector<std::string> contexts;
        std::vector<std::string> tags;

        // extract configuration for module
        std::vector<std::string> args;
        std::string moduleStr=it;
        Utils::trimSplit(args,moduleStr,';');
        int i=0;
        for (auto&& it1 : args)
        {
            switch (i)
            {
                case (0):
                {
                    // name
                    name=it1;
                    module=LogModuleTable::findModule(name.c_str());
                    if (module==nullptr)
                    {
                        auto err=_TR("No such log module defined:")+" "+name;
                        return err;
                    }
                }
                break;

                case (1):
                {
                    // verbosity
                    if (!it1.empty())
                    {
                        verbosity=Logger::stringToVerbosity(it1.c_str());
                        auto check=Logger::verbosityToString(verbosity);
                        if (strcasecmp(check,it1.c_str())!=0)
                        {
                            auto err=_TR("Invalid verbosity for log module")+" "+name;
                            return err;
                        }
                    }
                }
                break;

                case (2):
                {
                    // debug level
                    if (!it1.empty())
                    {
                        try
                        {
                            debugLevel=std::stoi(it1);
                        }
                        catch(const std::invalid_argument&)
                        {
                            auto err=_TR("Invalid debug level for log module")+" "+name;
                            return err;
                        }
                    }
                }
                break;

                case (3):
                {
                    // contexts
                    if (!it1.empty())
                    {
                        std::vector<std::string> tmpContexts;
                        std::string tmpContextsStr=it1;
                        Utils::trimSplit(tmpContexts,tmpContextsStr,',');
                        for (auto&& it2 : tmpContexts)
                        {
                            auto context=it2;
                            boost::trim(context);
                            contexts.push_back(context);
                        }
                    }
                }
                break;

                case (4):
                {
                    // tags
                    if (!it1.empty())
                    {
                        std::vector<std::string> tmpTags;
                        std::string tmpTagsStr=it1;
                        Utils::trimSplit(tmpTags,tmpTagsStr,',');
                        for (auto&& it2 : tmpTags)
                        {
                            auto tag=it2;
                            boost::trim(tag);
                            tags.push_back(tag);
                        }
                    }
                }
                break;

            }
            i++;
        }

        // load configuration to module        
        if (module!=nullptr)
        {
            module->configure(verbosity,debugLevel,std::move(contexts),std::move(tags));
        }
        else
        {
            return _TR("Configuration of logger module is mailformed: no module name is set!");
        }
    }

    return std::string();
}

//---------------------------------------------------------------
void Logger::resetModules()
{
    LoggerVerbosity verbosity=LoggerVerbosity::DEFAULT;
    int debugLevel=0;

    for (auto&& it : LogModuleTable::allModules())
    {
        std::vector<std::string> contexts;
        std::vector<std::string> tags;

        auto&& module=it;
        if (module!=nullptr)
        {
            module->configure(verbosity,debugLevel,std::move(contexts),std::move(tags));
        }
    }
}

//---------------------------------------------------------------
void Logger::start(bool logInSeparateThread)
{
    LogInSeparateThread=logInSeparateThread;
    if (LogInSeparateThread)
    {
        loggerThread()->start();
    }

    LogRunning.store(true,std::memory_order_release);
}

//---------------------------------------------------------------
void Logger::stop()
{
    LogRunning.store(false,std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_acquire);
    if (LogInSeparateThread)
    {
        loggerThread()->stop();
        loggerThread(true);
    }
}

//---------------------------------------------------------------
bool Logger::isRunning() noexcept
{
    return LogRunning.load(std::memory_order_relaxed);
}

/********************** LogModule **************************/

//---------------------------------------------------------------
LogModule::LogModule(
        const char *name
    ) noexcept : m_name(name),
        m_verbosity(LoggerVerbosity::DEFAULT),
        m_debugLevel(0),
        m_lock(false),
        /*
        this is only for log module named "thread->d" to avoid infinite loop when logging to "thread->d" module in separate thread->d
         */
       m_logInCallerThread(strcmp(name,"thread->d")==0)
{
}

//---------------------------------------------------------------
const char* LogModule::name() const noexcept
{
    return m_name.c_str();
}

//---------------------------------------------------------------
inline SharedPtr<LogModule::Log> LogModule::createLogRecord(const char* name)
{
    return allocateShared<LogModule::Log>(LogModuleTable::logRecordAllocator(),name);
}

//---------------------------------------------------------------
void LogModule::configure(
        LoggerVerbosity verbosity,
        int debugLevel,
        std::vector<std::string>&& contexts,
        std::vector<std::string>&& tags
    )
{
    m_lock.store(true,std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_acquire);
    m_verbosity=verbosity;
    m_debugLevel=debugLevel;
    m_contexts=std::move(contexts);
    m_tags=std::move(tags);
    std::atomic_thread_fence(std::memory_order_release);
    m_lock.store(false,std::memory_order_relaxed);
}

//---------------------------------------------------------------
inline bool LogModule::checkVerbosity(LoggerVerbosity verbosity) const noexcept
{
    if (m_lock.load(std::memory_order_acquire))
    {
        return false;
    }
    bool ok=false;
    if (verbosity!=LoggerVerbosity::NONE)
    {
        bool defaultVerbosity=m_verbosity==LoggerVerbosity::DEFAULT;
        if (!defaultVerbosity)
        {
            ok=m_verbosity!=LoggerVerbosity::NONE&&static_cast<int>(verbosity)<=static_cast<int>(m_verbosity);
        }
        else
        {
            auto defVerb=DefaultVerbosity();
            ok=defVerb!=LoggerVerbosity::NONE&&static_cast<int>(verbosity)<=static_cast<int>(defVerb);
        }
    }
    return ok;
}

//---------------------------------------------------------------
inline bool LogModule::checkDebugLevel(int level) const noexcept
{
    if (m_lock.load(std::memory_order_acquire))
    {
        return false;
    }
    bool ok=false;
    bool defaultVerbosity=m_verbosity==LoggerVerbosity::DEFAULT;
    if (!defaultVerbosity)
    {
        ok=m_verbosity==LoggerVerbosity::DEBUG
             &&
           level<=m_debugLevel;
    }
    else
    {
        ok=DefaultVerbosity()==LoggerVerbosity::DEBUG
                &&
           level<=DefaultDebugLevel();
    }
    return ok;
}

//---------------------------------------------------------------
inline bool LogModule::checkContext(const char *context) const noexcept
{
    if (m_lock.load(std::memory_order_acquire))
    {
        return false;
    }
    bool ok=m_contexts.empty()||context==nullptr;
    if (!ok)
    {
        for (auto&& it : m_contexts)
        {
            if (strcasecmp(it.c_str(),context)==0)
            {
                ok=true;
                break;
            }
        }
    }
    return ok;
}

//---------------------------------------------------------------
inline bool LogModule::checkTags(const pmr::CStringVector &tags, const char** tag) const noexcept
{
    if (m_lock.load(std::memory_order_acquire))
    {
        return false;
    }
    bool ok=m_tags.empty();
    if (!ok)
    {
        for (auto&& it : m_tags)
        {
            for (auto&& it1 : tags)
            {
                if (strcasecmp(it.c_str(),it1)==0)
                {
                    ok=true;
                    *tag=it1;
                    break;
                }
            }
            if (ok)
            {
                break;
            }
        }
    }
    return ok;
}

//---------------------------------------------------------------
LogRecord LogModule::dbgFilter(
        uint8_t debugLevel
    ) const
{
    if (checkDebugLevel(debugLevel))
    {
        auto record=createLogRecord(name());
        record->verbosity=LoggerVerbosity::DEBUG;
        record->debugLevel=debugLevel;
        return record;
    }
    return LogRecord();
}

//---------------------------------------------------------------
LogRecord LogModule::dbgFilter(
        const char* context,
        uint8_t debugLevel
    ) const
{
    if (checkDebugLevel(debugLevel)&&checkContext(context))
    {
        auto record=createLogRecord(name());
        record->verbosity=LoggerVerbosity::DEBUG;
        record->debugLevel=debugLevel;
        record->context=context;
        return record;
    }
    return LogRecord();
}

//---------------------------------------------------------------
LogRecord LogModule::dbgFilter(
        const pmr::CStringVector& tags,
        uint8_t debugLevel
    ) const
{
    const char* tag=nullptr;
    if (checkDebugLevel(debugLevel)&&checkTags(tags,&tag))
    {
        auto record=createLogRecord(name());
        record->verbosity=LoggerVerbosity::DEBUG;
        record->debugLevel=debugLevel;
        record->tag=tag;
        return record;
    }
    return LogRecord();
}

//---------------------------------------------------------------
LogRecord LogModule::dbgFilter(
        const char* context,
        const pmr::CStringVector& tags,
        uint8_t debugLevel
    ) const
{
    const char* tag=nullptr;
    if (checkDebugLevel(debugLevel)&&checkContext(context)&&checkTags(tags,&tag))
    {
        auto record=createLogRecord(name());
        record->verbosity=LoggerVerbosity::DEBUG;
        record->debugLevel=debugLevel;
        record->context=context;
        record->tag=tag;
        return record;
    }
    return LogRecord();
}

//---------------------------------------------------------------
LogRecord LogModule::filter(
        LoggerVerbosity verbosity,
        const char *context
    ) const
{
    if (checkVerbosity(verbosity)&&checkContext(context))
    {
        auto record=createLogRecord(name());
        record->verbosity=verbosity;
        record->context=context;
        return record;
    }
    return LogRecord();
}

//---------------------------------------------------------------
LogRecord LogModule::filter(
        LoggerVerbosity verbosity,
        const pmr::CStringVector& tags
    ) const
{
    const char* tag=nullptr;
    if (checkVerbosity(verbosity)&&checkTags(tags,&tag))
    {
        auto record=createLogRecord(name());
        record->verbosity=verbosity;
        record->tag=tag;
        return record;
    }
    return LogRecord();
}

//---------------------------------------------------------------
LogRecord LogModule::filter(
        LoggerVerbosity verbosity,
        const char *context,
        const pmr::CStringVector& tags
    ) const
{
    const char* tag=nullptr;
    if (checkVerbosity(verbosity)&&checkContext(context)&&checkTags(tags,&tag))
    {
        auto record=createLogRecord(name());
        record->verbosity=verbosity;
        record->context=context;
        record->tag=tag;
        return record;
    }
    return LogRecord();
}

/********************** FileLogHandler **************************/

//---------------------------------------------------------------
void FileLogHandler::openFile()
{
    if (!logFileName.empty())
    {
        if (LogFileAppend.load(std::memory_order_relaxed))
        {
            logFile.open(logFileName,std::ios_base::out | std::ios_base::app);
        }
        else
        {
            logFile.open(logFileName, std::ios_base::out | std::ios_base::trunc);
        }
        if (!logFile.is_open())
        {
            throw(std::runtime_error(_TR("Failed to open log file")+" "+logFileName));
        }
    }
}

//---------------------------------------------------------------
FileLogHandler::FileLogHandler(
        const std::string &logFile
    )
{
    logFileName=logFile;
    openFile();
}

//---------------------------------------------------------------
void FileLogHandler::handler(
        const FmtAllocatedBufferChar &s
    )
{
    if(!logFile.is_open())
    {
        openFile();
    }
    if(logFile.is_open())
    {
        logFile << lib::toStringView(s) << std::endl;
    }
}

//---------------------------------------------------------------
FileLogHandler::~FileLogHandler()
{
    if (logFile.is_open())
    {
        logFile.close();
    }
}

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END
