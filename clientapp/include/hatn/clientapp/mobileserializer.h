/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientapp/mobileserializer.h
  *
  *     MobileMessageSerializer / setMobileMessageSerializer - an app-installable hook over the
  *     JSON a mobile app receives for a bridge response or an event message. Kept out of
  *     mobileapp.h for the same reason mobileusererror.h is: mobileapp.h is on the C bridge
  *     include path, which has no dataunit headers.
  */

/****************************************************************************/

#ifndef HATNMOBILEAPPSERIALIZER_H
#define HATNMOBILEAPPSERIALIZER_H

#include <functional>
#include <string>

#include <hatn/dataunit/unit.h>

#include <hatn/clientapp/mobileapp.h>

HATN_CLIENTAPP_MOBILE_NAMESPACE_BEGIN

/**
 * @brief Hook that may produce the JSON of a message on its way to a mobile app.
 *
 * Called for the message of every bridge response and of every event delivered to a mobile app,
 * with the message's type name and the unit itself. Returns true when it wrote the JSON to @a json,
 * false to let the default serialization (Unit::toString()) run. The unit is passed non-const so a
 * hook may adjust it for the duration of the call (for example fill a field that only the mobile
 * JSON carries); it must leave the unit as it found it, since the same object may be used again by
 * the code that produced it.
 *
 * Called on the thread that produces the response or publishes the event. Intended to be installed
 * once, early, by app startup code; not thread-safe to install concurrently with serialization.
 */
using MobileMessageSerializer=std::function<bool (const std::string& messageTypeName,
                                                  HATN_DATAUNIT_NAMESPACE::Unit& unit,
                                                  std::string& json)>;

//! Installs the process-wide serializer hook. A default hook that always returns false is installed
//! initially, so the JSON is exactly Unit::toString() until an app installs one.
HATN_CLIENTAPP_EXPORT void setMobileMessageSerializer(MobileMessageSerializer serializer);

//! The JSON of @a unit for a mobile app: the installed hook's output, or Unit::toString().
HATN_CLIENTAPP_EXPORT std::string serializeMobileMessage(const std::string& messageTypeName,
                                                         HATN_DATAUNIT_NAMESPACE::Unit& unit);

HATN_CLIENTAPP_MOBILE_NAMESPACE_END

#endif // HATNMOBILEAPPSERIALIZER_H
