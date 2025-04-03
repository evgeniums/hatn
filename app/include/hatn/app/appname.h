/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file app/appname.h
  *
  */

/****************************************************************************/

#ifndef HATNAPPNAME_H
#define HATNAPPNAME_H

#include <string>

#include <hatn/app/app.h>

HATN_APP_NAMESPACE_BEGIN

struct AppName
{
    std::string execName;
    std::string displayName;
};

HATN_APP_NAMESPACE_END

#endif // HATNAPPNAME_H
