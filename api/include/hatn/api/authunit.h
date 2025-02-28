/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/authunit.h
  *
  */

/****************************************************************************/

#ifndef HATNAPIAUTHUNIT_H
#define HATNAPIAUTHUNIT_H

#include <hatn/dataunit/syntax.h>

#include <hatn/api/api.h>
#include <hatn/api/apiconstants.h>

HATN_API_NAMESPACE_BEGIN

HDU_UNIT(auth,
    HDU_FIELD(protocol,HDU_TYPE_FIXED_STRING(AuthProtocolNameLengthMax),1)
    HDU_FIELD(version,TYPE_UINT8,2,false,1)
    HDU_FIELD(content,TYPE_BYTES,3)
)

using AuthManaged=auth::managed;

HATN_API_NAMESPACE_END

#endif // HATNAPIAUTHUNIT_H
