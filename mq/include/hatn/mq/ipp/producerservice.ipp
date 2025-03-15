/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file mq/producerservice.ipp
  *
  *
  */

/****************************************************************************/

#ifndef HATNBMQPRODUCERSERVICE_IPP
#define HATNBMQPRODUCERSERVICE_IPP

#include <hatn/dataunit/visitors.h>

#include <hatn/mq/message.h>
#include <hatn/mq/producerservice.h>

HATN_MQ_NAMESPACE_BEGIN

namespace server {

//---------------------------------------------------------------

template <typename RequestT, typename MessageT>
validator::error_report ProducerMethodTraits<RequestT,MessageT>::validate(
        const common::SharedPtr<api::server::RequestContext<Request>>& request,
        const MessageT& msg
    ) const
{
    //! @todo implement mq message validation
    return validator::error_report{};
}

//---------------------------------------------------------------

template <typename RequestT, typename MessageT>
void ProducerMethodTraits<RequestT,MessageT>::exec(
        common::SharedPtr<api::server::RequestContext<Request>> request,
        api::server::RouteCb<Request> callback,
        common::SharedPtr<Message> msg
    ) const
{
    //! @todo implement mq message exec

    // fill db message fields

    // parse object

    // check it producer_pos outdated

    // save object and mq message in db

    // notify that mq message received

    // invoke callback
}

//---------------------------------------------------------------

} // namespace server

HATN_MQ_NAMESPACE_END

#endif // HATNBMQPRODUCERSERVICE_IPP
