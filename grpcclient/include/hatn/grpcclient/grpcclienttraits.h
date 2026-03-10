/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file grpcclient/grpcclienttraits.h
  *
  */

/****************************************************************************/

#ifndef HATNGRPCCLIENTTRAITS_H
#define HATNGRPCCLIENTTRAITS_H

#include <hatn/api/client/defaulttraits.h>
#include <hatn/grpcclient/grpcclientdefs.h>

HATN_GRPCCLIENT_NAMESPACE_BEGIN

struct DefaultGrpcClientTraits : public HATN_API_NAMESPACE::client::DefaultClientTraits
{
    using MessageType=HATN_API_NAMESPACE::Message<hana::false_,HATN_DATAUNIT_NAMESPACE::WireData>;
};

HATN_GRPCCLIENT_NAMESPACE_END

#endif // HATNGRPCCLIENTTRAITS_H
