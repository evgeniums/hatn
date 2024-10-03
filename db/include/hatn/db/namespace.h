/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file db/namespace.h
  *
  * Contains declaration of db table namespace.
  *
  */

/****************************************************************************/

#ifndef HATNDBNAMESPACE_H
#define HATNDBNAMESPACE_H

#include <hatn/common/stdwrappers.h>
#include <hatn/common/pmr/pmrtypes.h>
#include <hatn/common/flatmap.h>
#include <hatn/common/fixedbytearray.h>

#include <hatn/db/db.h>
#include <hatn/db/objectid.h>

HATN_DB_NAMESPACE_BEGIN

class Namespace
{
    public:

        const lib::string_view& tenancyName() const noexcept
        {
            return m_tenancyName;
        }

        const lib::string_view& topic() const noexcept
        {
            return m_topic;
        }

    private:

        lib::string_view m_tenancyName;
        lib::string_view m_topic;
};

using Topic=common::pmr::string;
using Topics=common::pmr::FlatSet<Topic>;

HATN_DB_NAMESPACE_END

#endif // HATNDBNAMESPACE_H
