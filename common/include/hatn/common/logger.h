/*
    Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file common/logger.h
  *
  *      Dracosha logger.
  *
  */

/****************************************************************************/

#ifndef HATNLOGGER_H
#define HATNLOGGER_H

#include <iostream>
#include <fstream>

#include <atomic>

#include <hatn/common/common.h>
#include <hatn/common/format.h>
#include <hatn/common/elapsedtimer.h>

#include <hatn/common/metautils.h>
#include <hatn/common/locker.h>
#include <hatn/common/fixedbytearray.h>
#include <hatn/common/pmr/pmrtypes.h>
#include <hatn/common/sharedptr.h>
#include <hatn/common/managedobject.h>
#include <hatn/common/weakptr.h>
#include <hatn/common/threadwithqueue.h>

#ifdef DEBUG
#undef DEBUG
#endif

HATN_COMMON_NAMESPACE_BEGIN

enum class LoggerVerbosity : int
{
    DEFAULT=-1,
    NONE=0,
    FATAL=1,
    ERR=2,
    WARNING=3,
    INFO=4,
    DEBUG=5
};

class LogModule;

#ifndef LOG_RECORD_BUCKET_COUNT
constexpr const int LogRecordBucketCount=4096;
#else
constexpr const int LogRecordBucketCount=LOG_RECORD_BUCKET_COUNT;
#endif

/**
 * @brief The LogModule class
 *
 * Thread safe - the logging is blocked when the module is being configured
 */
class HATN_COMMON_EXPORT LogModule final
{
    public:

        //! Log record
        class Log : public EnableManaged<Log>
        {
            public:

                ::hatn::common::LoggerVerbosity verbosity;
                const char* name;
                const char* context;
                const char* tag;
                uint8_t debugLevel;
                bool trace;
                bool logInCallerThread;

                Log(const char* name) noexcept :  verbosity(::hatn::common::LoggerVerbosity::NONE),name(name),context(nullptr),tag(nullptr),debugLevel(0),trace(false),logInCallerThread(false)
                {}
        };

        //! Ctor
        LogModule(const char* name) noexcept;

        //! Get module name
        const char* name() const noexcept;

        //! Filter debug log by level
        SharedPtr<LogModule::Log> dbgFilter(uint8_t debugLevel=0) const;

        //! Filter debug log by context
        SharedPtr<LogModule::Log> dbgFilter(const char* context, uint8_t debugLevel=0) const;

        //! Filter debug log by tags
        SharedPtr<LogModule::Log> dbgFilter(const pmr::CStringVector& tags, uint8_t debugLevel=0) const;

        //! Filter debug log by context and tags
        SharedPtr<LogModule::Log> dbgFilter(const char* context, const pmr::CStringVector& tags, uint8_t debugLevel=0) const;

        //! Filter log by verbosity and context
        SharedPtr<LogModule::Log> filter(::hatn::common::LoggerVerbosity verbosity, const char* context=nullptr) const;

        //! Filter log by verbosity and tags
        SharedPtr<LogModule::Log> filter(::hatn::common::LoggerVerbosity verbosity, const pmr::CStringVector& tags) const;

        //! Filter log by verbosity and context and tags
        SharedPtr<LogModule::Log> filter(::hatn::common::LoggerVerbosity verbosity, const char* context, const pmr::CStringVector& tags) const;

        /**
         * @brief Configure module
         */
        void configure(
            ::hatn::common::LoggerVerbosity verbosity,
            int debugLevel,
            std::vector<std::string>&& contexts,
            std::vector<std::string>&& tags
        );

        inline bool logInCallerThread() const noexcept
        {
            return m_logInCallerThread;
        }

    private:

        //! Create log record
        static inline SharedPtr<LogModule::Log> createLogRecord(const char* name);

        inline bool checkVerbosity(::hatn::common::LoggerVerbosity verbosity) const noexcept;
        inline bool checkDebugLevel(int level) const noexcept;
        inline bool checkContext(const char* context) const noexcept;
        inline bool checkTags(const pmr::CStringVector& tags, const char** tag) const noexcept;

        FixedByteArrayThrow16 m_name;

        ::hatn::common::LoggerVerbosity m_verbosity;
        int m_debugLevel;
        std::vector<std::string> m_contexts;
        std::vector<std::string> m_tags;        

        std::atomic<bool> m_lock;

        bool m_logInCallerThread;
};
using LogRecord=SharedPtr<LogModule::Log>;

//! Table of debug modules
class HATN_COMMON_EXPORT LogModuleTable final
{
    public:

