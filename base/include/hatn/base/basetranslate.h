/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file base/baseertranslate.h
  *
  * Contains declaration of translator for hatnbase library.
  *
  */

/****************************************************************************/

#ifndef HATNBASETRANSLATE_H
#define HATNBASETRANSLATE_H

#include <hatn/common/translate.h>
#include <hatn/base/base.h>

HATN_BASE_NAMESPACE_BEGIN

inline std::string _TRB(
    const std::string& phrase,
    const std::string& context="base"
    )
{
    return _TR(phrase,context);
}

HATN_BASE_NAMESPACE_END

#endif // HATNBASETRANSLATE_H
