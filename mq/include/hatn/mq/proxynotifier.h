/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file mq/producerservice.h
  *
  *
  */

/****************************************************************************/

#ifndef HATNBMQPROXYNOTIFIER_H
#define HATNBMQPROXYNOTIFIER_H

#include <hatn/common/taskcontext.h>

#include <hatn/dataunit/syntax.h>

#include <hatn/mq/mq.h>

HATN_MQ_NAMESPACE_BEGIN

template <typename LocalStorageT, typename ServerT, typename SchedulerT>
struct Traits
{
    using LocalStorage=LocalStorageT;
    using Server=ServerT;
    using Scheduler=SchedulerT;
};

template <typename Traits>
class Notifier : public common::TaskSubcontext
{
    public:

        using LocalStorage=typename Traits::LocalStorage;
        using Server=typename Traits::Server;
        using Scheduler=typename Traits::Scheduler;
};

HATN_MQ_NAMESPACE_END

#endif // HATNBMQPROXYNOTIFIER_H
