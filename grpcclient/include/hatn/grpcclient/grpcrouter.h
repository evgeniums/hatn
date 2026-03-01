/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file grpcclientt/grpcrouter.h
  *
  */

/****************************************************************************/

#ifndef HATNGRPCROUTER_H
#define HATNGRPCROUTER_H

#include <hatn/grpcclient/grpcclientdefs.h>
#include <hatn/api/client/tlsclientconfig.h>

HATN_GRPCCLIENT_NAMESPACE_BEGIN

namespace api=HATN_API_NAMESPACE;
namespace clientapi=HATN_API_NAMESPACE::client;

class Router : public clientapi::TlsClientConfig
{
    public:

        using clientapi::TlsClientConfig::TlsClientConfig;
};

HATN_GRPCCLIENT_NAMESPACE_END

#endif // HATNGRPCROUTER_H
