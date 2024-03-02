/*
   Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    
  */

/****************************************************************************/
/*
    
*/
/** @file common/logger.h
  *
  *      Dracosha logger module.
  *
  */

/****************************************************************************/

#ifndef HATNLOGGERMODULE_H
#define HATNLOGGERMODULE_H

HATN_COMMON_NAMESPACE_BEGIN
        class LogModule;
    }
}
/**
 * @brief Template for debug module instance.
 * Example of use is below for global module
 */
template <typename T> class HATN_Log
 {
    public:

        //! Get module instance, create it if not exists
        static ::hatn::common::LogModule* i();

        //! Do extra initializations of the log record
        static ::hatn::common::LogRecord initRecord(::hatn::common::LogRecord record);

        //! Filter debug log by level
        static ::hatn::common::LogRecord df(uint8_t debugLevel=0);

        //! Filter debug log by context
        static ::hatn::common::LogRecord df(const char* context, uint8_t debugLevel=0);

        //! Filter debug log by tags
        static ::hatn::common::LogRecord df(const ::hatn::common::pmr::CStringVector& tags, uint8_t debugLevel=0);

        //! Filter debug log by context and tags
        static ::hatn::common::LogRecord df(const char* context, const ::hatn::common::pmr::CStringVector& tags, uint8_t debugLevel=0);

        //! Filter log by verbosity
        static ::hatn::common::LogRecord f(::hatn::common::LoggerVerbosity verbosity, const char* context=nullptr);

        //! Filter log by verbosity and tags
        static ::hatn::common::LogRecord f(::hatn::common::LoggerVerbosity verbosity, const ::hatn::common::pmr::CStringVector& tags);

        //! Filter log by verbosity and tags
        static ::hatn::common::LogRecord f(::hatn::common::LoggerVerbosity verbosity, const char* context, const ::hatn::common::pmr::CStringVector& tags);

    private:

        static std::atomic<::hatn::common::LogModule*> Module;
};

#endif // HATNLOGGERMODULE_H
