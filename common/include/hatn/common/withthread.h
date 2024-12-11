/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/** @file common/withthread.h
  *
  *     Base classes for objects with thread.
  *
  */

/****************************************************************************/

#ifndef HATNWITHTHREAD_H
#define HATNWITHTHREAD_H

#include <hatn/common/common.h>

HATN_COMMON_NAMESPACE_BEGIN

class Thread;

//! Base class for objects with thread
class WithThread
{
    public:

        //! Ctor
        WithThread(
            Thread* thread
        ) noexcept : m_thread(thread)
        {}

        //! Get thread
        inline common::Thread* thread() const noexcept
        {
            return m_thread;
        }

    private:

        Thread* m_thread;
};

//---------------------------------------------------------------
HATN_COMMON_NAMESPACE_END

#endif // HATNOBJECTID_H
