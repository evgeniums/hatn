/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/tcpserver.сpp
  *
  */

#include "hatn/api/server/request.h"
#include <hatn/api/server/plaintcpserver.h>

HATN_API_NAMESPACE_BEGIN

HATN_API_NAMESPACE_END

HATN_TASK_CONTEXT_DEFINE(HATN_API_NAMESPACE::server::WithEnv<HATN_API_NAMESPACE::server::BasicEnv>,BasicEnv)
HATN_TASK_CONTEXT_DEFINE(HATN_API_NAMESPACE::server::Request<HATN_API_NAMESPACE::server::BasicEnv>,Request)

HATN_TASK_CONTEXT_DEFINE(HATN_API_NAMESPACE::server::PlainTcpConnection,PlainTcpConnection)
HATN_TASK_CONTEXT_DEFINE(HATN_API_NAMESPACE::server::PlainTcpServer,TcpServer)

