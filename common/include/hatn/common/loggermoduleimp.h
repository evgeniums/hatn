/*
   Copyright (c) 2020 - current, Evgeny Sidorov (dracosha.com), All rights reserved.
    
    
  */

/****************************************************************************/
/*
    
*/
/** @file common/loggermoduleimp.h
  *
  *      Implementation of logger templates.
  *
  */

/****************************************************************************/

#ifndef HATNLOGGERMODULEIMPL_H
#define HATNLOGGERMODULEIMPL_H

template <typename T> ::hatn::common::LogModule* HATN_Log<T>::i()
{
    auto m=Module.load(std::memory_order_acquire);
    if (m==nullptr)
    {
        ::hatn::common::LogModule* tmp=nullptr;
        auto newModule=new ::hatn::common::LogModule(T::name);
        if (!Module.compare_exchange_strong(tmp,newModule,std::memory_order_relaxed,std::memory_order_relaxed))
        {
            delete newModule;
        }
        else
        {
            auto module=Module.load(std::memory_order_acquire);
            auto registeredModule=::hatn::common::LogModuleTable::registerModule(module);
            if (registeredModule!=module)
            {
                Module.exchange(registeredModule,std::memory_order_relaxed);
                delete newModule;
            }
        }

        std::atomic_thread_fence(std::memory_order_release);
        m=Module.load(std::memory_order_relaxed);
    }
    return m;
}

template <typename T> ::hatn::common::LogRecord HATN_Log<T>::initRecord(::hatn::common::LogRecord record)
{
    if (record)
    {
        record->logInCallerThread=i()->logInCallerThread();
    }
    return record;
}

template <typename T> ::hatn::common::LogRecord HATN_Log<T>::df(uint8_t debugLevel)
{
    return initRecord(i()->dbgFilter(debugLevel));
}

template <typename T> ::hatn::common::LogRecord HATN_Log<T>::df(const char* context, uint8_t debugLevel)
{
    return initRecord(i()->dbgFilter(context,debugLevel));
}

template <typename T> ::hatn::common::LogRecord HATN_Log<T>::df(const ::hatn::common::pmr::CStringVector& tags, uint8_t debugLevel)
{
    return initRecord(i()->dbgFilter(tags,debugLevel));
}

template <typename T> ::hatn::common::LogRecord HATN_Log<T>::df(const char* context, const ::hatn::common::pmr::CStringVector& tags, uint8_t debugLevel)
{
    return initRecord(i()->dbgFilter(context,tags,debugLevel));
}

template <typename T> ::hatn::common::LogRecord HATN_Log<T>::f(::hatn::common::LoggerVerbosity verbosity, const char* context)
{
    return initRecord(i()->filter(verbosity,context));
}

template <typename T> ::hatn::common::LogRecord HATN_Log<T>::f(::hatn::common::LoggerVerbosity verbosity, const ::hatn::common::pmr::CStringVector& tags)
{
    return initRecord(i()->filter(verbosity,tags));
}

template <typename T> ::hatn::common::LogRecord HATN_Log<T>::f(::hatn::common::LoggerVerbosity verbosity, const char* context, const ::hatn::common::pmr::CStringVector& tags)
{
    return initRecord(i()->filter(verbosity,context,tags));
}

template <typename T> std::atomic<::hatn::common::LogModule*> HATN_Log<T>::Module(nullptr);

#endif // HATNLOGGERMODULEIMPL_H
