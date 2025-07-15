/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientapp/systemcaevent.h
  *
  */

/****************************************************************************/

#ifndef HATNSYSTEMCAEVENT_H
#define HATNSYSTEMCAEVENT_H

#include <hatn/clientapp/eventdispatcher.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

class HATN_CLIENTAPP_EXPORT RequestSystemCaList : public Event
{
    public:

        constexpr static const char* Category="system";
        constexpr static const char* Name="request_system_ca_list";

        RequestSystemCaList()
        {
            category=Category;
            event=Name;
        }
};

HATN_CLIENTAPP_NAMESPACE_END

#endif // HATNSYSTEMCAEVENT_H
