/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.
    
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
      
*/

/****************************************************************************/
/*
    
*/
/** @file dataunit/updatenotifier.h
  *
  *  Notifier of data unit updates
  *
  */

/****************************************************************************/

#ifndef HATNDATAUNITNOTIFIER_H
#define HATNDATAUNITNOTIFIER_H

#include <functional>

#include <hatn/common/sharedptr.h>
#include <hatn/dataunit/unit.h>

HATN_DATAUNIT_NAMESPACE_BEGIN

struct UpdateInvoker{};

//! @todo Implement notifier.
class UpdateNotifier
{
    public:

        void notify(UpdateInvoker* invoker,Field* field);
        void beginGroupUpdate(UpdateInvoker* invoker,Unit* unit,size_t groupID);
        void endGroupUpdate(UpdateInvoker* invoker,Unit* unit,size_t groupID);
};

HATN_DATAUNIT_NAMESPACE_END

#endif // HATNDATAUNITNOTIFIER_H
