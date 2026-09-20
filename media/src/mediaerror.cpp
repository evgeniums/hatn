/*
    Copyright (c) 2020 - current, Evgeny Sidorov (decfile.com), All rights reserved.

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)

*/

/****************************************************************************/
/*

*/
/** @file media/mediaerror.cpp
  *
  *      Contains definition of error category and library-wide helpers.
  *
  */

#include <hatn/common/translate.h>

#include <hatn/media/mediaerror.h>

HATN_MEDIA_NAMESPACE_BEGIN

/********************** MediaErrorCategory **************************/

//---------------------------------------------------------------
const MediaErrorCategory& MediaErrorCategory::getCategory() noexcept
{
    static MediaErrorCategory MediaErrorCategoryInstance;
    return MediaErrorCategoryInstance;
}

//---------------------------------------------------------------
std::string MediaErrorCategory::message(int code) const
{
    std::string result;
    switch (code)
    {
        HATN_MEDIA_ERRORS(HATN_ERROR_MESSAGE)

        default:
            result=_TR("unknown error");
    }

    return result;
}

//---------------------------------------------------------------
const char* MediaErrorCategory::codeString(int code) const
{
    return errorString(code,MediaErrorStrings);
}

//---------------------------------------------------------------
bool isOggOpusAvailable() noexcept
{
#ifdef HATN_MEDIA_HAS_OGG_OPUS
    return true;
#else
    return false;
#endif
}

//---------------------------------------------------------------

HATN_MEDIA_NAMESPACE_END
