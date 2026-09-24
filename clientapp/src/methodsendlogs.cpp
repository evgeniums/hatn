/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientapp/methodsendlogs.cpp
  *
  * Routes to the active LogsProvider from the registry. Returns
  * LOGS_PROVIDER_NOT_CONFIGURED when no provider has been selected.
  *
  * The live client core log is always injected implicitly (first in the list).
  * Additional paths from send_logs_request::file_names are appended after it.
  */

#include <hatn/app/app.h>

#include <hatn/clientapp/clientapperror.h>
#include <hatn/clientapp/clientapp.h>
#include <hatn/clientapp/logsprovider.h>
#include <hatn/clientapp/bridgemethod.h>
#include <hatn/clientapp/systemservice.h>
#include <hatn/clientapp/methodsendlogs.h>

// Instantiate HDU template bodies for send_logs_request (declared in methodsendlogs.h).
#include <hatn/dataunit/ipp/syntax.ipp>

HATN_CLIENTAPP_NAMESPACE_BEGIN

//---------------------------------------------------------------

void MethodSendLogs::exec(
        common::SharedPtr<HATN_APP_NAMESPACE::AppEnv> env,
        common::SharedPtr<Context> ctx,
        Request request,
        Callback callback
    )
{
    HATN_CTX_SCOPE("sendlogs::exec")

    auto msg = request.message.as<send_logs_request::managed>();
    auto comments = std::string(msg->fieldValue(send_logs_request::comments));
    const auto& extraNames = msg->field(send_logs_request::file_names);

    auto* ca = ContextApp::clientApp(env, ctx);
    if (!ca)
    {
        callback(clientAppError(ClientAppError::LOGS_PROVIDER_NOT_CONFIGURED), {});
        return;
    }
    auto* provider = ca->logsProviderRegistry().activeProvider();
    if (!provider)
    {
        callback(clientAppError(ClientAppError::LOGS_PROVIDER_NOT_CONFIGURED), {});
        return;
    }

    // Build the file path list.
    // First: the live client core log (identified by .txt/.log extension).
    std::vector<std::string> filePaths;
    for (const auto& f : ca->app().listLogFiles())
    {
        lib::filesystem::path p{f};
        const auto ext = p.extension().string();
        if (ext == ".txt" || ext == ".log")
        {
            filePaths.push_back(f);
            break;
        }
    }
    // Then: additional paths provided by the caller (e.g. iOS/Android platform logs).
    for (size_t i = 0; i < extraNames.count(); ++i)
    {
        filePaths.emplace_back(extraNames.at(i).stringView());
    }

    HATN_CTX_DEBUG_RECORDS(1, "send logs",
        {"core_log_found",    static_cast<int>(!filePaths.empty())},
        {"extra_path_count",  static_cast<int>(extraNames.count())},
        {"comments_length",   static_cast<int>(comments.size())}
    )

    provider->sendLogs(
        std::move(ctx),
        std::move(comments),
        std::move(filePaths),
        [callback = std::move(callback)](const Error& ec)
        {
            callback(ec, Response{});
        }
    );
}

//---------------------------------------------------------------

std::string MethodSendLogs::messageType() const
{
    return send_logs_request::conf().name;
}

//---------------------------------------------------------------

MessageBuilderFn MethodSendLogs::messageBuilder() const
{
    return messageBuilderT<send_logs_request::managed>();
}

//---------------------------------------------------------------

HATN_CLIENTAPP_NAMESPACE_END