        static LogModule* registerModule(LogModule* module);
        static LogModule* findModule(const char* name) noexcept;
        static std::vector<LogModule*> allModules() noexcept;

        static inline pmr::polymorphic_allocator<LogModule::Log>& logRecordAllocator() noexcept
        {
            return *LogModuleTable::instance()->m_logRecordAllocator;
        }
        static inline void setLogRecordAllocator(std::shared_ptr<pmr::polymorphic_allocator<LogModule::Log>> allocator) noexcept
        {
            LogModuleTable::instance()->m_logRecordAllocator=std::move(allocator);
        }

        static inline pmr::memory_resource* bufferMemoryResource() noexcept
        {
            return LogModuleTable::instance()->m_bufferMemoryResource;
        }
        static inline void setBufferMemoryResource(pmr::memory_resource* resource) noexcept
        {
            LogModuleTable::instance()->m_bufferMemoryResource=resource;
        }

        static inline TimeDuration elapsed() noexcept
        {
            return LogModuleTable::instance()->m_elapsedTimer.elapsed();
        }

        static LogModuleTable* instance() noexcept;
        void reset() noexcept;

    private:

        LogModule* doRegisterModule(LogModule* module);
        LogModule* doFindModule(const char* name) noexcept;

        LogModuleTable();
        ~LogModuleTable()=default;
        LogModuleTable(const LogModuleTable&)=delete;
        LogModuleTable(LogModuleTable&&) =delete;
        LogModuleTable& operator=(const LogModuleTable&)=delete;
        LogModuleTable& operator=(LogModuleTable&&) =delete;

        MutexLock m_locker;
        std::map<std::string,LogModule*> m_table;
        std::shared_ptr<pmr::polymorphic_allocator<LogModule::Log>> m_logRecordAllocator;
        pmr::memory_resource* m_bufferMemoryResource;
        ElapsedTimer m_elapsedTimer;
};

class Logger_p;

#ifndef LOG_QUEUE_DEPTH
constexpr const int LogQueueDepth=1024;
#else
constexpr const int LogQueueDepth=LOG_QUEUE_DEPTH;
#endif

//!  Dracosha logger
class HATN_COMMON_EXPORT Logger final
{
    public:

        typedef std::function<void (
            const FmtAllocatedBufferChar &s
        )> OutputHandler;

        //! Constructor
        Logger(
            const LogRecord& log
        );

        //! Get verbosity name
        static const char* verbosityToString(::hatn::common::LoggerVerbosity verbosity);
        //! Get verbosity level by name
        static ::hatn::common::LoggerVerbosity stringToVerbosity(const char* name);

        //! Destructor
        ~Logger();

        Logger(const Logger&)=delete;
        Logger(Logger&&) =delete;
        Logger& operator=(const Logger&)=delete;
        Logger& operator=(Logger&&) =delete;

        //! Get logger thread
        static ThreadWithQueue<TaskInlineContext<FmtAllocatedBufferChar>>* loggerThread(bool reset=false,
                                                                                        pmr::memory_resource* memResource=pmr::get_default_resource());

        static bool isSeparateThread() noexcept;

        //! Set default log verbosity
        static void setDefaultVerbosity(::hatn::common::LoggerVerbosity verbosity) noexcept;
        //! Get log verbosity
        static ::hatn::common::LoggerVerbosity defaultVerbosity() noexcept;

        //! Set default debug level
        static void setDefaultDebugLevel(int debugLevel=0) noexcept;
        //! Get default debug level
        static int defaultDebugLevel() noexcept;

        /**
         * @brief Configure log modules
         * @param modules List of configuration strings per module
         * @return Error string if parsing failed or empty string if parsing succeeded
         */
        static std::string configureModules(
            const std::vector<std::string>& modules
        );
        //! Reset all log modules: unset debug, clear contexts and tags
        static void resetModules();

        //! Enable tracing of fatal messages
        static void setFatalTracing(bool enable) noexcept;

        //! Enable appending logs to log files on restart
        static void setAppendLogFile(bool enable) noexcept;

        //! Enable logging to std::cerr additionally to the log handler
        static void setLogToStd(bool enable) noexcept;

        //! Return self reference
        inline Logger& operator () () noexcept
        {
            return *this;
        }

        //! Format and append string
        template <typename ... Args> void append(const char* format, Args&&... args)
        {
            fmt::format_to(std::back_inserter(*buffer),format,std::forward<Args>(args)...);
        }

