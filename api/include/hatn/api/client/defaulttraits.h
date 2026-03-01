/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/client/defaulttraits.h
  *
  */

/****************************************************************************/

#ifndef HATNAPICLIENTTRAITS_H
#define HATNAPICLIENTTRAITS_H

#include <hatn/logcontext/context.h>

#include <hatn/api/api.h>
#include <hatn/api/requestunit.h>
#include <hatn/api/message.h>

HATN_API_NAMESPACE_BEGIN

namespace client {

using ClientTaskContext=HATN_LOGCONTEXT_NAMESPACE::TaskLogContext;

struct DefaultClientTraits
{
    using Context=ClientTaskContext;
    using MessageType=Message<hana::true_,HATN_DATAUNIT_NAMESPACE::WireData>;
    using RequestUnit=api::RequestManaged;
};

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPICLIENTTRAITS_H
