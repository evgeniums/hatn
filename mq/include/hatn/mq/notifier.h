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

#ifndef HATNBMQNOTIFIER_H
#define HATNBMQNOTIFIER_H

#include <hatn/common/objecttraits.h>
#include <hatn/common/taskcontext.h>

#include <hatn/mq/mq.h>
#include <hatn/mq/message.h>

HATN_MQ_NAMESPACE_BEGIN

namespace client {

template <typename Traits>
class Notifier : public common::WithTraits<Traits>,
                 public common::TaskSubcontext
{
    public:

        using common::WithTraits<Traits>::WithTraits;

        template <typename ContextT, typename CallbackT, typename MessageT>
        void notify(
                common::SharedPtr<ContextT> ctx,
                CallbackT callback,
                lib::string_view topic,
                common::SharedPtr<MessageT> msg,
                MessageStatus status,
                const std::string& errMsg
            )
        {
            this->traits().notify(std::move(ctx),std::move(callback),topic,std::move(msg),status,errMsg);
        }
};

}

HATN_MQ_NAMESPACE_END

#endif // HATNBMQNOTIFIER_H
