/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/** @file base/stemmer.cpp
  *
  * Defines AbstractStemmer.
  *
  */

/****************************************************************************/

#include <hatn/base/stemmer.h>

HATN_BASE_NAMESPACE_BEGIN

//---------------------------------------------------------------

AbstractStemmer::~AbstractStemmer()
{}

//---------------------------------------------------------------

std::string AbstractStemmer::stemOrSelf(const LanguageTag& language, common::lib::string_view word) const
{
    auto stems=stem(language,word);
    if (!stems.empty())
    {
        return stems.front();
    }
    return std::string(word);
}

//---------------------------------------------------------------

HATN_BASE_NAMESPACE_END