        //! Format and append string and return self reference
        template <typename ... Args> Logger& operator () (const char* format, Args&&... args)
        {
            append(format,std::forward<Args>(args)...);
            return *this;
        }

        //! Stream data to buffer
        template <typename T> inline Logger& operator << (T&&t)
        {
            append("{}",std::forward<T>(t));
            return *this;
        }

        /**
         * @brief Set handler for output of log text
         * @param handler The handler where to send log messages
         *
         * \attention Not thread safe! Set the output handler on the program start only.
         */
        static void setOutputHandler(OutputHandler handler) noexcept;
        /**
         * @brief Set handler for output fatal log text
         * @param handler The handler where to send fatal messages
         *
         * \attention Not thread safe! Set the fatal log handler on the program start only.
         */
        static void setFatalLogHandler(OutputHandler handler) noexcept;

        /**
         * @brief Start logger
         * @param logInSeparateThread Enable logging in separate thread
         */
        static void start(bool logInSeparateThread=true);

        /**
         * @brief Stop logger
         */
        static void stop();

        /**
         * @brief Check if logger is running
         */
        static bool isRunning() noexcept;

    private:

        bool fatal;
        bool trace;
        std::ostream* stdStream;
        bool forceLogToStdInCallerThread;

        TaskInlineContext<FmtAllocatedBufferChar>* task;
        FmtAllocatedBufferChar  inlineBuffer;
        FmtAllocatedBufferChar*  buffer;
};

class FileLogHandler_p;
//! Standard log handler that prints to file
class HATN_COMMON_EXPORT FileLogHandler
{
    public:

        //! Constructor
        FileLogHandler(
            const std::string& logFile
        );

        //! Destructor
        virtual ~FileLogHandler();

        FileLogHandler(const FileLogHandler&)=delete;
        FileLogHandler(FileLogHandler&&) =delete;
        FileLogHandler& operator=(const FileLogHandler&)=delete;
        FileLogHandler& operator=(FileLogHandler&&) =delete;

        //! Debug handler
        void handler(
            const FmtAllocatedBufferChar &s
        );

    private:

        void openFile();

        std::string logFileName;
        std::ofstream logFile;
};

//---------------------------------------------------------------
    HATN_COMMON_NAMESPACE_END

#include <hatn/common/loggermodule.h>

