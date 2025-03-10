/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file mq/syncclient.h
  *
  *
  */

/****************************************************************************/

#ifndef HATNBMQCONSUMERCLIENT_H
#define HATNBMQCONSUMERCLIENT_H

#include <hatn/common/taskcontext.h>

#include <hatn/dataunit/syntax.h>

#include <hatn/mq/mq.h>

HATN_MQ_NAMESPACE_BEGIN

template <typename LocalStorageT, typename ServerT, typename SchedulerT, typename NotifierT>
struct Traits
{
    using LocalStorage=LocalStorageT;
    using Server=ServerT;
    using Scheduler=SchedulerT;
    using Notifier=NotifierT;
};

template <typename Traits>
class SyncClient : public common::TaskSubcontext
{
    public:

        using LocalStorage=typename Traits::LocalStorage;
        using Server=typename Traits::Server;
        using Scheduler=typename Traits::Scheduler;
        using Notifier=typename Traits::Notifier;
};

HATN_MQ_NAMESPACE_END

#endif // HATNBMQSYNCCLIENT_H
