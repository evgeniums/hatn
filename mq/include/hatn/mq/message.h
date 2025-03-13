/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file mq/message.h
  *
  *
  */

/****************************************************************************/

#ifndef HATNBMQMESSAGE_H
#define HATNBMQMESSAGE_H

#include <hatn/dataunit/syntax.h>

#include <hatn/db/model.h>

#include <hatn/mq/mq.h>

HATN_MQ_NAMESPACE_BEGIN

enum class Operation : uint8_t
{
    Create=0,
    Update=1,
    Delete=2
};

HDU_UNIT(message_content,
    HDU_FIELD(object,TYPE_DATAUNIT,1)
    HDU_FIELD(notification,TYPE_DATAUNIT,2)
)

HDU_UNIT(message,
    HDU_FIELD(pos,TYPE_OBJECT_ID,1)
    HDU_FIELD(producer_pos,TYPE_OBJECT_ID,2)
    HDU_FIELD(producer,TYPE_OBJECT_ID,3)
    HDU_FIELD(operation,HDU_TYPE_ENUM(Operation),4,true)
    HDU_FIELD(object_id,TYPE_STRING,5,true)
    HDU_FIELD(object_type,TYPE_STRING,6)
    HDU_FIELD(content,message_content::TYPE,7)    
)

HATN_DB_UNIQUE_INDEX(messageProducerIdx,message::producer)
HATN_DB_UNIQUE_INDEX(messagePosIdx,message::producer,message::pos)

HATN_DB_INDEX(objectIdTypeIdx,message::object_id,message::object_type,message::producer)
HATN_DB_INDEX(objectIdOpIdx,message::object_id,message::operation,message::producer)

HATN_DB_INDEX(objectTypeIdx,message::object_type,message::producer)

/** @todo Use message validator
 *
 * object_type must be not empty for Operation::Create
 * object.object must be set for Operation::Create and Operation::Update at producer
 * producer_pos and producer must be set at producer
 */

HATN_MQ_NAMESPACE_END

#endif // HATNBMQMESSAGE_H
