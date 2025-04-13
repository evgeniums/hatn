/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/server/microserviceconfig.h
  *
  */

/****************************************************************************/

#ifndef HATNAPIMICROSERVICECONFIG_H
#define HATNAPIMICROSERVICECONFIG_H

#include <hatn/dataunit/syntax.h>

#include <hatn/api/api.h>

HATN_API_NAMESPACE_BEGIN

namespace server {

HDU_UNIT(microservice_config,
    HDU_FIELD(name,TYPE_STRING,1,true)
    HDU_FIELD(dispatcher,TYPE_STRING,2)
    HDU_FIELD(auth_dispatcher,TYPE_STRING,3)
)

} // namespace server

HATN_API_NAMESPACE_END

#endif // HATNAPIMICROSERVICECONFIG_H
