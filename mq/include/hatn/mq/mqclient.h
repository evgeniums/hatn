/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file mq/methods.h
  *
  *
  */

/****************************************************************************/

#ifndef HATNBMQMETHODS_H
#define HATNBMQMETHODS_H

#include <hatn/api/method.h>

#include <hatn/mq/mq.h>
#include <hatn/mq/message.h>

HATN_MQ_NAMESPACE_BEGIN

class MethodProducerPost : public api::Method
{
    public:

        MethodProducerPost() : api::Method(ApiPostMethod)
        {}
};

HATN_MQ_NAMESPACE_END

#endif // HATNBMQMETHODS_H
