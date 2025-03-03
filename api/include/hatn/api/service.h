/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file api/service.h
  *
  */

/****************************************************************************/

#ifndef HATNAPISERVICE_H
#define HATNAPISERVICE_H

#include <hatn/api/api.h>
#include <hatn/api/withnameandversion.h>

HATN_API_NAMESPACE_BEGIN

class Service : public WithNameAndVersion<ServiceNameLengthMax>
{
    public:

        using WithNameAndVersion<ServiceNameLengthMax>::WithNameAndVersion;
};

HATN_API_NAMESPACE_END

#endif // HATNAPISERVICE_H
