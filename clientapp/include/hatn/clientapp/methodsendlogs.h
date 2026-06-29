/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientapp/methodsendlogs.h
  *
  * SystemService method: send application logs to developers.
  *
  * Request message fields:
  *   file_names  — repeated string, optional: additional log file paths in the
  *                 local filesystem (e.g. iOS/Android platform-specific logs
  *                 provided by Swift/Kotlin). The live client core log is always
  *                 included implicitly by the method — callers must not add it.
  *   comments    — string, optional (user-supplied comments)
  */

/****************************************************************************/

#ifndef HATNMETHODSENDLOGS_H
#define HATNMETHODSENDLOGS_H

#include <hatn/dataunit/syntax.h>

#include <hatn/clientapp/bridgemethod.h>

HATN_CLIENTAPP_NAMESPACE_BEGIN

// Request message schema for MethodSendLogs.
// Keep this in the header so callers (e.g. desktop DeveloperSettings controller)
// can create the message without depending on the anonymous-namespace version in
// the .cpp.
HDU_UNIT(send_logs_request,
    HDU_REPEATED_FIELD(file_names,TYPE_STRING,1)
    HDU_FIELD(comments,TYPE_STRING,2)
)

class SystemService;

class HATN_CLIENTAPP_EXPORT MethodSendLogs : public BridgeMethod<SystemService>
{
    public:

        constexpr static const char* Name = "send_logs";

        MethodSendLogs(SystemService* service) : BridgeMethod<SystemService>(service, Name)
        {}

        virtual void exec(
            common::SharedPtr<HATN_APP_NAMESPACE::AppEnv> env,
            common::SharedPtr<Context> ctx,
            Request request,
            Callback callback
        ) override;

        virtual std::string messageType() const override;

        virtual MessageBuilderFn messageBuilder() const override;
};

HATN_CLIENTAPP_NAMESPACE_END

#endif // HATNMETHODSENDLOGS_H