#define _LOG_CHECK_NAME(Name) \
    static_assert(#Name!=nullptr,"You must set log module name"); \
    static_assert(::hatn::common::CStrLength(#Name)>0,"Log module name cannot be empty"); \
    static_assert(::hatn::common::CStrLength(#Name)<=15,"Length of log module name cannot exceed 15"); \

//! Use this macro to declare debug module in one compilation unit/library/DLL
/**
  Note that macro MUST be put in global namespace
*/
#define DECLARE_LOG_MODULE(Name) \
    _LOG_CHECK_NAME(Name) \
    struct HATN_Log_##Name \
    { \
        static constexpr const char* name=#Name; \
    }; \
    template class HATN_Log<HATN_Log_##Name>;  \
    static auto dummy##Name=HATN_Log<HATN_Log_##Name>::i();

#if defined(_WIN32) && defined(__GNUC__)
    #define DECLARE_LOG_MODULE_EXTERN(Name,ExportAttr) template class ExportAttr HATN_Log<HATN_Log_##Name>;
#else
    #define DECLARE_LOG_MODULE_EXTERN(Name,ExportAttr) extern template class HATN_Log<HATN_Log_##Name>;
#endif

//! Use this macro to declare exported debug module
/**
  Note that macro MUST be put in global namespace
*/
#define DECLARE_LOG_MODULE_EXPORT(Name,ExportAttr) \
    _LOG_CHECK_NAME(Name) \
    struct ExportAttr HATN_Log_##Name \
    { \
        static constexpr const char* name=#Name; \
    }; \
    DECLARE_LOG_MODULE_EXTERN(Name,ExportAttr)

//! Example of declaring "global" debug module
DECLARE_LOG_MODULE_EXPORT(global,HATN_COMMON_EXPORT)

#if defined(_WIN32) && defined(__GNUC__)
    #define INIT_LOG_MODULE_INSTANTIATE(Name,ExportAttr) ;
#else
    #define INIT_LOG_MODULE_INSTANTIATE(Name,ExportAttr) template class ExportAttr HATN_Log<HATN_Log_##Name>;
#endif

//! Instantiate log module, put it in cpp file of the unit where you declare log module
#define INIT_LOG_MODULE(Name,ExportAttr) \
    INIT_LOG_MODULE_INSTANTIATE(Name,ExportAttr) \
    static auto dummy##Name=HATN_Log<HATN_Log_##Name>::i();

//! Helper for calling debug module
#define LOG_MODULE(Name) HATN_Log<HATN_Log_##Name>
#define LOGGER_FILTER_AND_LOG(LogRec,Msg) if (!LogRec.isNull()) ::hatn::common::Logger(LogRec)<<Msg;
#define _DSC_EXPAND(x) x

#define _DSC_LOGGER_FILTER_AND_LOG_STREAM(LogRec,Msg) if (!LogRec.isNull()) ::hatn::common::Logger(LogRec)<<Msg;
#define _DSC_LOGGER_FILTER_AND_LOG_OP(LogRec,Dummy,Msg) if (!LogRec.isNull()) ::hatn::common::Logger(LogRec).append Msg;

#define _DSC_GET_4TH_ARG(arg1, arg2, arg3, arg4, ...) arg4
#define _DSC_MACRO_CHOOSER(...) \
    _DSC_EXPAND(_DSC_GET_4TH_ARG(__VA_ARGS__, \
                _DSC_LOGGER_FILTER_AND_LOG_OP, \
                _DSC_LOGGER_FILTER_AND_LOG_STREAM \
                ))
#define _HATN_MACRO_DO(...) _DSC_EXPAND(_DSC_MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__))

#define LOGGER_FILTER_CHECK(LogRec) (!LogRec.isNull())

#define HATN_LOG(Verbosity,Name,...) _HATN_MACRO_DO(LOG_MODULE(Name)::f(Verbosity),__VA_ARGS__)
#define HATN_LOG_TAGS(Verbosity,Name,Tags,...) _HATN_MACRO_DO(LOG_MODULE(Name)::f(Verbosity,Tags),__VA_ARGS__)
#define HATN_LOG_CONTEXT(Verbosity,Name,Context,...) _HATN_MACRO_DO(LOG_MODULE(Name)::f(Verbosity,Context),__VA_ARGS__)
#define HATN_LOG_CONTEXT_TAGS(Verbosity,Name,Context,Tags,...) _HATN_MACRO_DO(LOG_MODULE(Name)::f(Verbosity,Context,Tags),__VA_ARGS__)

#define HATN_INFO(Name,...) HATN_LOG(::hatn::common::LoggerVerbosity::INFO,Name,__VA_ARGS__)
#define HATN_INFO_TAGS(Name,Tags,...) HATN_LOG_TAGS(::hatn::common::LoggerVerbosity::INFO,Name,Tags,__VA_ARGS__)
#define HATN_INFO_CONTEXT(Name,Context,...) HATN_LOG_CONTEXT(::hatn::common::LoggerVerbosity::INFO,Name,Context,__VA_ARGS__)
#define HATN_INFO_CONTEXT_TAGS(Name,Context,Tags,...) HATN_LOG_CONTEXT_TAGS(::hatn::common::LoggerVerbosity::INFO,Name,Context,Tags,__VA_ARGS__)

#define HATN_WARN(Name,...) HATN_LOG(::hatn::common::LoggerVerbosity::WARNING,Name,__VA_ARGS__)
#define HATN_WARN_TAGS(Name,Tags,...) HATN_LOG_TAGS(::hatn::common::LoggerVerbosity::WARNING,Name,Tags,__VA_ARGS__)
#define HATN_WARN_CONTEXT(Name,Context,...) HATN_LOG_CONTEXT(::hatn::common::LoggerVerbosity::WARNING,Name,Context,__VA_ARGS__)
#define HATN_WARN_CONTEXT_TAGS(Name,Context,Tags,...) HATN_LOG_CONTEXT_TAGS(::hatn::common::LoggerVerbosity::WARNING,Name,Context,Tags,__VA_ARGS__)

#define HATN_ERROR(Name,...) HATN_LOG(::hatn::common::LoggerVerbosity::ERR,Name,__VA_ARGS__)
#define HATN_ERROR_TAGS(Name,Tags,...) HATN_LOG_TAGS(::hatn::common::LoggerVerbosity::ERR,Name,Tags,__VA_ARGS__)
#define HATN_ERROR_CONTEXT(Name,Context,...) HATN_LOG_CONTEXT(::hatn::common::LoggerVerbosity::ERR,Name,Context,__VA_ARGS__)
#define HATN_ERROR_CONTEXT_TAGS(Name,Context,Tags,...) HATN_LOG_CONTEXT_TAGS(::hatn::common::LoggerVerbosity::ERR,Name,Context,Tags,__VA_ARGS__)

#define HATN_FATAL(Name,...) HATN_LOG(::hatn::common::LoggerVerbosity::FATAL,Name,__VA_ARGS__)
#define HATN_FATAL_TAGS(Name,Tags,...) HATN_LOG_TAGS(::hatn::common::LoggerVerbosity::FATAL,Name,Tags,__VA_ARGS__)
#define HATN_FATAL_CONTEXT(Name,Context,...) HATN_LOG_CONTEXT(::hatn::common::LoggerVerbosity::FATAL,Name,Context,__VA_ARGS__)
#define HATN_FATAL_CONTEXT_TAGS(Name,Context,Tags,...) HATN_LOG_CONTEXT_TAGS(::hatn::common::LoggerVerbosity::FATAL,Name,Context,Tags,__VA_ARGS__)

#define HATN_DEBUG(Name,...) _HATN_MACRO_DO(LOG_MODULE(Name)::df(),__VA_ARGS__)
#define HATN_DEBUG_LVL(Name,Level,...) _HATN_MACRO_DO(LOG_MODULE(Name)::df(Level),__VA_ARGS__)
#define HATN_DEBUG_TAGS(Name,Tags,Level,...) _HATN_MACRO_DO(LOG_MODULE(Name)::df(Tags,Level),__VA_ARGS__)
#define HATN_DEBUG_CONTEXT(Name,Context,Level,...) _HATN_MACRO_DO(LOG_MODULE(Name)::df(Context,Level),__VA_ARGS__)
#define HATN_DEBUG_CONTEXT_TAGS(Name,Context,Tags,Level,...) _HATN_MACRO_DO(LOG_MODULE(Name)::df(Context,Tags,Level),__VA_ARGS__)

#define G_INFO(...)  HATN_INFO(global,__VA_ARGS__)
#define G_WARN(...)  HATN_WARN(global,__VA_ARGS__)
#define G_ERROR(...) HATN_ERROR(global,__VA_ARGS__)
#define G_FATAL(...) HATN_FATAL(global,__VA_ARGS__)
#define G_DEBUG(...) HATN_DEBUG(global,__VA_ARGS__)

#define HATN_DEBUG_ID_LVL(Name,Level,...) HATN_DEBUG_CONTEXT(Name,this->idStr(),Level,__VA_ARGS__)
#define HATN_DEBUG_ID(Name,...) HATN_DEBUG_ID_LVL(Name,0,__VA_ARGS__)
#define HATN_ERROR_ID(Name,...) HATN_ERROR_CONTEXT(Name,this->idStr(),__VA_ARGS__)
#define HATN_WARN_ID(Name,...) HATN_WARN_CONTEXT(Name,this->idStr(),__VA_ARGS__)

#define HATN_FORMAT(Format,...) _,(Format,__VA_ARGS__)

#define HATN_LOG_CHECK(Verbosity,Name) LOGGER_FILTER_CHECK(LOG_MODULE(Name)::f(Verbosity))
#define HATN_LOG_CHECK_TAGS(Verbosity,Name,Tags) LOGGER_FILTER_CHECK(LOG_MODULE(Name)::f(Verbosity,Tags))
#define HATN_LOG_CHECK_CONTEXT(Verbosity,Name,Context) LOGGER_FILTER_CHECK(LOG_MODULE(Name)::f(Verbosity,Context))
#define HATN_LOG_CHECK_CONTEXT_TAGS(Verbosity,Name,Context,Tags) LOGGER_FILTER_CHECK(LOG_MODULE(Name)::f(Verbosity,Context,Tags))

#define HATN_DEBUG_CHECK(Name) LOGGER_FILTER_CHECK(LOG_MODULE(Name)::df())
#define HATN_DEBUG_CHECK_LVL(Name,Level) LOGGER_FILTER_CHECK(LOG_MODULE(Name)::df(Level))
#define HATN_DEBUG_CHECK_TAGS(Name,Tags,Level) LOGGER_FILTER_CHECK(LOG_MODULE(Name)::df(Tags,Level))
#define HATN_DEBUG_CHECK_CONTEXT(Name,Context,Level) LOGGER_FILTER_CHECK(LOG_MODULE(Name)::df(Context,Level))
#define HATN_DEBUG_CHECK_CONTEXT_TAGS(Name,Context,Tags,Level) LOGGER_FILTER_CHECK(LOG_MODULE(Name)::df(Context,Tags,Level))

#endif // HATNLOGGER_H
