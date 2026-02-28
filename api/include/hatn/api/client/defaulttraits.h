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

#include <hatn/api/api.h>
#include <hatn/api/requestunit.h>

HATN_API_NAMESPACE_BEGIN

namespace client {

using ClientTaskContext=common::TaskContext;

struct DefaultClientTraits
{
    using Context=ClientTaskContext;
    using MessageBuf=HATN_DATAUNIT_NAMESPACE::WireData;
    using RequestUnit=api::RequestManaged;
};

} // namespace client

HATN_API_NAMESPACE_END

#endif // HATNAPICLIENTTRAITS_H
