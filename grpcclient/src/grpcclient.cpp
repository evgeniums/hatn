/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

#include <grpcpp/version_info.h>

#include <hatn/grpcclient/grpcclient.h>

HATN_GRPCCLIENT_NAMESPACE_BEGIN

const char* grpcVersionString() noexcept
{
    return GRPC_CPP_VERSION_STRING;
}

HATN_GRPCCLIENT_NAMESPACE_END
