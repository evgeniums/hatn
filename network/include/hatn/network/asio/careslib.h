/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/

/** @file network/asio/careslib.h
  *
  *   DNS c-ares library initialization and cleanup
  *
  */

/****************************************************************************/

#ifndef HATNCARESLIB_H
#define HATNCARESLIB_H

#if defined(ANDROID) || defined(__ANDROID__)
#include <jni.h>
#endif

#include <hatn/common/pmr/allocatorfactory.h>

#include <hatn/network/network.h>

HATN_NETWORK_NAMESPACE_BEGIN

//! c-ares library initialization and cleanup
class HATN_NETWORK_EXPORT CaresLib
{
    public:

        CaresLib()=delete;
        ~CaresLib()=delete;
        CaresLib(const CaresLib&)=delete;
        CaresLib(CaresLib&&) =delete;
        CaresLib& operator=(const CaresLib&)=delete;
        CaresLib& operator=(CaresLib&&) =delete;

        //! Initialize c-ares library

        /**
         * @brief Init the c-ares library.
         * @param allocatorFactory pmr allocator factory.
         * @return Initialization status.
         *
         * @note On Android the library must be also initialized using JNI by
         * passing Connectivity Manager from java to ares_library_init_android().
         * This can take place any time after calling init().
         */
        static Error init(
            const common::pmr::AllocatorFactory* allocatorFactory=nullptr
        );

        //! Cleanup library
        static void cleanup();

        //! Get allocator factory to use in resolver
        inline static const common::pmr::AllocatorFactory* allocatorFactory() noexcept
        {
            return m_allocatorFactory;
        }

#if defined(ANDROID) || defined(__ANDROID__)

        static Error initAndroid(jobject connectivityManager);

        static bool isAndroidInitialized();

        static void initJvm(JavaVM *jvm);

#endif

    private:

        static const common::pmr::AllocatorFactory *m_allocatorFactory;
};

HATN_NETWORK_NAMESPACE_END

#endif // HATNCARESLIB_H
