/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file clientapp/methodsyncdatetime.сpp
  *
  */

#include <hatn/dataunit/syntax.h>

#include <hatn/clientapp/methodsyncdatetime.h>

#include <hatn/dataunit/ipp/syntax.ipp>

HATN_CLIENTAPP_NAMESPACE_BEGIN

HDU_UNIT(sync_datetime,
    HDU_FIELD(utc,TYPE_DATETIME,1)
    HDU_FIELD(local_timezone_offset_minutes,TYPE_INT32,2)
)

//---------------------------------------------------------------

void MethodSyncDateTime::exec(
        common::SharedPtr<HATN_APP_NAMESPACE::AppEnv> env,
        common::SharedPtr<Context> ctx,
        Request request,
        Callback callback
    )
{
    HATN_CTX_SCOPE("syncdatetime::exec")

    auto msg=request.message.as<sync_datetime::managed>();
    std::ignore=msg;
    std::ignore=ctx;
    std::ignore=env;
    //! @todo critical: implement datetime correction and publish event
    callback(Error{},Response{});
}

//---------------------------------------------------------------

std::string MethodSyncDateTime::messageType() const
{
    return messageTypeT<sync_datetime::conf>();
}

//---------------------------------------------------------------

MessageBuilderFn MethodSyncDateTime::messageBuilder() const
{
    return messageBuilderT<sync_datetime::managed>();
}

//---------------------------------------------------------------

HATN_CLIENTAPP_NAMESPACE_END

