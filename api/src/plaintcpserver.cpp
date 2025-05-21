/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/plaintcpserver.сpp
  *
  */

#include "hatn/api/server/serverrequest.h"
#include <hatn/api/server/plaintcpserver.h>

HATN_API_NAMESPACE_BEGIN

HATN_API_NAMESPACE_END

HATN_TASK_CONTEXT_DEFINE(HATN_API_SERVER_NAMESPACE::WithEnv<HATN_API_SERVER_NAMESPACE::BasicEnv>,BasicEnv)
HATN_TASK_CONTEXT_DEFINE(HATN_API_SERVER_NAMESPACE::Request<HATN_API_SERVER_NAMESPACE::BasicEnv>,Request)

HATN_TASK_CONTEXT_DEFINE(HATN_API_SERVER_NAMESPACE::PlainTcpConnection,PlainTcpConnection)
HATN_TASK_CONTEXT_DEFINE(HATN_API_SERVER_NAMESPACE::PlainTcpServer,TcpServer)

