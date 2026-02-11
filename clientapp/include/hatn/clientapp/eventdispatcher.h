/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientapp/eventdispatcher.h
  *
  */

/****************************************************************************/

#ifndef HATNCLIENTEVENTDISPATCHER_H
#define HATNCLIENTEVENTDISPATCHER_H

#include <hatn/app/eventdispatcher.h>
#include <hatn/clientapp/clientappdefs.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

using EventKey=HATN_APP_NAMESPACE::EventKey;
using Event=HATN_APP_NAMESPACE::Event;
using EventHandler=HATN_APP_NAMESPACE::EventHandler;
using EventSubscriptions=HATN_APP_NAMESPACE::EventSubscriptions;
using EventDispatcher=HATN_APP_NAMESPACE::EventDispatcher;

HATN_CLIENTAPP_NAMESPACE_END

#endif // HATNCLIENTEVENTDISPATCHER_H
